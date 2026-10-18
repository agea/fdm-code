#include "ERBlockCollapseProbe.h"
#include "ERBlockRoom.h"
#include "ERProceduralMap.h"
#include "ERBlockDebris.h"
#include "ERPlayerController.h"
#include "ERCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
AERBlockCollapseProbe::AERBlockCollapseProbe(){PrimaryActorTick.bCanEverTick=true;}
void AERBlockCollapseProbe::Check(bool Value,const TCHAR* Label)
{
    Failed|=!Value;UE_LOG(LogTemp,Display,TEXT("ER_COLLAPSE_PROBE_%s %s"),Value ? TEXT("PASS") : TEXT("FAIL"),Label);
}
namespace
{
// Removes a lattice (rows and columns every Step cells, plus the footing row) so
// every remaining island of every panel loses its support at once.
void CutLattice(AERBlockRoom* Room,int32 Step)
{
    TMap<FERCellId,float> Cut;const auto& Cells=Room->GetCellWorld();
    for(int32 P=0;P<Cells.GetPanels().Num();++P)
    {
        const auto& Panel=Cells.GetPanels()[P];
        for(int32 Z=0;Z<Panel.Dims.Z;++Z)for(int32 Y=0;Y<Panel.Dims.Y;++Y)for(int32 X=0;X<Panel.Dims.X;++X)
            if(Z%Step==0 || X%Step==0)Cut.Add({P,X,Y,Z},TNumericLimits<float>::Max());
    }
    Room->ApplyCellBatch(Cut);
}
// The console demo ring (wall 0, X 30..60, Z 17..40, 20cm deep below) on any room.
void CutRing(AERBlockRoom* Room)
{
    TMap<FERCellId,float> Ring;const auto& Cells=Room->GetCellWorld();
    for(int32 P=0;P<Cells.GetPanels().Num();++P)if(Cells.GetPanels()[P].Id/8==0)
        for(int32 X=30;X<=60;++X)for(int32 Z=17;Z<=40;++Z)for(int32 Y=0;Y<Cells.GetPanels()[P].Dims.Y;++Y)
            if(X==30 || X==60 || Z<=20 || Z==40)Ring.Add({P,X,Y,Z},TNumericLimits<float>::Max());
    Room->ApplyCellBatch(Ring);
}
int32 GlobalDebris(UWorld* World){int32 N=0;for(TActorIterator<AERBlockDebris> D(World);D;++D)++N;return N;}
}
void AERBlockCollapseProbe::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);Elapsed+=DeltaSeconds;
    auto* PC=Cast<AERPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(),0));
    auto* C=PC ? Cast<AERCharacter>(PC->GetPawn()) : nullptr;
    TActorIterator<AERBlockRoom> It(GetWorld());auto* Room=It ? *It : nullptr;
    if (!Room || !C) {if (Elapsed>30) {Check(false,TEXT("initialized"));FPlatformMisc::RequestExitWithStatus(false,1);}return;}
    const auto& Cells=Room->GetCellWorld();
    if (Phase==0 && Elapsed>2)
    {
        Room->Collapse.DebrisLifetime=4;Room->Collapse.DebrisSettleAfter=3;Room->Collapse.RemovalsPerStep=1;
        C->SetActorLocation(FVector(0,80,100),false,nullptr,ETeleportType::TeleportPhysics);C->GetCharacterMovement()->StopMovementImmediately();PC->SetControlRotation(FRotator(0,-90,0));
        Check(Room->GetSupport().IsBuilt() && Room->GetSupport().GetContactCount()>0,TEXT("cell_support_graph_with_panel_contacts"));
        UE_LOG(LogTemp,Display,TEXT("ER_COLLAPSE_SUPPORT contacts=%d build_ms=%.3f"),Room->GetSupport().GetContactCount(),Room->GetSupportBuildMs());
        if (FApp::CanEverRender()) FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Testing/Collapse-Before.png"),true,false);
        Phase=1;Elapsed=0;
    }
    else if (Phase==1 && Elapsed>.2)
    {
        PC->ConsoleCommand(TEXT("t.MaxFPS 5"));
        PC->ConsoleCommand(TEXT("ERBlockCollapseDemo"));Check(Room->IsSupportPending(),TEXT("event_driven_support_scheduled"));Phase=2;Elapsed=0;
    }
    else if (Phase==2)
    {
        FrameBudgetPassed&=Room->SupportSteps-PreviousSteps<=1;PreviousSteps=Room->SupportSteps;
        SlowFrameObserved|=DeltaSeconds>.1;
        if (Elapsed>2 && !FpsRestored) {PC->ConsoleCommand(TEXT("t.MaxFPS 60"));FpsRestored=true;}
        if (Room->LastSpawned>Room->Collapse.DebrisPerStep) Check(false,TEXT("configured_step_limits"));
        // Physics advances slowly at 5 FPS: measure the fall only at normal frame rate.
        if ((Room->IsSupportPending() || !FpsRestored) && Elapsed<15) return;
        Check(FrameBudgetPassed && SlowFrameObserved,TEXT("slow_frames_never_catch_up_multiple_steps"));
        Check(true,TEXT("configured_step_limits"));
        // Two drywall boards, each with a 29x19-cell island inside the ring.
        Check(!Room->IsSupportPending() && Cells.GetDetachedCount()==2*29*19,TEXT("isolated_patch_removed"));
        Check(Cells.IsAlive({0,10,0,10}) && Cells.IsAlive({0,45,0,50}) && Room->ValidateCells(),TEXT("anchored_wall_and_lintel_above_ring_stay"));
        int32 Alive=0,Physics=0;Room->GetDebrisCounts(Alive,Physics);Check(Alive==2 && Physics==2,TEXT("one_chaos_fragment_per_detached_component"));
        Phase=3;Elapsed=0;
    }
    else if (Phase==3 && Elapsed>.35)
    {
        int32 Fell=0;bool Responses=true;
        for (TActorIterator<AERBlockDebris> D(GetWorld());D;++D)
        {
            // The island's cells span Z 105..200cm (centre 150): fallen debris is clearly lower.
            if (D->GetActorLocation().Z<Room->GetActorLocation().Z+140) ++Fell;
            UE_LOG(LogTemp,Display,TEXT("ER_COLLAPSE_DEBRIS pos=%s scale=%s simulating=%d velocity=%s"),*D->GetActorLocation().ToString(),*D->GetActorScale3D().ToString(),D->IsSimulating(),*D->Mesh->GetPhysicsLinearVelocity().ToString());
            Responses&=D->Mesh->GetCollisionEnabled()==ECollisionEnabled::PhysicsOnly && D->Mesh->GetCollisionResponseToChannel(ECC_Pawn)==ECR_Ignore && D->Mesh->GetCollisionResponseToChannel(ECC_GameTraceChannel1)==ECR_Ignore;
        }
        Check(Fell==2,TEXT("actual_chaos_gravity_fall"));Check(Responses,TEXT("debris_does_not_block_players_or_shots"));
        if (FApp::CanEverRender()) FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Testing/Collapse-Falling.png"),true,false);
        UE_LOG(LogTemp,Display,TEXT("ER_COLLAPSE_TIMING graph_build_ms=%.3f evaluate_ms=%.3f peak_step_ms=%.3f steps=%d support_bytes=%llu tracked_bytes=%llu"),Room->GetSupportBuildMs(),Room->GetSupport().LastMs,Room->PeakSupportMs,Room->SupportSteps,uint64(Room->GetMemoryProfile().SupportBytes),uint64(Room->GetMemoryProfile().TrackedBytes()));
        Phase=4;Elapsed=0;
    }
    else if (Phase==4)
    {
        int32 Alive=0,Physics=0;Room->GetDebrisCounts(Alive,Physics);
        if (Physics>0 && Elapsed<5) return;
        Check(Alive==2 && Physics==0,TEXT("physics_deactivated_after_settle_budget"));Phase=5;Elapsed=0;
    }
    else if (Phase==5)
    {
        int32 Alive=0,Physics=0;Room->GetDebrisCounts(Alive,Physics);
        if (Alive>0 && Elapsed<5) return;
        Check(Alive==0,TEXT("debris_lifetime_cleanup"));
        Check(Room->Regenerate() && Cells.GetDetachedCount()==0,TEXT("regeneration_restores_support"));
        Room->Collapse.MaxDebris=10000;Room->Collapse.DebrisPerStep=10000;Room->Collapse.RemovalsPerStep=64;
        Room->Collapse.DebrisLifetime=20;Room->Collapse.DebrisSettleAfter=1;
        CutLattice(Room,6);
        Phase=6;Elapsed=0;
    }
    else if (Phase==6)
    {
        int32 Alive=0,Physics=0;Room->GetDebrisCounts(Alive,Physics);
        if (Room->LastSpawned>8) Check(false,TEXT("per_step_debris_cap"));
        if (Alive<64 && Room->IsSupportPending() && Elapsed<15) return;
        Check(Alive==64 && Room->LastSpawned<=8,TEXT("hard_global_64_and_per_step_8_caps"));
        ExtraRoom=GetWorld()->SpawnActor<AERBlockRoom>(FVector(3000,0,0),FRotator::ZeroRotator);
        ExtraRoom->Collapse.MaxDebris=10000;ExtraRoom->Collapse.DebrisPerStep=10000;ExtraRoom->Collapse.RemovalsPerStep=64;
        CutLattice(ExtraRoom.Get(),6);
        Phase=7;Elapsed=0;
    }
    else if (Phase==7)
    {
        if ((Room->IsSupportPending() || ExtraRoom->IsSupportPending()) && Elapsed<15) return;
        Check(GlobalDebris(GetWorld())==64,TEXT("global_cap_across_two_rooms"));
        Check(Cells.GetRemovedCount()==Cells.GetCellCount() && ExtraRoom->GetCellWorld().GetRemovedCount()==ExtraRoom->GetCellWorld().GetCellCount(),TEXT("overflow_removed_without_floating_cells"));
        Check(Room->GetMemoryProfile().CollisionBodies==0 && Room->Floor->IsCollisionEnabled(),TEXT("wall_collision_gone_floor_preserved"));
        Room->Regenerate();int32 Alive=0,Physics=0;Room->GetDebrisCounts(Alive,Physics);
        Check(Alive==0 && !Room->IsSupportPending() && Room->ValidateCells(),TEXT("regen_clears_owned_bodies_and_timer"));
        ExtraRoom->Regenerate();ExtraRoom->Collapse.MaxDebris=8;
        CutRing(ExtraRoom.Get());
        Phase=8;Elapsed=0;
    }
    else if (Phase==8)
    {
        if (ExtraRoom->IsSupportPending() && Elapsed<15) return;
        int32 Alive=0,Physics=0;ExtraRoom->GetDebrisCounts(Alive,Physics);Check(Alive>0,TEXT("endplay_fixture_has_debris"));
        ExtraRoom->Destroy();Phase=9;Elapsed=0;
    }
    else if (Phase==9 && Elapsed>.2)
    {
        Check(GlobalDebris(GetWorld())==0,TEXT("endplay_clears_owned_debris"));
        const FTransform T(FVector(6000,0,0));
        auto* Map=GetWorld()->SpawnActorDeferred<AERProceduralMap>(AERProceduralMap::StaticClass(),T,nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        Map->LayoutConfig.Seed=7919;Map->LayoutConfig.RoomCount=10;Map->FinishSpawning(T);ExtraRoom=Map;
        // Remove every footing row: the whole shell loses support at once.
        TMap<FERCellId,float> Base;const auto& MC=Map->GetCellWorld();
        for(int32 P=0;P<MC.GetPanels().Num();++P)for(int32 X=0;X<MC.GetPanels()[P].Dims.X;++X)for(int32 Y=0;Y<MC.GetPanels()[P].Dims.Y;++Y)Base.Add({P,X,Y,0},TNumericLimits<float>::Max());
        Map->ApplyCellBatch(Base);Check(MC.GetCellCount()>1000000,TEXT("ten_room_collapse_fixture"));
        Phase=10;Elapsed=0;LargeElapsed=GetWorld()->GetTimeSeconds();
    }
    else if (Phase==10)
    {
        if (ExtraRoom->LastSpawned>4) Check(false,TEXT("large_map_step_budgets"));
        if (LargeSteps.Num()<ExtraRoom->SupportSteps) LargeSteps.Add(ExtraRoom->LastSupportMs);
        if (ExtraRoom->IsSupportPending() && Elapsed<30) return;
        const auto& MC=ExtraRoom->GetCellWorld();
        Check(!ExtraRoom->IsSupportPending() && MC.GetRemovedCount()==MC.GetCellCount() && ExtraRoom->ValidateCells(),TEXT("ten_room_full_unsupported_shell_removed"));
        int32 Alive=0,Physics=0;ExtraRoom->GetDebrisCounts(Alive,Physics);Check(Alive<=32 && Physics<=32,TEXT("large_map_default_debris_budget"));
        LargeSteps.Sort();
        const double P95=LargeSteps.IsEmpty()?0:LargeSteps[FMath::Clamp(FMath::CeilToInt(LargeSteps.Num()*.95)-1,0,LargeSteps.Num()-1)];
        UE_LOG(LogTemp,Display,TEXT("ER_COLLAPSE_LARGE cells=%lld destroyed=%lld detached=%lld chunks=%d contacts=%d steps=%d graph_build_ms=%.3f evaluate_ms=%.3f step_p95_ms=%.3f peak_step_ms=%.3f completion_s=%.3f support_bytes=%llu"),
            MC.GetCellCount(),MC.GetRemovedCount()-MC.GetDetachedCount(),MC.GetDetachedCount(),MC.GetChunks().Num(),ExtraRoom->GetSupport().GetContactCount(),ExtraRoom->SupportSteps,ExtraRoom->GetSupportBuildMs(),ExtraRoom->GetSupport().LastMs,P95,ExtraRoom->PeakSupportMs,GetWorld()->GetTimeSeconds()-LargeElapsed,uint64(ExtraRoom->GetMemoryProfile().SupportBytes));
        ExtraRoom->Destroy();Phase=11;Elapsed=0;
    }
    else if (Phase==11 && Elapsed>.2)
    {
        Check(GlobalDebris(GetWorld())==0,TEXT("large_map_endplay_cleanup"));
        UE_LOG(LogTemp,Display,TEXT("ER_COLLAPSE_PROBE_COMPLETE result=%s %s"),Failed ? TEXT("FAIL") : TEXT("PASS"),*Room->GetStatistics());FPlatformMisc::RequestExitWithStatus(false,Failed ? 1 : 0);Phase=12;
    }
}
