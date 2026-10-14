#include "ERMatch.h"
#include "ERCharacter.h"
#include "ERCorpse.h"
#include "ERTeamGallery.h"
#include "ERSmokeCloud.h"
#include "ERPlayerController.h"
#include "ERWeapon.h"
#include "ERNetworkProbe.h"
#include "ERBotController.h"
#include "ERBotProbe.h"
#include "ERMovementProbe.h"
#include "ERAccuracyProbe.h"
#include "ERArsenalProbe.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "GameFramework/GameSession.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

void AERPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AERPlayerState, Team); DOREPLIFETIME(AERPlayerState, Kills);
    DOREPLIFETIME(AERPlayerState, Deaths); DOREPLIFETIME(AERPlayerState, TeamKills);
    DOREPLIFETIME(AERPlayerState, PrimaryWeapon);
    DOREPLIFETIME(AERPlayerState, AlphaSkin); DOREPLIFETIME(AERPlayerState, OmegaSkin); DOREPLIFETIME(AERPlayerState, ActiveSkin);
}
void AERGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AERGameState, AlphaScore); DOREPLIFETIME(AERGameState, OmegaScore);
    DOREPLIFETIME(AERGameState, MatchEndServerTime); DOREPLIFETIME(AERGameState, RestartServerTime);
    DOREPLIFETIME(AERGameState, Winner);
}
AERGameMode::AERGameMode()
{
    DefaultPawnClass = AERCharacter::StaticClass();
    PlayerControllerClass = AERPlayerController::StaticClass();
    PlayerStateClass = AERPlayerState::StaticClass();
    GameStateClass = AERGameState::StaticClass();
    HUDClass = AERHUD::StaticClass();
    bDelayedStart = true;
    bUseSeamlessTravel = false;
}
void AERGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    Super::InitGame(MapName, Options, ErrorMessage);
    bStationaryBots = UGameplayStatics::ParseOption(Options, TEXT("ERBotMode")).Equals(TEXT("Target"), ESearchCase::IgnoreCase);
    DesiredBots = FMath::Clamp(UGameplayStatics::GetIntOption(Options, TEXT("ERBots"), 0), 0, bStationaryBots ? 2 * ERSkins::Count : 3);
}
void AERGameMode::RemoveBot()
{
    if (TestBots.IsEmpty()) return;
    AERBotController* Bot = TestBots.Pop();
    if (IsValid(Bot))
    {
        UE_LOG(LogTemp,Display,TEXT("ER_BOT_REMOVED name=%s"),*Bot->GetPlayerState<AERPlayerState>()->GetPlayerName());
        if (APawn* Pawn = Bot->GetPawn()) { Bot->UnPossess(); Pawn->Destroy(); }
        if (Bot->TrainingStart) Bot->TrainingStart->Destroy();
        Bot->Destroy();
    }
    NumBots = TestBots.Num();
}
void AERGameMode::PrunePendingSlots()
{
    const double Now = GetWorld()->GetTimeSeconds();
    PendingHumanSlots.RemoveAll([Now](double ExpiresAt) { return Now >= ExpiresAt; });
}
void AERGameMode::FillBots()
{
    if (UGameplayStatics::GetCurrentLevelName(GetWorld()).Contains(TEXT("L_MainMenu")) || GetNumPlayers() == 0) return;
    PrunePendingSlots();
    const int32 Count = bStationaryBots ? DesiredBots : FMath::Min(DesiredBots, FMath::Max(0, 4-GetNumPlayers()-PendingHumanSlots.Num()));
    while (TestBots.Num() > Count) RemoveBot();
    while (TestBots.Num() < Count)
    {
        auto* Bot = GetWorld()->SpawnActor<AERBotController>();
        auto* State = Bot ? Bot->GetPlayerState<AERPlayerState>() : nullptr;
        if (!State) { if (Bot) Bot->Destroy(); break; }
        Bot->bTrainingTarget = bStationaryBots;
        State->SetIsABot(true);
        State->AlphaSkin = State->OmegaSkin = (TestBots.Num() + 1) % ERSkins::Count;
        State->SetPlayerName(FString::Printf(TEXT("%s %d"), bStationaryBots ? TEXT("Bersaglio") : TEXT("Bot"), TestBots.Num()+1));
        if (bStationaryBots && DesiredBots >= 2)
        {
            int32 AlphaTargets = 0, OmegaTargets = 0;
            for (const auto& Target : TestBots) if (auto* PS = Target->GetPlayerState<AERPlayerState>())
            {
                AlphaTargets += PS->Team == EERTeam::Alpha;
                OmegaTargets += PS->Team == EERTeam::Omega;
            }
            State->Team = OmegaTargets <= AlphaTargets ? EERTeam::Omega : EERTeam::Alpha;
        }
        else AssignTeam(State);
        if (bStationaryBots && DesiredBots == 2 * ERSkins::Count)
        {
            const int32 Skin = TestBots.Num() / 2;
            State->AlphaSkin = State->OmegaSkin = Skin;
            State->SetPlayerName(FString::Printf(TEXT("%s %d"), State->Team == EERTeam::Alpha ? TEXT("Alpha") : TEXT("Omega"), Skin + 1));
            // Keep each variant's place on respawn. The two clear spawn strips
            // provide room for five capsules without modifying the depot map.
            const FVector Position(State->Team == EERTeam::Alpha ? -2300.f : 2300.f, (Skin - 2) * 180.f, 100.f);
            const FRotator Facing(0, State->Team == EERTeam::Alpha ? 0.f : 180.f, 0);
            Bot->TrainingStart = GetWorld()->SpawnActor<AERTeamPlayerStart>(Position, Facing);
            if (Bot->TrainingStart) Bot->TrainingStart->Team = State->Team;
        }
        TestBots.Add(Bot); NumBots = TestBots.Num();
        RestartPlayer(Bot);
        UE_LOG(LogTemp,Display,TEXT("ER_BOT_JOINED name=%s team=%s mode=%s skin=%d pawn=%d"),*State->GetPlayerName(), State->Team==EERTeam::Alpha ? TEXT("Alpha") : TEXT("Omega"),bStationaryBots ? TEXT("Target") : TEXT("Patrol"), State->ActiveSkin, Bot->GetPawn() != nullptr);
    }
}
void AERGameMode::BeginPlay()
{
    Super::BeginPlay();
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("ERPostureGallery")) || FParse::Param(FCommandLine::Get(), TEXT("ERTeamGallery")) || FParse::Param(FCommandLine::Get(), TEXT("ERCorpseGallery")) || FParse::Param(FCommandLine::Get(), TEXT("ERSkinGallery")) || FParse::Param(FCommandLine::Get(), TEXT("ERFPSForearmsGallery"))) GetWorld()->SpawnActor<AERTeamGallery>();
#endif
    if (GameSession) GameSession->MaxPlayers = 4;
    GetWorldTimerManager().SetTimer(MatchTimer, this, &AERGameMode::CheckMatch, 0.2f, true);
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("ERNetworkProbe"))) GetWorld()->SpawnActor<AERNetworkProbe>();
    if (FParse::Param(FCommandLine::Get(), TEXT("ERMovementProbe"))) GetWorld()->SpawnActor<AERMovementProbe>();
    if (FParse::Param(FCommandLine::Get(), TEXT("ERBotProbe")) || FParse::Param(FCommandLine::Get(), TEXT("ERTargetSkinsProbe"))) GetWorld()->SpawnActor<AERBotProbe>();
    if (FParse::Param(FCommandLine::Get(), TEXT("ERAccuracyProbe"))) GetWorld()->SpawnActor<AERAccuracyProbe>();
    if (FParse::Param(FCommandLine::Get(), TEXT("ERArsenalProbe")) || FParse::Param(FCommandLine::Get(), TEXT("ERArsenalGallery")) || FParse::Param(FCommandLine::Get(), TEXT("ERHitscanGallery")) || FParse::Param(FCommandLine::Get(), TEXT("ERGrenadeGallery")) || FParse::Param(FCommandLine::Get(), TEXT("ERSmokeGallery")))
        GetWorld()->SpawnActor<AERArsenalProbe>();
