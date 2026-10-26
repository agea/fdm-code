#include "ERClaymore.h"
#include "ERMatch.h"
#include "ERPlayerController.h"
#include "ERMapCatalog.h"
#include "ERLobbyRules.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

void AERPlayerState::CopyProperties(APlayerState* Target)
{
    Super::CopyProperties(Target);
    if (auto* PS = Cast<AERPlayerState>(Target))
    {
        PS->LoadoutPreset=LoadoutPreset; PS->LoadoutVariant=LoadoutVariant;
        PS->Team = Team; PS->PendingTeam = PendingTeam; PS->PrimaryWeapon = PrimaryWeapon;
        PS->AlphaSkin = AlphaSkin; PS->OmegaSkin = OmegaSkin;
        PS->SpareMagazines = SpareMagazines; PS->HECount = HECount;
        PS->SmokeCount = SmokeCount; PS->StunCount = StunCount;
    }
}
void AERGameMode::CheckLobby()
{
    if (!bLobby || bLobbyTravelPending) return;
    auto* State = GetGameState<AERGameState>();
    if (!State) return;
    int32 Humans = 0, Ready = 0;
    TArray<int32> Votes;
    for (auto Item : State->PlayerArray)
    {
        auto* PS = Cast<AERPlayerState>(Item);
        if (!PS || PS->IsABot() || PS->IsInactive() || PS->IsOnlyASpectator()) continue;
        ++Humans; Ready += PS->bLobbyReady; Votes.Add(PS->MapVote);
    }
    if (!ERLobby::HasQuorum(Humans, Ready)) return;
    const auto Leaders = ERMaps::LobbyLeaders(Votes);
    if (Leaders.IsEmpty()) return;
    const int32 Map = Leaders[FMath::RandHelper(Leaders.Num())];
    bLobbyTravelPending = true;
    State->bLobbyStarting = true;
    State->ForceNetUpdate();
    UE_LOG(LogTemp, Display, TEXT("ER_LOBBY_START humans=%d ready=%d map=%d"), Humans, Ready, Map);
    // Give all clients a frame to see the transition status before seamless travel.
    FTimerHandle TravelTimer;
    GetWorldTimerManager().SetTimer(TravelTimer, FTimerDelegate::CreateWeakLambda(this, [this, Map]()
    {
        const auto& Entry = ERMaps::At(Map);
        FString URL = FString(Entry.Package) + FString::Printf(TEXT("?listen?ERAlphaBots=%d?EROmegaBots=%d?ERBotMode=Patrol?ERMaxPlayers=%d?MaxPlayers=%d"), AlphaBotCount, OmegaBotCount, MaxHumanPlayers, MaxHumanPlayers);
        if (GetGameState<AERGameState>()->bOnline) URL += TEXT("?EROnline=1");
        if (Entry.Procedural) URL += FString::Printf(TEXT("?MapSeed=%d"), FMath::Rand());
        if (!GetWorld()->ServerTravel(URL, true))
        {
            bLobbyTravelPending = false;
            GetGameState<AERGameState>()->bLobbyStarting = false;
            for (auto Item : GameState->PlayerArray) if (auto* PS = Cast<AERPlayerState>(Item)) PS->bLobbyReady = false;
            UE_LOG(LogTemp, Error, TEXT("ER_LOBBY_TRAVEL_FAILED"));
        }
    }), 1.f, false);
}
bool AERPlayerController::IsInLobby() const
{
    const auto* State = GetWorld()->GetGameState<AERGameState>();
    return IsMenuLevel() && State && State->bLobby;
}
void AERPlayerController::ServerLobbyTeam_Implementation(int32 Team)
{
    const auto* Mode = GetWorld()->GetAuthGameMode<AERGameMode>();
    const auto* State = GetWorld()->GetGameState<AERGameState>();
    if (!Mode || !Mode->IsLobby() || !State || State->bLobbyStarting || Team < 0 || Team > 1) return;
    if (auto* PS = GetPlayerState<AERPlayerState>())
    { AERClaymore::RemoveFor(GetWorld(),PS); PS->Team = Team == 0 ? EERTeam::Alpha : EERTeam::Omega; PS->bLobbyReady = false; PS->ForceNetUpdate(); }
}
void AERPlayerController::ServerLobbyVote_Implementation(int32 Map)
{
    const auto* Mode = GetWorld()->GetAuthGameMode<AERGameMode>();
    const auto* State = GetWorld()->GetGameState<AERGameState>();
    if (!Mode || !Mode->IsLobby() || !State || State->bLobbyStarting || Map < -1 || Map >= ERMaps::All().Num()) return;
    if (Map >= 0 && !ERMaps::At(Map).InRotation) return;
    if (auto* PS = GetPlayerState<AERPlayerState>()) { PS->MapVote = Map; PS->ForceNetUpdate(); }
}
void AERPlayerController::ServerLobbyReady_Implementation(bool Ready)
{
    auto* Mode = GetWorld()->GetAuthGameMode<AERGameMode>();
    const auto* State = GetWorld()->GetGameState<AERGameState>();
    if (!Mode || !Mode->IsLobby() || !State || State->bLobbyStarting) return;
    if (auto* PS = GetPlayerState<AERPlayerState>()) { PS->bLobbyReady = Ready; PS->ForceNetUpdate(); }
    Mode->CheckLobby();
}
void AERPlayerController::ServerSetSupplies_Implementation(int32 Magazines, int32 HE, int32 Smoke, int32 Stun)
{
    if(!GetWorld()->URL.HasOption(TEXT("ERLegacyArsenal")))return;
    const auto* State = GetWorld()->GetGameState<AERGameState>();
    if (State && State->bLobbyStarting) return;
    if (Magazines < 0 || Magazines > 20 || HE < 0 || HE > 20 || Smoke < 0 || Smoke > 20 || Stun < 0 || Stun > 20) return;
    if (auto* PS = GetPlayerState<AERPlayerState>())
    {
        PS->SpareMagazines = Magazines; PS->HECount = HE; PS->SmokeCount = Smoke; PS->StunCount = Stun;
        PS->bLobbyReady = false; PS->ForceNetUpdate();
    }
}
void AERPlayerController::PostSeamlessTravel()
{
    Super::PostSeamlessTravel();
    MenuPage = 0;
    if (IsLocalController()) ShowMenu(IsMenuLevel());
}
