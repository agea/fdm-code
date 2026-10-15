#include "ERBlockStressProbe.h"
#include "ERProceduralMap.h"
#include "ERCharacter.h"
#include "ERWeapon.h"
#include "ERPlayerController.h"
#include "ERGrenadeExplosion.h"
#include "Camera/CameraComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
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
    Rows.Add(FString::Printf(TEXT("%s,%s,%d,%d,%.0f,%d,%s,%.6f,%.6f,%.6f,%.6f,%.6f,%d,%d,%d,%d,%d,%llu,%llu,%llu,%llu,%llu,%d,%d,%llu,%llu,%d,%d,%llu"),
        *Label,FApp::CanEverRender() ? TEXT("Metal") : TEXT("NullRHI"),Map->LayoutConfig.Seed,Map->LayoutConfig.RoomCount,Map->Config.ChunkSize,Sample,Op,P.TotalMs,P.LogicalMs,P.ClearMs,P.AddMs,P.MappingMs,P.Groups,P.Uploaded,Removed,Candidates,Examined,
        uint64(M.TrackedBytes()),uint64(M.LogicalBytes),uint64(M.LayoutBytes),uint64(M.MappingBytes),uint64(M.InstanceResourceBytes),M.CollisionBodies,M.Components,M.ResidentBytes,uint64(M.SupportBytes),M.Debris,M.SimulatingDebris,uint64(M.NetworkBytes)));
}
void AERBlockStressProbe::RunSample(AERProceduralMap* Map,AERCharacter* Character)
{
    Map->Config.ChunkSize=Case%3==0 ? 200 : Case%3==1 ? 400 : 800;
    Map->LayoutConfig.Seed=Case<3 ? 42 : 7919; Map->LayoutConfig.RoomCount=10;
    Check(Map->Regenerate(),TEXT("regeneration"));
    Character->SetActorLocation(Map->GetSpawnLocation(),false,nullptr,ETeleportType::TeleportPhysics);
    Character->GetCharacterMovement()->StopMovementImmediately();
    Check(Map->ValidateMappings(),TEXT("regenerated_mappings"));
    Record(Map,TEXT("generation"),Map->GenerationProfile);
    const auto Before=Map->GetMemoryProfile();
    const int32 Initial=Map->GetInstanceCount();
    const int32 FloorBodies=Before.CollisionBodies-Initial;
    Check(FloorBodies>0,TEXT("registered_floor_bodies"));
    TArray<FERBlockId> Order;
    for (const auto& B:Map->BlockWorld->GetBlocks()) Order.Add(B.Id);
    FRandomStream Random(7919);
    for (int32 I=Order.Num()-1;I>0;--I) Order.Swap(I,Random.RandRange(0,I));
    FCollisionQueryParams Query(SCENE_QUERY_STAT(ERBlockStress),false,Character);
    FHitResult Hit;
    auto CheckHole=[&](const FERBlockId& Id)
    {
        const auto T=Map->GetBlockTransform(Id); const FVector P=T.GetLocation(),N=T.GetRotation().GetAxisY();
        Check(!GetWorld()->LineTraceSingleByChannel(Hit,P+N*9,P-N*9,ECC_GameTraceChannel1,Query),TEXT("removed_volume_has_no_collision"));
    };
    for (int32 I=0;I<16;++I)
    {
        const auto B=Map->ApplyDamageBatch({{Order[I],0,true}});
        Check(B.Destroyed==1 && B.GroupsRebuilt==1,TEXT("single_removal_one_group"));
        Record(Map,TEXT("single"),Map->UpdateProfile,1);CheckHole(Order[I]);
    }
    TArray<FERBlockDamageRequest> Batch;
    for (int32 I=16;I<272;++I) {Batch.Add({Order[I],0,true});Batch.Add({Order[I],0,true});}
    const auto Removed=Map->ApplyDamageBatch(Batch);
    Check(Removed.Destroyed==256 && Removed.Damaged==256 && Removed.GroupsRebuilt<=Map->GetGroupCount(),TEXT("batch_unique_updates"));
    Record(Map,TEXT("batch256"),Map->UpdateProfile,Removed.Destroyed);
    for (int32 I=16;I<272;++I) CheckHole(Order[I]);
    Check(Map->ValidateMappings(),TEXT("random_batch_identity"));
    const auto After=Map->GetMemoryProfile();
    Check(After.CollisionBodies==Before.CollisionBodies-272,TEXT("destroyed_bodies_released"));
    // Query exactly the same conservative centre test as the documented index,
    // but brute-force all live records independently to detect stale/missed IDs.
    float HalfDiagonal=0;
    for (const auto& B:Map->BlockWorld->GetBlocks()) HalfDiagonal=FMath::Max(HalfDiagonal,float(B.LocalTransform.GetScale3D().Size()*50));
    for (int32 I=0;I<32;++I)
    {
        const FVector P=Map->BlockWorld->Find(Order[Random.RandRange(0,Order.Num()-1)])->LocalTransform.GetLocation();
        const auto Found=Map->BlockWorld->QuerySphere(P,250);
        TSet<FERBlockId> Expected;
        for (const auto& B:Map->BlockWorld->GetBlocks()) if (B.IsAttached() && FVector::DistSquared(P,B.LocalTransform.GetLocation())<=FMath::Square(250.+HalfDiagonal)) Expected.Add(B.Id);
        Check(Expected.Num()==Found.Num(),TEXT("spatial_query_matches_live_reference"));
        for (const auto& Id:Found) Check(Expected.Contains(Id),TEXT("no_stale_spatial_candidate"));
        FERBlockOperationProfile Q;Q.TotalMs=Map->BlockWorld->LastQuery.Ms;
        Record(Map,TEXT("query250"),Q,0,Found.Num(),Map->BlockWorld->LastQuery.Examined);
    }
    // Snapshot cover, candidate queries and batch mutation timed separately.
    const auto& R=Map->GetLayout().Rooms[0];
    const auto Blast=AERBlockRoom::ApplyWorldExplosion(GetWorld(),FVector(R.Max.X-10,(R.Min.Y+R.Max.Y)*.5f,125),250,2400,Character);
    FERBlockOperationProfile E;E.TotalMs=Blast.TotalMs;E.LogicalMs=Blast.QueryMs;E.ClearMs=Blast.ExposureMs;E.AddMs=Blast.MutationMs;E.Groups=Blast.GroupsRebuilt;
    Record(Map,TEXT("explosion250"),E,Blast.Destroyed,Blast.Candidates,Blast.CoverTraces);
    Check(Map->ValidateMappings(),TEXT("explosion_identity"));
    Batch.Reset(); for (const auto& B:Map->BlockWorld->GetBlocks()) if (B.IsAttached()) Batch.Add({B.Id,0,true});
    const auto All=Map->ApplyDamageBatch(Batch);Record(Map,TEXT("clear_all"),Map->UpdateProfile,All.Destroyed);
    Check(Map->GetInstanceCount()==0 && Map->ValidateMappings(),TEXT("all_instances_removed"));
    Check(Map->GetMemoryProfile().CollisionBodies==FloorBodies,TEXT("all_wall_bodies_released"));
    for (const auto& Id:Order) CheckHole(Id);
    const FVector Spawn=Map->GetSpawnLocation();
    Check(GetWorld()->LineTraceSingleByChannel(Hit,Spawn,Spawn-FVector(0,0,200),ECC_WorldStatic,Query),TEXT("floor_preserved_after_full_destruction"));
    Check(!GetWorld()->SweepSingleByChannel(Hit,Spawn,Spawn+FVector(500,0,0),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(34,92.5f),Query),TEXT("capsule_no_ghost_wall"));
    Check(Map->BlockWorld->QuerySphere(Spawn,500).IsEmpty(),TEXT("empty_world_query"));
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
        Rows.Add(TEXT("label,rhi,seed,rooms,chunk_cm,sample,operation,total_ms,logical_ms,clear_ms,add_ms,mapping_ms,groups,uploaded,removed,candidates,examined,tracked_bytes,logical_bytes,layout_bytes,mapping_bytes,instance_bytes,collision_bodies,components,resident_bytes,support_bytes,debris,simulating_debris,network_bytes"));
    }
    if (Phase==0)
    {
        if (Elapsed<2) return;
        if (Case<6)
        {
            RunSample(Map,C);++Sample;
            if (Sample>=Samples) {UE_LOG(LogTemp,Display,TEXT("ER_BLOCK_BENCHMARK_CASE seed=%d chunk=%.0f samples=%d result=%s"),Map->LayoutConfig.Seed,Map->Config.ChunkSize,Samples,Failed ? TEXT("FAIL") : TEXT("PASS"));++Case;Sample=-1;FFileHelper::SaveStringArrayToFile(Rows,*Output);}
            return;
        }
        if (BenchmarkOnly) {Finish();return;}
        Map->Config.ChunkSize=400;Map->LayoutConfig.Seed=7919;Map->Regenerate();
        C->SetActorLocation(Map->GetSpawnLocation(),false,nullptr,ETeleportType::TeleportPhysics);C->GetCharacterMovement()->StopMovementImmediately();
        PC->SetControlRotation(FRotator(0,0,0));
        TArray<FERBlockDamageRequest> Prep;
        for (const auto& B:Map->BlockWorld->GetBlocks()) if (B.Id.Wall==5) Prep.Add({B.Id,B.Health-1,false});
        Map->ApplyDamageBatch(Prep);InitialShots=C->Weapon->AuthorizedShotCount;C->Weapon->StartFire();Phase=1;Elapsed=0;
    }
    else if (Phase==1)
    {
        if (C->Weapon->MagazineAmmo>0 && Elapsed<8) return;
        C->Weapon->StopFire();Check(C->Weapon->AuthorizedShotCount==InitialShots+30,TEXT("real_automatic_magazine_30_shots"));
        Check(Map->BlockWorld->GetDestroyedCount()>0 && Map->ValidateMappings(),TEXT("real_burst_identity_and_destruction"));
        UE_LOG(LogTemp,Display,TEXT("ER_BLOCK_STRESS_BURST shots=%u destroyed=%d"),C->Weapon->AuthorizedShotCount-InitialShots,Map->BlockWorld->GetDestroyedCount());
        if (FApp::CanEverRender()) FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Testing/Destruction-Stress-Burst.png"),true,false);
        Phase=2;Elapsed=0;
    }
    else if (Phase==2)
    {
        if (Elapsed<1) return;
        Map->Regenerate();
        C->SetActorLocation(Map->GetSpawnLocation(),false,nullptr,ETeleportType::TeleportPhysics);C->GetCharacterMovement()->StopMovementImmediately();
        const auto Block=Map->BlockWorld->GetBlocks()[0];const FVector P=Map->GetBlockTransform(Block.Id).GetLocation();
        int32 Decals=0,Emitters=0,Bursts=0;
        if (FApp::CanEverRender())
        {
            for (int32 Budget:{0,24,1000})
            {
                Map->Regenerate();Map->MaxImpactEffects=Budget;
                for (int32 I=0;I<200;++I)
                {
                    Map->SpawnBlockImpact(Block.Id,P,FVector(0,1,0));
                    Map->GetEffectCounts(Decals,Emitters,Bursts);
                    Check(Decals<=FMath::Clamp(Budget,0,64) && Emitters<=FMath::Clamp(Budget,0,64),TEXT("impact_effect_budget_every_spawn"));
                }
                if (Budget>0) Check(Decals>0 && Emitters>0,TEXT("real_decals_and_niagara_created"));
            }
            const int32 OldDecals=Decals,OldEmitters=Emitters;
            Map->SpawnBlockImpact(Block.Id,C->Camera->GetComponentLocation()+FVector(10000,0,0),FVector::UpVector);
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
            Map->SpawnBlockImpact(Block.Id,P,FVector::UpVector);Map->SpawnExplosionEffect(P);
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
            Map->SpawnBlockImpact(Block.Id,P,FVector::UpVector);Map->SpawnExplosionEffect(P);
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
