#include "ERBlockStressProbe.h"
#include "ERProceduralMap.h"
#include "ERCharacter.h"
#include "ERWeapon.h"
#include "ERPlayerController.h"
#include "ERGrenadeExplosion.h"
#include "Camera/CameraComponent.h"
#include "Components/DecalComponent.h"
#include "NiagaraComponent.h"
#include "UObject/UObjectIterator.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "UObject/GarbageCollection.h"
#include "UnrealClient.h"
AERBlockStressProbe::AERBlockStressProbe() { PrimaryActorTick.bCanEverTick=true; }
void AERBlockStressProbe::Check(bool Value,const TCHAR* Name)
{
    if (!Value) { Failed=true;UE_LOG(LogTemp,Error,TEXT("ER_BLOCK_STRESS_FAIL %s case=%d sample=%d"),Name,Case,Sample); }
}
void AERBlockStressProbe::Record(AERProceduralMap* Map,const TCHAR* Op,const FERBlockOperationProfile& P,int32 Removed,int32 Candidates,int32 Examined)
{
    if (Sample<0) return;
    const auto M=Map->GetMemoryProfile();
    // Cell columns: chunk_cm = chunk edge in cm; clear/add/mapping = mesh/components/collision ms;
    // groups = chunks rebuilt; uploaded = quads; instance_bytes = CPU mesh copies.
    Rows.Add(FString::Printf(TEXT("%s,%s,%d,%d,%.0f,%d,%s,%.6f,%.6f,%.6f,%.6f,%.6f,%d,%d,%d,%d,%d,%llu,%llu,%llu,%llu,%llu,%d,%d,%llu,%llu,%d,%d,%llu,%lld,%d"),
        *Label,FApp::CanEverRender() ? TEXT("Metal") : TEXT("NullRHI"),Map->LayoutConfig.Seed,Map->LayoutConfig.RoomCount,Map->CellSettings.ChunkCells*Map->CellSettings.CellSize,Sample,Op,P.TotalMs,P.LogicalMs,P.MeshMs,P.AddMs,P.CollisionMs,P.Groups,P.Quads,Removed,Candidates,Examined,
        uint64(M.TrackedBytes()),uint64(M.LogicalBytes),uint64(M.LayoutBytes),uint64(M.MappingBytes),uint64(M.InstanceResourceBytes),M.CollisionBodies,M.Components,M.ResidentBytes,uint64(M.SupportBytes),M.Debris,M.SimulatingDebris,uint64(M.NetworkBytes),M.Cells,M.CollisionBoxes));
}
void AERBlockStressProbe::RunSample(AERProceduralMap* Map,AERCharacter* Character)
{
    Map->CellSettings.ChunkCells=Case%3==0 ? 16 : Case%3==1 ? 32 : 64;
    Map->LayoutConfig.Seed=Case<3 ? 42 : 7919; Map->LayoutConfig.RoomCount=10;
    Check(Map->Regenerate(),TEXT("regeneration"));
    Character->SetActorLocation(Map->GetSpawnLocation(),false,nullptr,ETeleportType::TeleportPhysics);
    Character->GetCharacterMovement()->StopMovementImmediately();
    Check(Map->ValidateCells(),TEXT("regenerated_chunks"));
    Record(Map,TEXT("generation"),Map->GenerationProfile);
    const auto& Cells=Map->GetCellWorld();
    // Random outer-layer cells away from panel ends, in a fixed order.
    TArray<FERCellId> Order;FRandomStream Random(7919);
    while (Order.Num()<272)
    {
        const int32 P=Random.RandRange(0,Cells.GetPanels().Num()-1);const auto& Panel=Cells.GetPanels()[P];
        if (Panel.bPane || Panel.Dims.X<6 || Panel.Dims.Z<6) continue;
        const FERCellId Id{P,Random.RandRange(2,Panel.Dims.X-3),Random.RandBool() ? 0 : Panel.Dims.Y-1,Random.RandRange(2,Panel.Dims.Z-3)};
        if (Cells.IsAlive(Id) && !Order.Contains(Id)) Order.Add(Id);
    }
    FCollisionQueryParams Query(SCENE_QUERY_STAT(ERBlockStress),false,Character);
    FHitResult Hit;
    auto CheckHole=[&](const FERCellId& Id)
    {
        const auto& Panel=Cells.GetPanels()[Id.Panel];
        const FVector N=Map->GetActorTransform().TransformVectorNoScale(Panel.Transform.GetRotation().GetAxisY())*(Id.Y==0 ? -1 : 1);
        const FVector C=Map->GetActorTransform().TransformPosition(Cells.GetCellCenter(Id));
        Check(!GetWorld()->LineTraceSingleByChannel(Hit,C+N*(Panel.Cell.Y*.5-.5),C,ECC_GameTraceChannel1,Query),TEXT("removed_volume_has_no_collision"));
    };
    for (int32 I=0;I<16;++I)
    {
        TMap<FERCellId,float> One;One.Add(Order[I],TNumericLimits<float>::Max());
        const auto B=Map->ApplyCellBatch(One);
        Check(B.Destroyed==1 && B.GroupsRebuilt>=1 && B.GroupsRebuilt<=3,TEXT("single_removal_dirty_chunk_and_neighbours"));
        Record(Map,TEXT("single"),Map->UpdateProfile,1);CheckHole(Order[I]);
    }
    TMap<FERCellId,float> Batch;
    for (int32 I=16;I<272;++I) Batch.FindOrAdd(Order[I])+=TNumericLimits<float>::Max();
    const auto Removed=Map->ApplyCellBatch(Batch);
    Check(Removed.Destroyed==256 && Removed.Damaged==256 && Removed.GroupsRebuilt<=Map->GetChunkCount(),TEXT("batch_unique_updates"));
    Record(Map,TEXT("batch256"),Map->UpdateProfile,Removed.Destroyed);
    for (int32 I=16;I<272;++I) CheckHole(Order[I]);
    Check(Map->ValidateCells(),TEXT("random_batch_chunks"));
    // Support check over the whole map after the batch, synchronously.
    Map->RequestSupportCheck();Map->StepSupport();
    FERBlockOperationProfile S;S.TotalMs=Map->LastSupportMs;S.LogicalMs=Map->GetSupport().LastMs;
    Record(Map,TEXT("support"),S,Map->LastDetached,Map->LastUnsupportedComponents,Map->GetSupport().LastRelabelled);
    while (Map->IsSupportPending()) Map->StepSupport();
    // HE grenade 10cm inside room 0's east wall, then the legacy linear blast.
    const auto& R=Map->GetLayout().Rooms[0];
    const FVector Inside=Map->GetActorTransform().TransformPosition(FVector(R.Max.X-10,(R.Min.Y+R.Max.Y)*.5f,110));
    const auto Grenade=AERBlockRoom::ApplyWorldExplosion(GetWorld(),Inside,FERBlockExplosionProfile(),Character);
    FERBlockOperationProfile G=Map->UpdateProfile;G.TotalMs=Grenade.TotalMs;G.LogicalMs=Grenade.QueryMs+Grenade.ExposureMs;
    Record(Map,TEXT("grenade_he"),G,Grenade.Destroyed,Grenade.Candidates,Grenade.CoverTraces);
    const auto Blast=AERBlockRoom::ApplyWorldExplosion(GetWorld(),Inside+FVector(0,0,15),250,2400,Character);
    FERBlockOperationProfile E=Map->UpdateProfile;E.TotalMs=Blast.TotalMs;E.LogicalMs=Blast.QueryMs+Blast.ExposureMs;
    Record(Map,TEXT("explosion250"),E,Blast.Destroyed,Blast.Candidates,Blast.CoverTraces);
    Check(Grenade.Destroyed>0 && Map->ValidateCells(),TEXT("explosion_chunks"));
    // Clear every panel around room 0 in one batch.
    const FBox Area=FBox(FVector(R.Min.X,R.Min.Y,0),FVector(R.Max.X,R.Max.Y,Map->Config.Height)).ExpandBy(FVector(60,60,0));
    TMap<FERCellId,float> All;
    for (int32 P=0;P<Cells.GetPanels().Num();++P)
    {
        const auto& Panel=Cells.GetPanels()[P];if (!Panel.Bounds.Intersect(Area)) continue;
        for (int32 Z=0;Z<Panel.Dims.Z;++Z)for (int32 Y=0;Y<Panel.Dims.Y;++Y)for (int32 X=0;X<Panel.Dims.X;++X)if (Cells.IsAlive({P,X,Y,Z}))All.Add({P,X,Y,Z},TNumericLimits<float>::Max());
    }
    const auto Cleared=Map->ApplyCellBatch(All);Record(Map,TEXT("clear_room"),Map->UpdateProfile,Cleared.Destroyed);
    Check(Cleared.Destroyed==All.Num() && Map->ValidateCells(),TEXT("room_cells_removed"));
    const FVector Spawn=Map->GetSpawnLocation();
    Check(GetWorld()->LineTraceSingleByChannel(Hit,Spawn,Spawn-FVector(0,0,200),ECC_WorldStatic,Query),TEXT("floor_preserved_after_room_destruction"));
    Check(!GetWorld()->SweepSingleByChannel(Hit,Spawn,Map->GetActorTransform().TransformPosition(FVector(R.Max.X+45,(R.Min.Y+R.Max.Y)*.5f,100)),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(34,92.5f),Query),TEXT("capsule_no_ghost_wall"));
    CollectGarbage(RF_NoFlags);
    Record(Map,TEXT("empty_after_gc"),{});
}
void AERBlockStressProbe::Finish()
{
    if (!FFileHelper::SaveStringArrayToFile(Rows,*Output)) Check(false,TEXT("save_csv"));
    UE_LOG(LogTemp,Display,TEXT("ER_BLOCK_STRESS_COMPLETE result=%s csv=%s rows=%d samples=%d"),Failed ? TEXT("FAIL") : TEXT("PASS"),*Output,Rows.Num()-1,Samples);
    FPlatformMisc::RequestExitWithStatus(false,Failed ? 1 : 0);Phase=99;
}
void AERBlockStressProbe::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);Elapsed+=DeltaSeconds;
    if (Phase==3) { if (Elapsed>2) Finish(); return; }
    TActorIterator<AERProceduralMap> It(GetWorld());auto* Map=It ? *It : nullptr;
    auto* PC=Cast<AERPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(),0));
    auto* C=PC ? Cast<AERCharacter>(PC->GetPawn()) : nullptr;
    if (!Map || !C || !C->Weapon) { if (Elapsed>30) {Check(false,TEXT("initialized"));Finish();}return; }
    if (!Started)
    {
        Started=true;FParse::Value(FCommandLine::Get(),TEXT("ERBlockBenchmarkLabel="),Label);
        Label=Label.Replace(TEXT("/"),TEXT("_")).Replace(TEXT("\\"),TEXT("_")).Replace(TEXT(","),TEXT("_"));
        FParse::Value(FCommandLine::Get(),TEXT("ERBlockBenchmarkSamples="),Samples);Samples=FMath::Clamp(Samples,1,30);
        BenchmarkOnly=FParse::Param(FCommandLine::Get(),TEXT("ERBlockBenchmarkOnly"));
        Output=FPaths::ProjectSavedDir()/TEXT("Testing")/(TEXT("destruction-benchmark-")+Label+(FApp::CanEverRender() ? TEXT("-metal.csv") : TEXT("-nullrhi.csv")));
        Rows.Add(TEXT("label,rhi,seed,rooms,chunk_cm,sample,operation,total_ms,logical_ms,clear_ms,add_ms,mapping_ms,groups,uploaded,removed,candidates,examined,tracked_bytes,logical_bytes,layout_bytes,mapping_bytes,instance_bytes,collision_bodies,components,resident_bytes,support_bytes,debris,simulating_debris,network_bytes,cells,collision_boxes"));
    }
    if (Phase==0)
    {
        if (Elapsed<2) return;
        if (Case<6)
        {
            RunSample(Map,C);++Sample;
            if (Sample>=Samples) {UE_LOG(LogTemp,Display,TEXT("ER_BLOCK_BENCHMARK_CASE seed=%d chunk=%.0f samples=%d result=%s"),Map->LayoutConfig.Seed,Map->CellSettings.ChunkCells*Map->CellSettings.CellSize,Samples,Failed ? TEXT("FAIL") : TEXT("PASS"));++Case;Sample=-1;FFileHelper::SaveStringArrayToFile(Rows,*Output);}
            return;
        }
        if (BenchmarkOnly) {Finish();return;}
        Map->CellSettings.ChunkCells=32;Map->LayoutConfig.Seed=7919;Map->Regenerate();
        C->SetActorLocation(Map->GetSpawnLocation(),false,nullptr,ETeleportType::TeleportPhysics);C->GetCharacterMovement()->StopMovementImmediately();
        PC->SetControlRotation(FRotator(0,0,0));
        // Room 0's wall in front of the spawn 1 HP from removal: every cartridge breaks cells.
        TMap<FERCellId,float> Prep;const auto& Cells=Map->GetCellWorld();
        for (int32 P=0;P<Cells.GetPanels().Num();++P) if (Cells.GetPanels()[P].Id/8==5)
        {
            const auto& Panel=Cells.GetPanels()[P];
            for (int32 Z=0;Z<Panel.Dims.Z;++Z)for (int32 Y=0;Y<Panel.Dims.Y;++Y)for (int32 X=0;X<Panel.Dims.X;++X)if (Cells.IsAlive({P,X,Y,Z}))Prep.Add({P,X,Y,Z},Cells.GetHealth({P,X,Y,Z})-1);
        }
        Map->ApplyCellBatch(Prep);InitialShots=C->Weapon->AuthorizedShotCount;C->Weapon->StartFire();Phase=1;Elapsed=0;
    }
    else if (Phase==1)
    {
        if (C->Weapon->MagazineAmmo>0 && Elapsed<8) return;
        C->Weapon->StopFire();Check(C->Weapon->AuthorizedShotCount==InitialShots+30,TEXT("real_automatic_magazine_30_shots"));
        Check(Map->GetCellWorld().GetRemovedCount()>0 && Map->ValidateCells(),TEXT("real_burst_chunks_and_destruction"));
        UE_LOG(LogTemp,Display,TEXT("ER_BLOCK_STRESS_BURST shots=%u destroyed=%lld"),C->Weapon->AuthorizedShotCount-InitialShots,Map->GetCellWorld().GetRemovedCount());
        if (FApp::CanEverRender()) FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Testing/Destruction-Stress-Burst.png"),true,false);
        Phase=2;Elapsed=0;
    }
    else if (Phase==2)
    {
        if (Elapsed<1) return;
        Map->Regenerate();
        C->SetActorLocation(Map->GetSpawnLocation(),false,nullptr,ETeleportType::TeleportPhysics);C->GetCharacterMovement()->StopMovementImmediately();
        const FERCellId Block{0,1,0,1};const FVector P=Map->GetCellTransform(Block).GetLocation();
        const EERBlockMaterial Material=Map->GetCellWorld().GetPanels()[0].Material;
        int32 Decals=0,Emitters=0,Bursts=0;
        if (FApp::CanEverRender())
        {
            for (int32 Budget:{0,24,1000})
            {
                Map->Regenerate();Map->MaxImpactEffects=Budget;
                for (int32 I=0;I<200;++I)
                {
                    Map->SpawnCellImpact(Material,P,FVector(0,1,0));
                    Map->GetEffectCounts(Decals,Emitters,Bursts);
                    Check(Decals<=FMath::Clamp(Budget,0,64) && Emitters<=FMath::Clamp(Budget,0,64),TEXT("impact_effect_budget_every_spawn"));
                }
                if (Budget>0) Check(Decals>0 && Emitters>0,TEXT("real_decals_and_niagara_created"));
            }
            const int32 OldDecals=Decals,OldEmitters=Emitters;
            Map->SpawnCellImpact(Material,C->Camera->GetComponentLocation()+FVector(10000,0,0),FVector::UpVector);
            Map->GetEffectCounts(Decals,Emitters,Bursts);
            Check(Decals==OldDecals && Emitters==OldEmitters,TEXT("distant_impacts_culled"));
            for (int32 I=0;I<32;++I)
            {
                Map->SpawnExplosionEffect(P);Map->GetEffectCounts(Decals,Emitters,Bursts);
                Check(Bursts<=8,TEXT("explosion_effect_budget_every_spawn"));
            }
            Check(Bursts==8,TEXT("real_bursts_created_and_capped"));
            UE_LOG(LogTemp,Display,TEXT("ER_BLOCK_STRESS_FX decals=%d emitters=%d bursts=%d"),Decals,Emitters,Bursts);
        }
        else
        {
            Map->SpawnCellImpact(Material,P,FVector::UpVector);Map->SpawnExplosionEffect(P);
            Map->GetEffectCounts(Decals,Emitters,Bursts);Check(Decals==0 && Emitters==0 && Bursts==0,TEXT("headless_has_no_cosmetic_fx"));
        }
        Map->Regenerate();Map->GetEffectCounts(Decals,Emitters,Bursts);
        Check(Decals==0 && Emitters==0 && Bursts==0,TEXT("regeneration_clears_owned_effects"));
        Map->MaxImpactEffects=24;
        if (FApp::CanEverRender())
        {
            TSet<UActorComponent*> ExistingFX;
            for (TObjectIterator<UActorComponent> Effect;Effect;++Effect)
                if ((Effect->IsA<UDecalComponent>() || Effect->IsA<UNiagaraComponent>()) && Effect->GetWorld()==GetWorld() && Effect->IsRegistered()) ExistingFX.Add(*Effect);
            Map->SpawnCellImpact(Material,P,FVector::UpVector);Map->SpawnExplosionEffect(P);
            TArray<TWeakObjectPtr<UActorComponent>> NewFX;
            for (TObjectIterator<UActorComponent> Effect;Effect;++Effect)
                if ((Effect->IsA<UDecalComponent>() || Effect->IsA<UNiagaraComponent>()) && Effect->GetWorld()==GetWorld() && Effect->IsRegistered() && !ExistingFX.Contains(*Effect)) NewFX.Add(*Effect);
            Check(NewFX.Num()>=2,TEXT("endplay_fixture_has_real_impact_components"));
            Map->Destroy();
            for (const auto& Effect:NewFX) Check(!Effect.IsValid() || !Effect->IsRegistered(),TEXT("room_endplay_no_orphan_impact_components"));
            int32 Remaining=0;for (TActorIterator<AERGrenadeExplosion> Burst(GetWorld());Burst;++Burst) ++Remaining;
            Check(Remaining==0,TEXT("room_endplay_cleans_owned_bursts"));
        }
        UE_LOG(LogTemp,Display,TEXT("ER_BLOCK_STRESS_LIFECYCLE result=%s"),Failed ? TEXT("FAIL") : TEXT("PASS"));
        Phase=3;Elapsed=0;
    }
    else if (Phase==3 && Elapsed>2) Finish();
}
