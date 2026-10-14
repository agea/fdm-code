#include "ERBotProbe.h"
#include "ERBotController.h"
#include "ERCharacter.h"
#include "ERMatch.h"
#include "ERWeapon.h"
#include "ERPlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

AERBotProbe::AERBotProbe() { bReplicates = true; bAlwaysRelevant = true; PrimaryActorTick.bCanEverTick = true; }
void AERBotProbe::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const { Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME(AERBotProbe,Phase); }
void AERBotProbe::Check(bool Value, const TCHAR* Label)
{
    if (!Value) bFailed = true;
    UE_LOG(LogTemp,Display,TEXT("ER_BOT_TEST_%s %s phase=%d"),Value ? TEXT("PASS") : TEXT("FAIL"),Label,Phase);
}
void AERBotProbe::Advance(int32 Next, double Delay) { Phase = Next; Deadline = GetWorld()->GetTimeSeconds()+Delay; ForceNetUpdate(); }
void AERBotProbe::PlacePlayers(bool bAcrossWall)
{
    const float Y = bAcrossWall ? 0 : -1500;
    Host->GetPawn()->TeleportTo(FVector(-1000,Y,90),FRotator::ZeroRotator);
    Bot->GetPawn()->TeleportTo(FVector(1000,Y,90),FRotator(0,180,0));
    CastChecked<AERCharacter>(Host->GetPawn())->GetCharacterMovement()->StopMovementImmediately();
    CastChecked<AERCharacter>(Bot->GetPawn())->GetCharacterMovement()->StopMovementImmediately();
    Host->SetControlRotation(FRotator::ZeroRotator); Host->ClientSetRotation(FRotator::ZeroRotator);
    Bot->SetControlRotation(FRotator(0,180,0));
}
void AERBotProbe::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("ERTargetSkinsProbe")))
    {
        if (GetWorld()->GetTimeSeconds() < (HasAuthority() ? 20.f : 5.f)) return;
        int32 AlphaMask = 0, OmegaMask = 0, Count = 0;
        bool bValid = true;
        for (TActorIterator<AERCharacter> It(GetWorld()); It; ++It)
        {
            const auto* PS = It->GetPlayerState<AERPlayerState>();
            if (!PS || !PS->IsABot()) continue;
            ++Count;
            bValid &= PS->ActiveSkin >= 0 && PS->ActiveSkin < ERSkins::Count &&
                It->GetTeamAppearance().Matches(It->GetMesh()) && It->GetVelocity().Size() < 1.f &&
                !It->Health->bDead && It->Weapon && It->Weapon->MagazineAmmo == 30;
            if (PS->ActiveSkin >= 0 && PS->ActiveSkin < ERSkins::Count)
            {
                if (PS->Team == EERTeam::Alpha) AlphaMask |= 1 << PS->ActiveSkin;
                else if (PS->Team == EERTeam::Omega) OmegaMask |= 1 << PS->ActiveSkin;
                else bValid = false;
            }
        }
        bValid &= Count == 2 * ERSkins::Count && AlphaMask == 31 && OmegaMask == 31;
        if (!HasAuthority() && (!bValid || bReplicaSeen)) return;
        if (HasAuthority()) bValid &= GetWorld()->GetAuthGameMode<AERGameMode>()->GetNumPlayers() == 2;
        UE_LOG(LogTemp,Display,TEXT("ER_TARGET_SKINS_%s side=%s count=%d alpha=%d omega=%d"),
            bValid ? TEXT("PASS") : TEXT("FAIL"), HasAuthority() ? TEXT("host") : TEXT("client"), Count, AlphaMask, OmegaMask);
        bReplicaSeen = true;
        if (HasAuthority()) FPlatformMisc::RequestExitWithStatus(false, bValid ? 0 : 1);
        return;
    }
    if (!HasAuthority())
    {
        for (TActorIterator<AERCharacter> It(GetWorld()); It; ++It)
        {
            const auto* PS = It->GetPlayerState<AERPlayerState>();
            if (!PS || !PS->IsABot()) continue;
            if (!bReplicaSeen && PS->Team != EERTeam::None && It->Health->Health == 100)
            {
                bReplicaSeen = true; UE_LOG(LogTemp,Display,TEXT("ER_BOT_CLIENT_PASS pawn_team_health_replicated"));
            }
            if (!bReplicaDeathSeen && It->Health->bDead && PS->Deaths == 1)
            {
                bReplicaDeathSeen = true; UE_LOG(LogTemp,Display,TEXT("ER_BOT_CLIENT_PASS bot_death_score_replicated"));
            }
        }
        return;
    }
    auto* Mode = GetWorld()->GetAuthGameMode<AERGameMode>();
    auto* GS = GetWorld()->GetGameState<AERGameState>();
    if (!Mode || !GS) return;
    const double Now = GetWorld()->GetTimeSeconds();
    if (Phase == 0)
    {
        for (TActorIterator<AERBotController> It(GetWorld()); It; ++It) Bot = *It;
        for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
            if (auto* PC = Cast<AERPlayerController>(It->Get()); PC && PC->IsLocalController()) Host = PC;
        if (!Bot || !Host || !Bot->GetPawn() || !Host->GetPawn() || Mode->GetNumPlayers() != 2) return;
        // Isolate patrol from combat: while a client starts, the bot may already
        // have reached a lane and stopped to engage the host.
        bTargetMode = Bot->bTrainingTarget;
        Host->GetPawn()->TeleportTo(FVector(-2400,0,100),FRotator::ZeroRotator);
        APawn* BotPawn = Bot->GetPawn(); Bot->UnPossess();
        BotPawn->TeleportTo(FVector(2400,0,100),FRotator(0,180,0)); Bot->Possess(BotPawn);
        InitialPosition = Bot->GetPawn()->GetActorLocation();
        Check(GS->IsMatchInProgress() && Bot->GetPlayerState<AERPlayerState>()->Team != Host->GetPlayerState<AERPlayerState>()->Team,TEXT("bot_opposite_team_match_started"));
        TArray<AERBotPatrolPoint*> Route;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(ERBotRouteTest),false);
        for (TActorIterator<AERCharacter> It(GetWorld()); It; ++It) Params.AddIgnoredActor(*It);
        for (TActorIterator<AERBotPatrolPoint> It(GetWorld()); It; ++It) Route.Add(*It);
        Route.Sort([](const AERBotPatrolPoint& A, const AERBotPatrolPoint& B) { return A.RouteIndex < B.RouteIndex; });
        bool ClearRoute = Route.Num() == 12;
        for (int32 I=0; I<Route.Num(); ++I)
        {
            FHitResult Hit;
            ClearRoute &= !GetWorld()->SweepSingleByChannel(Hit,Route[I]->GetActorLocation(),Route[(I+1)%Route.Num()]->GetActorLocation(),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(34,92.5f),Params);
        }
        Check(ClearRoute,TEXT("all_patrol_segments_clear_for_capsule"));
        for (TActorIterator<AERCharacter> It(GetWorld()); It; ++It) if (It->Controller != Bot) It->Health->ProtectionEndServerTime=Now+60;
        Advance(1,2); return;
    }
    auto* H = Cast<AERCharacter>(Host->GetPawn());
    auto* B = Cast<AERCharacter>(Bot->GetPawn());
    if (!H || !B || !H->Weapon || !B->Weapon) { Check(false,TEXT("valid_pawns")); FPlatformMisc::RequestExitWithStatus(false,1); return; }
    // Stop after the first actual hit, so the rest of the probe retains a living host.
    if (Phase == 2 && !bTargetMode && H->Health->Health < 100) Deadline = Now;
    if (Now < Deadline) return;
    switch (Phase)
    {
    case 1:
        Check(B->GetTeamAppearance().Matches(B->GetMesh()), TEXT("bot_team_mesh_animation_hidden_equipment"));
        UE_LOG(LogTemp,Display,TEXT("ER_BOT_PATROL displacement=%.1f velocity=%.1f local=%d ignored=%d"),FVector::Dist2D(InitialPosition,B->GetActorLocation()),B->GetVelocity().Size2D(),B->IsLocallyControlled(),B->IsMoveInputIgnored());
        Check(bTargetMode ? FVector::Dist2D(InitialPosition,B->GetActorLocation()) < 1 : FVector::Dist2D(InitialPosition,B->GetActorLocation()) > 100,
            bTargetMode ? TEXT("target_stays_stationary") : TEXT("bot_patrol_moves"));
        PlacePlayers(false); H->Health->ProtectionEndServerTime=0; Advance(2,4); break;
    case 2:
        Check(bTargetMode ? B->Weapon->MagazineAmmo==30 && H->Health->Health==100 : B->Weapon->MagazineAmmo<30 && H->Health->Health<100,
            bTargetMode ? TEXT("target_never_fires") : TEXT("bot_real_projectile_damage_ammo"));
        B->ServerStopFire(); PlacePlayers(true); AmmoBeforeWall=B->Weapon->MagazineAmmo; H->Health->ProtectionEndServerTime=Now+60;
        Advance(3,1); break;
    case 3:
        Check(B->Weapon->MagazineAmmo==AmmoBeforeWall,TEXT("central_wall_blocks_bot_sight_fire"));
        B->Weapon->MagazineAmmo=0; B->Weapon->ReserveAmmo=90;
        if (bTargetMode) B->ServerReload();
        Advance(4,.4); break;
    case 4:
        Check(B->Weapon->bReloading,TEXT("bot_shared_reload_started")); Bot->bTrainingTarget=true; Advance(5,1.3); break;
    case 5:
        Check(B->Weapon->MagazineAmmo==30 && B->Weapon->ReserveAmmo==60 && !B->Weapon->bReloading,TEXT("bot_shared_reload_ammo_transfer"));
        PlacePlayers(false); B->Health->ProtectionEndServerTime=0; H->ServerStartFire(); Advance(6,.8); break;
    case 6:
        H->ServerStopFire(); DeadPawn=B;
        Check(B->Health->bDead && Bot->bAwaitingRespawn && Bot->GetPlayerState<AERPlayerState>()->Deaths==1 && Host->GetPlayerState<AERPlayerState>()->Kills==1,TEXT("player_kills_bot_scores_death"));
        Mode->RequestRespawn(Bot);
        Check(Bot->GetPawn()==B && B->Health->bDead,TEXT("bot_early_respawn_rejected")); Advance(7,3.3); break;
    case 7:
        Check(B != DeadPawn.Get() && !B->Health->bDead && B->Health->Health==100 && !Bot->bAwaitingRespawn && B->Health->IsProtected() && B->Weapon->MagazineAmmo==30 && B->Weapon->ReserveAmmo==90,
            TEXT("bot_requests_respawn_new_life_protected"));
        GS->AlphaScore=50; Advance(8,.5); break;
    case 8:
        Check(GS->GetMatchState()==MatchState::WaitingPostMatch,TEXT("bot_round_finishes")); Advance(9,10.7); break;
    case 9:
        Check(GS->IsMatchInProgress() && !B->Health->bDead && GS->AlphaScore==0 && Bot->GetPlayerState<AERPlayerState>()->Deaths==0 && Mode->GetNumTestBots()==1,TEXT("round_restarts_with_bot"));
        UE_LOG(LogTemp,Display,TEXT("ER_BOT_PROBE_COMPLETE result=%s mode=%s"),bFailed ? TEXT("FAIL") : TEXT("PASS"),bTargetMode ? TEXT("Target") : TEXT("Patrol"));
        FPlatformMisc::RequestExitWithStatus(false,bFailed ? 1 : 0); Advance(10,1000); break;
    }
#endif
}
