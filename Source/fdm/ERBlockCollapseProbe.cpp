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
void AERBlockCollapseProbe::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);Elapsed+=DeltaSeconds;
    auto* PC=Cast<AERPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(),0));
    auto* C=PC ? Cast<AERCharacter>(PC->GetPawn()) : nullptr;
    TActorIterator<AERBlockRoom> It(GetWorld());auto* Room=It ? *It : nullptr;
    if (!Room || !C) {if (Elapsed>30) {Check(false,TEXT("initialized"));FPlatformMisc::RequestExitWithStatus(false,1);}return;}
    if (Phase==0 && Elapsed>2)
    {
        Room->Collapse.DebrisLifetime=2;Room->Collapse.DebrisSettleAfter=1;Room->Collapse.WorkPerStep=64;Room->Collapse.RemovalsPerStep=4;
        C->SetActorLocation(FVector(0,80,100),false,nullptr,ETeleportType::TeleportPhysics);C->GetCharacterMovement()->StopMovementImmediately();PC->SetControlRotation(FRotator(0,-90,0));
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
        if (Room->LastSupportWork>64 || Room->LastDetached>4 || Room->LastSpawned>4) Check(false,TEXT("configured_step_limits"));
        if (Room->IsSupportPending() && Elapsed<15) return;
        Check(FrameBudgetPassed && SlowFrameObserved,TEXT("slow_frames_never_catch_up_multiple_steps"));
        Check(true,TEXT("configured_step_limits"));
        Check(!Room->IsSupportPending() && Room->BlockWorld->GetDetachedCount()==9,TEXT("isolated_patch_removed"));
        Check(Room->BlockWorld->Find({0,2,6})->IsAttached() && Room->ValidateMappings(),TEXT("anchored_wall_and_native_mappings"));
        int32 Alive=0,Physics=0;Room->GetDebrisCounts(Alive,Physics);Check(Alive==9 && Physics==9,TEXT("nine_selected_chaos_bodies"));
        Phase=3;Elapsed=0;
    }
    else if (Phase==3 && Elapsed>.35)
    {
        int32 Fell=0;bool Responses=true;
        for (TActorIterator<AERBlockDebris> D(GetWorld());D;++D)
        {
            const auto* B=Room->BlockWorld->Find(D->SourceId);
            if (B && D->GetActorLocation().Z<Room->GetBlockTransform(B->Id).GetLocation().Z-10) ++Fell;
            Responses&=D->Mesh->GetCollisionEnabled()==ECollisionEnabled::PhysicsOnly && D->Mesh->GetCollisionResponseToChannel(ECC_Pawn)==ECR_Ignore && D->Mesh->GetCollisionResponseToChannel(ECC_GameTraceChannel1)==ECR_Ignore;
        }
        Check(Fell==9,TEXT("actual_chaos_gravity_fall"));Check(Responses,TEXT("debris_does_not_block_players_or_shots"));
        if (FApp::CanEverRender()) FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Testing/Collapse-Falling.png"),true,false);
        UE_LOG(LogTemp,Display,TEXT("ER_COLLAPSE_TIMING graph_build_ms=%.3f peak_step_ms=%.3f steps=%d tracked_bytes=%llu"),Room->GetSupportBuildMs(),Room->PeakSupportMs,Room->SupportSteps,uint64(Room->GetMemoryProfile().TrackedBytes()));
        Phase=4;Elapsed=0;
    }
    else if (Phase==4 && Elapsed>.85)
    {
        int32 Alive=0,Physics=0;Room->GetDebrisCounts(Alive,Physics);Check(Alive==9 && Physics==0,TEXT("physics_deactivated_after_settle_budget"));Phase=5;Elapsed=0;
    }
    else if (Phase==5 && Elapsed>1)
    {
        int32 Alive=0,Physics=0;Room->GetDebrisCounts(Alive,Physics);Check(Alive==0,TEXT("debris_lifetime_cleanup"));
        Check(Room->Regenerate() && Room->BlockWorld->GetDetachedCount()==0,TEXT("regeneration_restores_support"));
        Room->Collapse.MaxDebris=10000;Room->Collapse.DebrisPerStep=10000;Room->Collapse.RemovalsPerStep=8;Room->Collapse.WorkPerStep=16384;
        Room->Collapse.DebrisLifetime=20;Room->Collapse.DebrisSettleAfter=1;
        TArray<FERBlockDamageRequest> Base;for (const auto& B:Room->BlockWorld->GetBlocks()) if (B.Id.Row==0) Base.Add({B.Id,0,true});Room->ApplyDamageBatch(Base);
        Phase=6;Elapsed=0;
    }
    else if (Phase==6)
    {
        int32 Alive=0,Physics=0;Room->GetDebrisCounts(Alive,Physics);
        if (Alive<64 && Room->IsSupportPending() && Elapsed<15) return;
        Check(Alive==64 && Room->LastSpawned<=8,TEXT("hard_global_64_and_per_step_8_caps"));
        ExtraRoom=GetWorld()->SpawnActor<AERBlockRoom>(FVector(3000,0,0),FRotator::ZeroRotator);
        ExtraRoom->Collapse.MaxDebris=10000;ExtraRoom->Collapse.DebrisPerStep=10000;
        TArray<FERBlockDamageRequest> Base;for(const auto& B:ExtraRoom->BlockWorld->GetBlocks()) if(B.Id.Row==0) Base.Add({B.Id,0,true});ExtraRoom->ApplyDamageBatch(Base);
        Phase=7;Elapsed=0;
    }
    else if (Phase==7)
    {
        if ((Room->IsSupportPending() || ExtraRoom->IsSupportPending()) && Elapsed<15) return;
        int32 Global=0;for(TActorIterator<AERBlockDebris> D(GetWorld());D;++D) ++Global;
        Check(Global==64,TEXT("global_cap_across_two_rooms"));
        Check(Room->BlockWorld->GetDetachedCount()==462 && ExtraRoom->BlockWorld->GetDetachedCount()==462,TEXT("overflow_removed_without_floating_blocks"));
        Check(Room->GetMemoryProfile().CollisionBodies==0 && Room->Floor->IsCollisionEnabled(),TEXT("wall_collision_gone_floor_preserved"));
        Room->Regenerate();int32 Alive=0,Physics=0;Room->GetDebrisCounts(Alive,Physics);
        Check(Alive==0 && !Room->IsSupportPending() && Room->ValidateMappings(),TEXT("regen_clears_owned_bodies_and_timer"));
        ExtraRoom->Regenerate();ExtraRoom->Collapse.MaxDebris=8;
        TArray<FERBlockDamageRequest> Ring;for(int32 Col=3;Col<=7;++Col) for(int32 R=5;R<=9;++R) if(Col==3 || Col==7 || R==5 || R==9) Ring.Add({{0,Col,R},0,true});ExtraRoom->ApplyDamageBatch(Ring);
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
        int32 Global=0;for(TActorIterator<AERBlockDebris> D(GetWorld());D;++D) ++Global;Check(Global==0,TEXT("endplay_clears_owned_debris"));
        const FTransform T(FVector(6000,0,0));
        auto* Map=GetWorld()->SpawnActorDeferred<AERProceduralMap>(AERProceduralMap::StaticClass(),T,nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        Map->LayoutConfig.Seed=7919;Map->LayoutConfig.RoomCount=10;Map->FinishSpawning(T);ExtraRoom=Map;
        TArray<FERBlockDamageRequest> Base;for(const auto& B:Map->BlockWorld->GetBlocks()) if (B.LocalTransform.GetLocation().Z<13) Base.Add({B.Id,0,true});
        Map->ApplyDamageBatch(Base);Check(Map->GetInstanceCount()>9000,TEXT("ten_room_collapse_fixture"));
        Phase=10;Elapsed=0;LargeElapsed=GetWorld()->GetTimeSeconds();
    }
    else if (Phase==10)
    {
        if (ExtraRoom->LastDetached>128 || ExtraRoom->LastSpawned>4 || ExtraRoom->LastSupportWork>4096) Check(false,TEXT("large_map_step_budgets"));
        if (LargeSteps.Num()<ExtraRoom->SupportSteps) LargeSteps.Add(ExtraRoom->LastSupportMs);
        if (ExtraRoom->IsSupportPending() && Elapsed<20) return;
        Check(!ExtraRoom->IsSupportPending() && ExtraRoom->GetInstanceCount()==0 && ExtraRoom->ValidateMappings(),TEXT("ten_room_full_unsupported_shell_removed"));
        int32 Alive=0,Physics=0;ExtraRoom->GetDebrisCounts(Alive,Physics);Check(Alive<=32 && Physics<=32,TEXT("large_map_default_debris_budget"));
        LargeSteps.Sort();
        const double P95=LargeSteps.IsEmpty()?0:LargeSteps[FMath::Clamp(FMath::CeilToInt(LargeSteps.Num()*.95)-1,0,LargeSteps.Num()-1)];
        UE_LOG(LogTemp,Display,TEXT("ER_COLLAPSE_LARGE blocks=%d destroyed=%d detached=%d steps=%d graph_build_ms=%.3f step_p95_ms=%.3f peak_step_ms=%.3f completion_s=%.3f support_bytes=%llu"),ExtraRoom->BlockWorld->GetBlocks().Num(),ExtraRoom->BlockWorld->GetDestroyedCount(),ExtraRoom->BlockWorld->GetDetachedCount(),ExtraRoom->SupportSteps,ExtraRoom->GetSupportBuildMs(),P95,ExtraRoom->PeakSupportMs,GetWorld()->GetTimeSeconds()-LargeElapsed,uint64(ExtraRoom->GetMemoryProfile().SupportBytes));
        ExtraRoom->Destroy();Phase=11;Elapsed=0;
    }
    else if (Phase==11 && Elapsed>.2)
    {
        int32 Global=0;for(TActorIterator<AERBlockDebris> D(GetWorld());D;++D) ++Global;Check(Global==0,TEXT("large_map_endplay_cleanup"));
        UE_LOG(LogTemp,Display,TEXT("ER_COLLAPSE_PROBE_COMPLETE result=%s %s"),Failed ? TEXT("FAIL") : TEXT("PASS"),*Room->GetStatistics());FPlatformMisc::RequestExitWithStatus(false,Failed ? 1 : 0);Phase=12;
    }
}
