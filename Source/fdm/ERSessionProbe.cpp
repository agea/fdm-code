#include "ERSessionProbe.h"
#include "ERMatch.h"
#include "ERCharacter.h"
#include "ERWeapon.h"
#include "ERBotController.h"
#include "ERPlayerController.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

AERSessionProbe::AERSessionProbe() { PrimaryActorTick.bCanEverTick=true; }
void AERSessionProbe::Check(bool Value,const TCHAR* Label)
{
    Failed |= !Value;
    UE_LOG(LogTemp,Display,TEXT("ER_SESSION_%s %s"),Value ? TEXT("PASS") : TEXT("FAIL"),Label);
}
void AERSessionProbe::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    const double Now=GetWorld()->GetTimeSeconds();
    auto* Mode=GetWorld()->GetAuthGameMode<AERGameMode>();
    if (!Mode) return;
    AERCharacter* Human=nullptr;
    for (FConstPlayerControllerIterator It=GetWorld()->GetPlayerControllerIterator();It;++It)
        if (It->Get()->IsLocalController()) Human=Cast<AERCharacter>(It->Get()->GetPawn());
    if (!Human || !Human->Weapon) return;
    const bool Lab=GetWorld()->GetMapName().Contains(TEXT("Lab"));
    if (Phase==0 && Now>2)
    {
        Check(Mode->IsMatchInProgress() && Mode->GetNumPlayers()==1,TEXT("solo_match_started"));
        Check(Human->GetPlayerState<AERPlayerState>()->Team==EERTeam::Omega,TEXT("selected_team_omega"));
        int32 Alpha=0,Omega=0;
        for (TActorIterator<AERBotController> It(GetWorld());It;++It)
        {
            const auto* PS=It->GetPlayerState<AERPlayerState>();
            Alpha+=PS->Team==EERTeam::Alpha; Omega+=PS->Team==EERTeam::Omega;
            Check(It->GetPawn()!=nullptr,TEXT("bot_has_clear_spawn"));
            if (It->GetPawn()) Positions.Add(*It,It->GetPawn()->GetActorLocation());
        }
        Check(Alpha==2 && Omega==1,TEXT("per_team_bot_counts"));
        Phase=1; Deadline=Now+4;
        return;
    }
    if (Phase==1 && Now>=Deadline)
    {
        for (const auto& Pair:Positions) if (Pair.Key.IsValid() && Pair.Key->GetPawn())
        {
            const float Distance=FVector::Dist2D(Pair.Value,Pair.Key->GetPawn()->GetActorLocation());
            UE_LOG(LogTemp,Display,TEXT("ER_SESSION_BOT displacement=%.1f"),Distance);
            if (Pair.Key->bTrainingTarget) Check(Distance<5,TEXT("stationary_bot"));
            if (Pair.Key->bMovementOnly)
            {
                Check(Distance>20,TEXT("movement_bot_moves"));
                const auto* C=Cast<AERCharacter>(Pair.Key->GetPawn());
                Check(C && C->Weapon->MagazineAmmo==C->Weapon->Data->MagazineSize,TEXT("movement_bot_never_fires"));
            }
        }
        if (!Lab) { Phase=3; Deadline=Now+(GetNetMode()==NM_ListenServer ? 60 : 1); return; }
        Phase=2;
    }
    if (Phase==2)
    {
        auto* W=Human->Weapon.Get();
        if (W->bReloading) return;
        if (Reloads>0) Check(W->MagazineAmmo==W->Data->MagazineSize,TEXT("lab_reload_from_empty_reserve"));
        if (Reloads==3) { Phase=3; Deadline=Now+(GetNetMode()==NM_ListenServer ? 60 : 1); return; }
        W->MagazineAmmo=0; W->ReserveAmmo=0; W->Reload();
        Check(W->bReloading,TEXT("lab_reload_started"));
        ++Reloads;
        return;
    }
    if (Phase==3 && (Now>=Deadline || (GetNetMode()==NM_ListenServer && Mode->GetNumPlayers()==2)))
    {
        if (GetNetMode()==NM_ListenServer)
        {
            Check(Mode->GetNumPlayers()==2,TEXT("remote_joined"));
            bool ClientTeam=false;
            for (FConstPlayerControllerIterator It=GetWorld()->GetPlayerControllerIterator();It;++It)
                if (!It->Get()->IsLocalController()) ClientTeam=It->Get()->GetPlayerState<AERPlayerState>()->Team==EERTeam::Alpha;
            Check(ClientTeam,TEXT("remote_selected_team_alpha"));
        }
        UE_LOG(LogTemp,Display,TEXT("ER_SESSION_COMPLETE result=%s"),Failed ? TEXT("FAIL") : TEXT("PASS"));
        Phase=4; FPlatformMisc::RequestExitWithStatus(false,Failed ? 1 : 0);
    }
}