#endif
}
void AERGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
    PrunePendingSlots();
    const bool bFull = GetNumPlayers()+PendingHumanSlots.Num() >= 4;
    // PreLogin and spawning the human can happen in different frames. Keep the
    // freed slot reserved so CheckMatch cannot refill it with another bot.
    if (!bFull && !bStationaryBots)
        while (!TestBots.IsEmpty() && GetNumPlayers()+TestBots.Num()+PendingHumanSlots.Num() >= 4) RemoveBot();
    Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
    if (bFull) ErrorMessage = TEXT("Server pieno: massimo 4 giocatori.");
    if (ErrorMessage.IsEmpty()) PendingHumanSlots.Add(GetWorld()->GetTimeSeconds()+30);
}
void AERGameMode::Logout(AController* Player)
{
    if (auto* Character = Cast<AERCharacter>(Player->GetPawn())) if (Character->Weapon) Character->Weapon->Cancel();
    Super::Logout(Player);
    UE_LOG(LogTemp,Display,TEXT("ER_PLAYER_LEFT players=%d"),GetNumPlayers());
}
FString AERGameMode::SanitizeName(const FString& Name)
{
    FString Result;
    for (TCHAR C : Name.Left(24)) if (FChar::IsAlnum(C) || C == TCHAR(' ') || C == TCHAR('_') || C == TCHAR('-')) Result.AppendChar(C);
    Result.TrimStartAndEndInline();
    return Result.IsEmpty() ? TEXT("Recluta") : Result;
}
FString AERGameMode::InitNewPlayer(APlayerController* Player, const FUniqueNetIdRepl& Id, const FString& Options, const FString& Portal)
{
    FString Error = Super::InitNewPlayer(Player, Id, Options, Portal);
    if (Error.IsEmpty())
    {
        auto* State = Player->GetPlayerState<AERPlayerState>();
        if (State)
        {
            State->PrimaryWeapon = FMath::Clamp(UGameplayStatics::GetIntOption(Options, TEXT("Primary"), 0), 0, 6);
            State->AlphaSkin = ERSkins::ValidIndex(UGameplayStatics::GetIntOption(Options, TEXT("AlphaSkin"), 0));
            State->OmegaSkin = ERSkins::ValidIndex(UGameplayStatics::GetIntOption(Options, TEXT("OmegaSkin"), 0));
            State->SetPlayerName(SanitizeName(UGameplayStatics::ParseOption(Options, TEXT("Name")))); AssignTeam(State);
        }
    }
    return Error;
}
void AERGameMode::AssignTeam(AERPlayerState* Player)
{
    if (!Player || Player->Team != EERTeam::None) return;
    int32 Alpha = 0, Omega = 0;
    for (APlayerState* Item : GameState->PlayerArray)
    {
        auto* State = Cast<AERPlayerState>(Item);
        if (!State || State == Player || State->IsInactive()) continue;
        Alpha += State->Team == EERTeam::Alpha; Omega += State->Team == EERTeam::Omega;
    }
    if (Alpha == Omega) { Player->Team = NextTieTeam; NextTieTeam = NextTieTeam == EERTeam::Alpha ? EERTeam::Omega : EERTeam::Alpha; }
    else Player->Team = Alpha < Omega ? EERTeam::Alpha : EERTeam::Omega;
}
void AERGameMode::HandleStartingNewPlayer_Implementation(APlayerController* Player)
{
    if (!Player->IsLocalController() && !PendingHumanSlots.IsEmpty()) PendingHumanSlots.RemoveAt(0);
    AssignTeam(Player->GetPlayerState<AERPlayerState>());
    if (UGameplayStatics::GetCurrentLevelName(GetWorld()).Contains(TEXT("L_MainMenu"))) return;
    const auto* PS = Player->GetPlayerState<AERPlayerState>();
    UE_LOG(LogTemp,Display,TEXT("ER_PLAYER_JOINED players=%d team=%s"),GetNumPlayers(),PS && PS->Team==EERTeam::Alpha ? TEXT("Alpha") : TEXT("Omega"));
    if (!Player->GetPawn() && GetMatchState() != MatchState::WaitingPostMatch) RestartPlayer(Player);
    FillBots();
    if (ReadyToStartMatch()) StartMatch();
}
bool AERGameMode::ReadyToStartMatch_Implementation()
{
    if (GetMatchState() != MatchState::WaitingToStart) return false;
    bool Alpha = false, Omega = false;
    for (APlayerState* Item : GameState->PlayerArray)
    {
        const auto* State = Cast<AERPlayerState>(Item);
        if (!State || State->IsInactive()) continue;
        Alpha |= State->Team == EERTeam::Alpha; Omega |= State->Team == EERTeam::Omega;
    }
    return Alpha && Omega;
}
AActor* AERGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    auto* State = Player ? Player->GetPlayerState<AERPlayerState>() : nullptr;
    if (!State) return nullptr;
    const auto* Capsule = DefaultPawnClass->GetDefaultObject<ACharacter>()->GetCapsuleComponent();
    const auto* Bot = Cast<AERBotController>(Player);
    if (Bot && Bot->TrainingStart)
    {
        FCollisionQueryParams Params(SCENE_QUERY_STAT(ERTrainingSpawn), false);
        if (Player->GetPawn()) Params.AddIgnoredActor(Player->GetPawn());
        return GetWorld()->OverlapBlockingTestByChannel(Bot->TrainingStart->GetActorLocation(), FQuat::Identity, ECC_Pawn,
            FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight()), Params) ? nullptr : Bot->TrainingStart.Get();
    }
    TArray<AERTeamPlayerStart*> Choices;
    for (TActorIterator<AERTeamPlayerStart> It(GetWorld()); It; ++It)
    {
        if (It->Team != State->Team) continue;
        // Exhibition starts belong only to their target; humans keep map starts.
        bool bTrainingStart = false;
        for (const auto& Target : TestBots) bTrainingStart |= Target->TrainingStart == *It;
        if (bTrainingStart) continue;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(ERSpawn), false);
        if (Player->GetPawn()) Params.AddIgnoredActor(Player->GetPawn());
        if (!GetWorld()->OverlapBlockingTestByChannel(It->GetActorLocation(), FQuat::Identity, ECC_Pawn,
            FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight()), Params)) Choices.Add(*It);
    }
    return Choices.IsEmpty() ? nullptr : Choices[FMath::RandRange(0, Choices.Num()-1)];
}
void AERGameMode::RestartPlayer(AController* Player)
{
    if (!Player || Player->IsPendingKillPending() || Player->GetPawn()) return;
    // The engine's generic fallback may use an occupied cached start or world origin.
    // A team match must wait for an actually clear start instead.
    if (AActor* Start = ChoosePlayerStart(Player))
    {
        if (auto* PS = Player->GetPlayerState<AERPlayerState>())
        { PS->ActiveSkin = ERSkins::ValidIndex(PS->Team == EERTeam::Alpha ? PS->AlphaSkin : PS->OmegaSkin); PS->ForceNetUpdate(); }
        RestartPlayerAtPlayerStart(Player, Start);
    }
    if (auto* Character = Cast<AERCharacter>(Player->GetPawn()))
        Character->InitializeLoadout(Player->GetPlayerState<AERPlayerState>()->PrimaryWeapon);
}
UClass* AERGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
    const auto* Bot = Cast<AERBotController>(InController);
    if (Bot && TrainingTargetPawnClass)
    {
        UE_LOG(LogTemp, Display, TEXT("ER_BOT_PAWN_CLASS mode=%s class=%s"), bStationaryBots ? TEXT("Target") : TEXT("Patrol"), *TrainingTargetPawnClass->GetPathName());
        return TrainingTargetPawnClass.Get();
    }
    return Super::GetDefaultPawnClassForController_Implementation(InController);
}
void AERGameMode::HandleMatchHasStarted()
{
    Super::HandleMatchHasStarted();
    auto* State = GetGameState<AERGameState>();
    State->AlphaScore = State->OmegaScore = 0;
    State->Winner = EERTeam::None;
    State->MatchEndServerTime = State->GetServerWorldTimeSeconds() + MatchDuration;
    for (APlayerState* Item : State->PlayerArray) if (auto* PS = Cast<AERPlayerState>(Item)) PS->Kills = PS->Deaths = PS->TeamKills = 0;
    UE_LOG(LogTemp, Display, TEXT("ER_MATCH_STARTED players=%d duration=%.1f"), GetNumPlayers(), MatchDuration);
}
void AERGameMode::CheckMatch()
{
    if (UGameplayStatics::GetCurrentLevelName(GetWorld()).Contains(TEXT("L_MainMenu"))) return;
    auto* State = GetGameState<AERGameState>();
    if (!State) return;
    FillBots();
    if (GetMatchState() == MatchState::WaitingToStart && ReadyToStartMatch()) StartMatch();
    if (IsMatchInProgress() && (State->AlphaScore >= ScoreLimit || State->OmegaScore >= ScoreLimit || State->GetServerWorldTimeSeconds() >= State->MatchEndServerTime)) FinishRound();
    if (GetMatchState() == MatchState::WaitingPostMatch && State->GetServerWorldTimeSeconds() >= State->RestartServerTime) ResetRound();
    // Retry only initial spawns. A death never automatically requests a respawn.
    if (GetMatchState() == MatchState::WaitingToStart || IsMatchInProgress())
        for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
            if (auto* PC = Cast<AERPlayerController>(It->Get()); PC && !PC->GetPawn() && !PC->bAwaitingRespawn) RestartPlayer(PC);
    if (GetMatchState() == MatchState::WaitingToStart || IsMatchInProgress())
        for (AERBotController* Bot : TestBots) if (IsValid(Bot) && !Bot->GetPawn() && !Bot->bAwaitingRespawn) RestartPlayer(Bot);
}
void AERGameMode::FinishRound()
{
    auto* State = GetGameState<AERGameState>();
    State->Winner = State->AlphaScore == State->OmegaScore ? EERTeam::None : (State->AlphaScore > State->OmegaScore ? EERTeam::Alpha : EERTeam::Omega);
    State->RestartServerTime = State->GetServerWorldTimeSeconds() + 10;
    EndMatch();
    for (TActorIterator<AERWeapon> It(GetWorld()); It; ++It) It->Cancel();
    for (TActorIterator<AERProjectile> It(GetWorld()); It; ++It) It->Destroy();
}
void AERGameMode::ResetRound()
{
    for (TActorIterator<AERCorpse> It(GetWorld()); It; ++It) It->Destroy();
    for (TActorIterator<AERSmokeCloud> It(GetWorld()); It; ++It) It->Destroy();
    SetMatchState(MatchState::WaitingToStart);
    for (TActorIterator<AController> It(GetWorld()); It; ++It)
    {
        AController* Controller = *It;
        auto* PC = Cast<AERPlayerController>(Controller);
        auto* Bot = Cast<AERBotController>(Controller);
        if (!PC && !Bot) continue;
        if (APawn* Old = Controller->GetPawn()) { Controller->UnPossess(); Old->Destroy(); }
        if (PC) { PC->bAwaitingRespawn = false; PC->RespawnAllowedAt = 0; }
        if (Bot) Bot->ResetLife();
        if (auto* PS = Controller->GetPlayerState<AERPlayerState>()) PS->Kills = PS->Deaths = PS->TeamKills = 0;
        RestartPlayer(Controller);
    }
    auto* State = GetGameState<AERGameState>();
    State->AlphaScore = State->OmegaScore = 0; State->MatchEndServerTime = 0; State->RestartServerTime = 0; State->Winner = EERTeam::None;
    if (ReadyToStartMatch()) StartMatch();
}
void AERGameMode::RecordDeath(AERCharacter* Victim, AController* Killer)
{
    auto* VictimPS = Victim->GetPlayerState<AERPlayerState>();
    auto* KillerPS = Killer ? Killer->GetPlayerState<AERPlayerState>() : nullptr;
    if (VictimPS) ++VictimPS->Deaths;
    if (VictimPS && KillerPS && KillerPS != VictimPS)
    {
        if (VictimPS->Team == KillerPS->Team) ++KillerPS->TeamKills;
        else { ++KillerPS->Kills; auto* GS = GetGameState<AERGameState>(); if (KillerPS->Team == EERTeam::Alpha) ++GS->AlphaScore; else ++GS->OmegaScore; }
    }
    if (auto* PC = Cast<AERPlayerController>(Victim->Controller))
    {
        PC->bAwaitingRespawn = true;
        PC->RespawnAllowedAt = GetWorld()->GetTimeSeconds() + 3;
    }
    if (auto* Bot = Cast<AERBotController>(Victim->Controller))
    {
        Bot->bAwaitingRespawn = true;
        Bot->RespawnAllowedAt = GetWorld()->GetTimeSeconds()+3;
    }
    CheckMatch();
}
void AERGameMode::RequestRespawn(AController* Controller)
{
    auto* PC = Cast<AERPlayerController>(Controller);
    auto* Bot = Cast<AERBotController>(Controller);
    if (!IsMatchInProgress() || (!PC && !Bot)) return;
    const bool Waiting = PC ? PC->bAwaitingRespawn : Bot->bAwaitingRespawn;
    const double AllowedAt = PC ? PC->RespawnAllowedAt : Bot->RespawnAllowedAt;
    if (!Waiting || GetWorld()->GetTimeSeconds() < AllowedAt) return;
    auto* Old = Cast<AERCharacter>(Controller->GetPawn());
    if (Old && !Old->Health->bDead) return;
    if (!ChoosePlayerStart(Controller)) return;
    if (Old) { Controller->UnPossess(); Old->Destroy(); }
    RestartPlayer(Controller);
    if (Controller->GetPawn())
    {
        if (PC) { PC->bAwaitingRespawn = false; PC->RespawnAllowedAt = 0; }
        if (Bot) Bot->ResetLife();
    }
}
