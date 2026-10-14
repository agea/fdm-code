#include "ERNetworkProbe.h"
#include "ERCharacter.h"
#include "ERCorpse.h"
#include "EngineUtils.h"
#include "Components/SkeletalMeshComponent.h"
#include "ERMatch.h"
#include "ERPlayerController.h"
#include "ERWeapon.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "HAL/PlatformMisc.h"

AERNetworkProbe::AERNetworkProbe() { bReplicates = true; bAlwaysRelevant = true; PrimaryActorTick.bCanEverTick = true; SetNetUpdateFrequency(30); }
void AERNetworkProbe::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME(AERNetworkProbe, Phase);
}
void AERNetworkProbe::Advance(int32 Next, float Delay) { Phase = Next; Deadline = GetWorld()->GetTimeSeconds()+Delay; ForceNetUpdate(); }
void AERNetworkProbe::Check(bool Condition, const TCHAR* Label)
{
    if (Condition) { UE_LOG(LogTemp, Display, TEXT("ER_TEST_PASS %s"), Label); }
    else { UE_LOG(LogTemp, Error, TEXT("ER_TEST_FAIL %s phase=%d"), Label,Phase); bFailed = true; }
}
void AERNetworkProbe::PlacePlayers(bool AcrossWall)
{
    if (!Host->GetPawn() || !Client->GetPawn()) return;
    const float Y = AcrossWall ? 0 : -1500;
    Host->GetPawn()->TeleportTo(FVector(-1000,Y,90), FRotator::ZeroRotator);
    Client->GetPawn()->TeleportTo(FVector(1000,Y,90), FRotator(0,180,0));
    Host->SetControlRotation(FRotator::ZeroRotator); Host->ClientSetRotation(FRotator::ZeroRotator);
    Client->SetControlRotation(FRotator(0,180,0)); Client->ClientSetRotation(FRotator(0,180,0));
}
void AERNetworkProbe::HostShot()
{
    if (auto* Pawn = Cast<AERCharacter>(Host->GetPawn())) { Pawn->ServerStartFire(); Pawn->ServerStopFire(); }
}
void AERNetworkProbe::ClientStep()
{
    auto* PC = Cast<AERPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(),0));
    if (!PC || !PC->IsLocalController() || !PC->GetPawn() || ClientPhase == Phase) return;
    auto* Pawn = Cast<AERCharacter>(PC->GetPawn()); if (!Pawn || !Pawn->Weapon) return;
    ClientPhase = Phase;
    UE_LOG(LogTemp, Display, TEXT("ER_CLIENT_PHASE %d hp=%.0f ammo=%d"),Phase,Pawn->Health->Health,Pawn->Weapon->MagazineAmmo);
    switch (Phase)
    {
        case 2:
        {
            bool bAppearanceMatches = true;
            bool bAlphaSeen = false, bOmegaSeen = false;
            for (TActorIterator<AERCharacter> It(GetWorld()); It; ++It)
            {
                const auto* PS = It->GetPlayerState<AERPlayerState>();
                if (!PS || PS->Team == EERTeam::None) continue;
                bAlphaSeen |= PS->Team == EERTeam::Alpha;
                bOmegaSeen |= PS->Team == EERTeam::Omega;
                const auto& Appearance = It->GetTeamAppearance();
                bAppearanceMatches &= Appearance.Matches(It->GetMesh());
            }
            UE_LOG(LogTemp, Display, TEXT("ER_TEAM_CLIENT_%s phase=%d"), bAppearanceMatches && bAlphaSeen && bOmegaSeen ? TEXT("PASS") : TEXT("FAIL"), Phase);
            break;
        }
        case 5:
            if (FParse::Param(FCommandLine::Get(),TEXT("ERSkinProbe"))) PC->ServerSetSkins(1,2);
            PC->ServerRequestRespawn(); break;
        case 6: PC->ServerRequestRespawn(); break;
        case 7: PC->SetControlRotation(FRotator(0,180,0)); Pawn->ServerStartFire(); Pawn->ServerStopFire(); break;
        case 8: case 9: Pawn->ServerReload(); break;
        case 10: for(int32 I=0;I<8;++I) { Pawn->ServerStartFire(); Pawn->ServerStopFire(); } break;
        case 11: Pawn->ServerStartFire(); break;
        case 12: Pawn->ServerStopFire(); break;
    }
}
void AERNetworkProbe::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
#if !UE_BUILD_SHIPPING
    if (!HasAuthority()) { ClientStep(); return; }
    auto* Mode = GetWorld()->GetAuthGameMode<AERGameMode>();
    auto* GS = GetWorld()->GetGameState<AERGameState>();
    if (!Mode || !GS) return;
    if (Phase == 0)
    {
        for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        {
            auto* PC = Cast<AERPlayerController>(It->Get()); if (!PC) continue;
            if (PC->IsLocalController()) Host = PC; else Client = PC;
        }
        if (!Host || !Client || !Host->GetPawn() || !Client->GetPawn() || !Mode->IsMatchInProgress()) return;
        PlacePlayers();
        CastChecked<AERCharacter>(Host->GetPawn())->Health->ProtectionEndServerTime = GetWorld()->GetTimeSeconds()+4;
        CastChecked<AERCharacter>(Client->GetPawn())->Health->ProtectionEndServerTime = GetWorld()->GetTimeSeconds()+4;
        Check(Host->GetPlayerState<AERPlayerState>()->Team != Client->GetPlayerState<AERPlayerState>()->Team,TEXT("T03_balanced_teams"));
        Advance(1,.6f); return;
    }
    if (GetWorld()->GetTimeSeconds() < Deadline) return;
    auto* H = Cast<AERCharacter>(Host->GetPawn()); auto* C = Cast<AERCharacter>(Client->GetPawn());
    if (!H || !C || !H->Weapon || !C->Weapon) { Check(false,TEXT("valid_pawns_and_weapons")); FPlatformMisc::RequestExitWithStatus(false,1); return; }
    switch (Phase)
    {
        case 1:
            if (FParse::Param(FCommandLine::Get(),TEXT("ERSkinProbe")))
            {
                Check(Host->GetPlayerState<AERPlayerState>()->ActiveSkin == 3 && Client->GetPlayerState<AERPlayerState>()->ActiveSkin == 4, TEXT("Skin_preferences_replicated"));
                Host->ServerSetSkins(999,-1);
                Check(Host->GetPlayerState<AERPlayerState>()->AlphaSkin == 3, TEXT("Skin_invalid_index_rejected"));
            }
            Check(H->GetTeamAppearance().Matches(H->GetMesh()) && C->GetTeamAppearance().Matches(C->GetMesh()), TEXT("Team_human_mesh_animation_hidden_equipment"));
            HostShot(); Advance(2,.5f); break;
        case 2:
            Check(C->Health->Health==100 && H->Weapon->MagazineAmmo==29 && !H->Health->IsProtected(),TEXT("T14_protection_shot_cancels"));
            C->Health->ProtectionEndServerTime=0; HostShot();
            Check(C->Health->Health==64, TEXT("hitscan_damage_applied_before_shot_returns"));
            { bool Bullet=false; for (TActorIterator<AERProjectile> It(GetWorld()); It; ++It) Bullet |= It->Payload==EERPayload::Bullet;
              Check(!Bullet, TEXT("hitscan_no_physical_bullet_actors")); }
            Advance(3,.5f); break;
        case 3: Check(C->Health->Health==64,TEXT("T04_host_first_hitscan_64")); HostShot(); Advance(4,.5f); break;
        case 4: Check(C->Health->Health==28,TEXT("T04_host_second_hitscan_28")); HostShot(); Advance(5,3.4f); break;
        case 5:
            if (FParse::Param(FCommandLine::Get(),TEXT("ERSkinProbe")))
                Check(Client->GetPlayerState<AERPlayerState>()->OmegaSkin == 2 && Client->GetPlayerState<AERPlayerState>()->ActiveSkin == 4,TEXT("Skin_pending_until_respawn"));
            for (TActorIterator<AERCorpse> It(GetWorld()); It; ++It) FirstCorpse = *It;
            Check(FirstCorpse.IsValid() && FirstCorpse->Body->IsSimulatingPhysics(), TEXT("Death_corpse_ragdoll_created"));
            Check(FirstCorpse.IsValid() && FirstCorpse->GetTeamAppearance().Matches(FirstCorpse->Body, true), TEXT("Team_omega_corpse_matches_live_character"));
            Check(FirstCorpse.IsValid() && FirstCorpse->HasValidRagdollPose(), TEXT("Death_omega_limbs_and_hidden_accessories_during_physics"));
            Check(C->Health->bDead && C->Health->Health==0 && Host->GetPlayerState<AERPlayerState>()->Kills==1,TEXT("T04_death_score_T12_no_auto_respawn"));
            Check(Client->bAwaitingRespawn && Client->GetPlayerState<AERPlayerState>()->Deaths==1,TEXT("T12_early_request_rejected"));
            Advance(6,1.f); break;
        case 6:
            if (FParse::Param(FCommandLine::Get(),TEXT("ERSkinProbe")))
            {
                Check(FirstCorpse.IsValid() && FirstCorpse->SkinIndex == 4,TEXT("Skin_corpse_preserves_old_selection"));
                Check(Client->GetPlayerState<AERPlayerState>()->ActiveSkin == 2,TEXT("Skin_new_selection_on_respawn"));
            }
            Check(FirstCorpse.IsValid(), TEXT("Death_corpse_survives_respawn"));
            Check(C->GetTeamAppearance().Matches(C->GetMesh()), TEXT("Team_appearance_survives_respawn"));
            Check(FirstCorpse.IsValid() && FirstCorpse->Body->GetSocketLocation(TEXT("pelvis")).Z < FirstCorpse->GetActorLocation().Z - 35.f,
                TEXT("Death_corpse_falls_to_ground"));
            Check(!C->Health->bDead && C->Health->Health==100 && C->Weapon->MagazineAmmo==30 && C->Health->IsProtected(),TEXT("T12_requested_respawn_T13_no_shot"));
            PlacePlayers(); H->Health->ProtectionEndServerTime=0; Advance(7,1.f); break;
        case 7:
            Check(H->Health->Health==64 && C->Weapon->MagazineAmmo==29,TEXT("T04_client_rpc_hitscan_to_host"));
            C->Weapon->MagazineAmmo=20; C->Weapon->ReserveAmmo=90; Advance(8,2.1f); break;
        case 8:
            Check(C->Weapon->MagazineAmmo==30 && C->Weapon->ReserveAmmo==80 && !C->Weapon->bReloading,TEXT("T08_partial_reload"));
            C->Weapon->MagazineAmmo=20; C->Weapon->ReserveAmmo=5; Advance(9,2.1f); break;
        case 9:
            Check(C->Weapon->MagazineAmmo==25 && C->Weapon->ReserveAmmo==0,TEXT("T09_low_reserve"));
            H->Health->ProtectionEndServerTime=GetWorld()->GetTimeSeconds()+20;
            C->Weapon->MagazineAmmo=30;
            ClientFeedbackBefore = C->ShotFeedbackCount;
            ClientAimBefore = Client->GetControlRotation();
            Advance(10,1.f); break;
        case 10:
            Check(C->Weapon->MagazineAmmo==29,TEXT("T06_rapid_rpc_presses_one_shot"));
            if (FParse::Param(FCommandLine::Get(), TEXT("ERShotAudit")))
                Check(C->ShotFeedbackCount == ClientFeedbackBefore + 1, TEXT("Shot_rapid_requests_one_feedback"));
            C->Weapon->MagazineAmmo=30; Advance(11,4.2f); break;
        case 11:
            UE_LOG(LogTemp,Display,TEXT("ER_TEST_CLIP ammo=%d host_hp=%.0f"),C->Weapon->MagazineAmmo,H->Health->Health);
            Check(C->Weapon->MagazineAmmo==0 && H->Health->Health==64,TEXT("T07_empty_clip_30_shots"));
            if (FParse::Param(FCommandLine::Get(), TEXT("ERShotAudit")))
            {
                Check(C->ShotFeedbackCount == ClientFeedbackBefore + 31, TEXT("Shot_30_rounds_30_feedback_events"));
                Check(Client->GetControlRotation().Equals(ClientAimBefore, .01f), TEXT("Shot_cosmetic_recoil_preserves_server_aim"));
                C->ServerStartFire(); C->ServerStopFire();
            }
            PlacePlayers(true); C->Health->Health=100; C->Health->ProtectionEndServerTime=0; Advance(12,.7f); break;
        case 12: HostShot(); Advance(13,.5f); break;
        case 13:
            if (FParse::Param(FCommandLine::Get(), TEXT("ERShotAudit")))
                Check(C->ShotFeedbackCount == ClientFeedbackBefore + 31, TEXT("Shot_empty_clip_no_feedback"));
            Check(C->Health->Health==100,TEXT("T05_central_wall_blocks_hitscan"));
            Client->GetPlayerState<AERPlayerState>()->Team=Host->GetPlayerState<AERPlayerState>()->Team;
            PlacePlayers(); H->ServerStartFire(); Advance(14,.7f); break;
        case 14:
            H->ServerStopFire();
            Check(C->Health->bDead && Host->GetPlayerState<AERPlayerState>()->Kills==1 && Host->GetPlayerState<AERPlayerState>()->TeamKills==1 && GS->AlphaScore+GS->OmegaScore==1,TEXT("T11_friendly_fire_no_enemy_score"));
            Client->GetPlayerState<AERPlayerState>()->Team=Host->GetPlayerState<AERPlayerState>()->Team==EERTeam::Alpha ? EERTeam::Omega : EERTeam::Alpha;
            GS->AlphaScore=GS->OmegaScore=0;
            GS->MatchEndServerTime=GetWorld()->GetTimeSeconds()+.1f; Advance(15,.6f); break;
        case 15:
            Check(GS->GetMatchState()==MatchState::WaitingPostMatch && GS->Winner==EERTeam::None,TEXT("T16_time_limit_draw")); Advance(16,10.8f); break;
        case 16:
            Check(GS->IsMatchInProgress() && GS->AlphaScore==0 && GS->OmegaScore==0 && !C->Health->bDead && H->Health->Health==100,TEXT("T16_round_restarts"));
            H->Health->ProtectionEndServerTime=0; H->Weapon->MagazineAmmo=20;
            H->ServerReload(); H->Health->ReceiveDamage(100,nullptr); H->Health->ReceiveDamage(100,nullptr);
            Advance(17,3.4f); break;
        case 17:
            Check(H->Health->bDead && !H->Weapon->bReloading && H->Weapon->MagazineAmmo==20 && Host->GetPlayerState<AERPlayerState>()->Deaths==1,TEXT("T10_reload_cancel_T15_single_death"));
            Mode->RequestRespawn(Host); Advance(18,1.7f); break;
        case 18:
            for (TActorIterator<AERCorpse> It(GetWorld()); It; ++It)
                Check(It->HasValidRagdollPose(), TEXT("Death_alpha_limbs_and_hidden_accessories_after_settle"));
            Check(!H->Health->bDead && H->Weapon->MagazineAmmo==30 && H->Weapon->ReserveAmmo==90,TEXT("T10_new_life_ammo_unchanged"));
            GS->AlphaScore=50; Advance(19,.6f); break;
        case 19:
            Check(GS->GetMatchState()==MatchState::WaitingPostMatch && GS->Winner==EERTeam::Alpha,TEXT("T16_score_limit_winner"));
            UE_LOG(LogTemp,Display,TEXT("ER_NETWORK_PROBE_COMPLETE result=%s"),bFailed ? TEXT("FAIL") : TEXT("PASS"));
            FPlatformMisc::RequestExitWithStatus(false,bFailed ? 1 : 0); Advance(20,1000); break;
    }
#endif
}
