#include "ERBotLoadoutProbe.h"
#include "ERLoadout.h"
#include "ERBotController.h"
#include "ERCharacter.h"
#include "ERMatch.h"
#include "ERWeapon.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "HAL/PlatformMisc.h"

AERBotLoadoutProbe::AERBotLoadoutProbe()
{
    bReplicates = true; bAlwaysRelevant = true; PrimaryActorTick.bCanEverTick = true;
}
void AERBotLoadoutProbe::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME(AERBotLoadoutProbe, Phase);
}
void AERBotLoadoutProbe::ServerObserved_Implementation(int32 ObservedPhase)
{
    if (ObservedPhase == Phase) Acknowledged = Phase;
}
void AERBotLoadoutProbe::Advance(int32 Next, float Wait)
{
    Phase = Next; Deadline = GetWorld()->GetTimeSeconds()+Wait; ForceNetUpdate();
}
bool AERBotLoadoutProbe::Observe(bool FreshLife)
{
    int32 Count = 0, Mask = 0;
    bool Valid = true;
    TMap<TWeakObjectPtr<AERPlayerState>,TWeakObjectPtr<AERCharacter>> Current;
    for (TActorIterator<AERCharacter> It(GetWorld()); It; ++It)
    {
        auto* PS = It->GetPlayerState<AERPlayerState>();
        if (!PS || !PS->IsABot() || It->Health->bDead) continue;
        ++Count;
        auto* Gun = It->Weapon.Get();
        const int32 Slot = It->Inventory.IndexOfByKey(Gun);
        const auto Kit=ERLoadouts::Resolve(It->ActivePreset,It->ActiveVariant,PS->Team==EERTeam::Omega,*It);
        const int32 Preset=ERLoadouts::Presets().IndexOfByKey(It->ActivePreset),Variant=ERLoadouts::Variants().IndexOfByKey(It->ActiveVariant);
        Valid &= It->bPresetInventory && It->Inventory.Num()==Kit.Items.Num() && Gun && Gun->Data==Kit.Items[0].Data && Slot==0 && PS->ActiveSkin==Kit.Skin && Preset>=0 && Variant>=0;
        if(Preset>=0 && Variant>=0)Mask|=1<<(Preset*3+Variant);
        for(int32 I=0;I<Kit.Items.Num() && I<It->Inventory.Num();++I)
        {
            const auto* Item=It->Inventory[I].Get();Valid &= Item && Item->Data==Kit.Items[I].Data;
            if(HasAuthority())Valid &= Item && Item->MagazineAmmo==Kit.Items[I].Loaded && Item->ReserveAmmo==Kit.Items[I].Reserve;
        }
        if (FreshLife) Valid &= PreviousPawns.Contains(PS) && PreviousPawns.FindRef(PS) != *It;
        if (Phase == 3) Valid &= PS->Deaths == 0;
        Current.Add(PS,*It);
    }
    Valid &= Count == 6;
    if (Valid)
    {
        PreviousPawns = MoveTemp(Current);
        UE_LOG(LogTemp,Display,TEXT("ER_BOT_LOADOUT_PASS side=%s phase=%d count=%d mask=%d"),HasAuthority()?TEXT("host"):TEXT("client"),Phase,Count,Mask);
    }
    return Valid;
}
void AERBotLoadoutProbe::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
#if !UE_BUILD_SHIPPING
    const double Now = GetWorld()->GetTimeSeconds();
    if (!HasAuthority())
    {
        if ((Phase == 1 || Phase == 2 || Phase == 3) && ClientPhase != Phase && Observe(Phase > 1))
        { ClientPhase = Phase; ServerObserved(Phase); }
        return;
    }
    auto* Mode = GetWorld()->GetAuthGameMode<AERGameMode>();
    auto* GS = GetWorld()->GetGameState<AERGameState>();
    if (!Mode || !GS) return;
    // Freeze AI decisions for this asset-selection fixture; mode flags remain intact.
    for (TActorIterator<AERBotController> It(GetWorld()); It; ++It)
    {
        It->SetActorTickEnabled(false);
        if (auto* C = Cast<AERCharacter>(It->GetPawn())) { C->GetCharacterMovement()->StopMovementImmediately(); C->ServerStopFire(); }
    }
    if (Phase == 0)
    {
        APlayerController* Remote = nullptr;
        for (FConstPlayerControllerIterator It=GetWorld()->GetPlayerControllerIterator();It;++It)
            if (!It->Get()->IsLocalController()) Remote = It->Get();
        if (!GS->IsMatchInProgress() || Mode->GetNumTestBots() != 6 || !Remote || !Remote->GetPawn()) return;
        SetOwner(Remote); Phase = 1;
        if (!Observe(false)) Failed = true;
        Advance(1,30); return;
    }
    if ((Phase == 1 || Phase == 2 || Phase == 3) && Acknowledged != Phase)
    {
        if (Now < Deadline) return;
        UE_LOG(LogTemp,Error,TEXT("ER_BOT_LOADOUT_FAIL remote_timeout phase=%d"),Phase);
        FPlatformMisc::RequestExitWithStatus(false,1); return;
    }
    if (Phase == 1)
    {
        for (TActorIterator<AERBotController> It(GetWorld());It;++It)
            if (auto* C=Cast<AERCharacter>(It->GetPawn())) { C->Health->ProtectionEndServerTime=0; C->Health->ReceiveDamage(1000,nullptr); }
        Advance(10,3.3f); return;
    }
    if (Phase == 10 && Now >= Deadline)
    {
        for (TActorIterator<AERBotController> It(GetWorld());It;++It) Mode->RequestRespawn(*It);
        Phase = 2;
        if (!Observe(true)) Failed = true;
        Advance(2,30); return;
    }
    if (Phase == 2) { GS->AlphaScore=Mode->ScoreLimit; Advance(20,11.5f); return; }
    if (Phase == 20 && Now >= Deadline)
    {
        Phase = 3;
        if (!GS->IsMatchInProgress() || !Observe(true)) Failed = true;
        Advance(3,30); return;
    }
    if (Phase == 3)
    {
        UE_LOG(LogTemp,Display,TEXT("ER_BOT_LOADOUT_COMPLETE result=%s"),Failed?TEXT("FAIL"):TEXT("PASS"));
        FPlatformMisc::RequestExitWithStatus(false,Failed?1:0); Advance(99,1000);
    }
#endif
}
