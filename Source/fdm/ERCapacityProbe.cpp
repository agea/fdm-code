#include "ERCapacityProbe.h"
#include "ERPlayerController.h"
#include "ERMatch.h"
#include "ERCharacter.h"
#include "ERBlockRoom.h"
#include "ERMapCatalog.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/GameSession.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"

AERCapacityProbe::AERCapacityProbe()
{
    bReplicates = true; bAlwaysRelevant = true; PrimaryActorTick.bCanEverTick = true;
}
void AERCapacityProbe::Check(bool Value, const TCHAR* Label)
{
    Failed |= !Value;
    UE_LOG(LogTemp, Display, TEXT("ER_CAPACITY_%s %s"), Value ? TEXT("PASS") : TEXT("FAIL"), Label);
}
void AERCapacityProbe::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
#if !UE_BUILD_SHIPPING
    auto* PC = Cast<AERPlayerController>(GetWorld()->GetFirstPlayerController());
    auto* GI = GetGameInstance<UERGameInstance>();
    auto* GS = GetWorld()->GetGameState<AERGameState>();
    if (!PC || !PC->IsLocalController() || !GI || !GS || GetWorld()->GetTimeSeconds() < 1) return;
    const bool Host = FParse::Param(FCommandLine::Get(), TEXT("ERCapacityHost"));
    int32 Limit = 6, Client = 0, Map = INDEX_NONE;
    FString MapId;
    FParse::Value(FCommandLine::Get(), TEXT("ERCapacityLimit="), Limit);
    FParse::Value(FCommandLine::Get(), TEXT("ERCapacityClient="), Client);
    FParse::Value(FCommandLine::Get(), TEXT("ERCapacityMap="), MapId);
    for (int32 I = 0; I < ERMaps::All().Num(); ++I) if (MapId == ERMaps::At(I).Id) Map = I;
    if (!ERMaps::Enabled(Map))
    {
        Check(false, TEXT("capacity_map_in_lobby_pool"));
        FPlatformMisc::RequestExitWithStatus(false, 1);
        return;
    }
    const double Now = GetWorld()->GetTimeSeconds();
    if (PC->IsMenuLevel() && GetNetMode() == NM_Standalone)
    {
        if (Phase == 0)
        {
            GI->LocalName = Host ? TEXT("CapacityHost") : FString::Printf(TEXT("CapacityClient%d"), Client);
            GI->PlayerTeam = 0;
            if (Host) { GI->MaxPlayers = Limit; GI->AlphaBots = GI->OmegaBots = 0; GI->StartLANServer(); }
            else GI->FindLANServer();
            Phase = 1;
        }
        return;
    }
    auto* PS = PC->GetPlayerState<AERPlayerState>();
    if (!PS) return;
    if (GS->bLobby)
    {
        if (Phase == 0)
        {
            Check(GS->ServerMaxPlayers == Limit, TEXT("capacity_replicated_in_lobby"));
            PC->ServerLobbyTeam(0); // all humans on one team exercises crowded bases
            PC->ServerLobbyVote(Map);
            Phase = 1;
        }
        if (GS->PlayerArray.Num() != Limit) return;
        AERPlayerState* HostState = nullptr;
        int32 Ready = 0;
        for (auto Item : GS->PlayerArray) if (auto* ItemPS = Cast<AERPlayerState>(Item))
        { Ready += ItemPS->bLobbyReady; if (ItemPS->GetPlayerName() == TEXT("CapacityHost")) HostState = ItemPS; }
        const int32 Needed = Limit / 2 + 1;
        if (Host && Phase == 1)
        {
            PC->ServerLobbyReady(true);
            Phase = 2;
        }
        else if (Host && Phase == 2 && Ready == Needed - 1)
        {
            Check(!GS->bLobbyStarting, TEXT("below_dynamic_quorum_waits"));
            Check(!GetWorld()->GetAuthGameMode<AERGameMode>()->HasOpenPlayerSlot(), TEXT("full_lobby_uses_configured_limit"));
            PC->ServerLobbyVote(-1); // release the last quorum voter after observing the boundary
            Phase = 3;
        }
        if (!Host && Phase == 1 && HostState && HostState->bLobbyReady)
        {
            if (Client < Needed - 1 || (Client == Needed - 1 && HostState->MapVote == -1))
            { PC->ServerLobbyReady(true); Phase = 2; }
        }
        return;
    }
    if (!PC->GetPawn() || GS->PlayerArray.Num() < Limit) return;
    if (ERMaps::At(Map).Procedural)
    {
        TActorIterator<AERBlockRoom> Room(GetWorld());
        if (!Room || !Room->IsMapReady() || PC->IsBlockMapLoading()) return;
    }
    if (!Host)
    {
        if (Phase == 0)
        {
            Check(GS->ServerMaxPlayers == Limit && PS->Team == EERTeam::Alpha, TEXT("capacity_and_team_survive_travel"));
            UE_LOG(LogTemp, Display, TEXT("ER_CAPACITY_CLIENT_SPAWNED id=%d"), Client);
            Phase = 1;
        }
        return;
    }
    auto* Mode = GetWorld()->GetAuthGameMode<AERGameMode>();
    TArray<AERCharacter*> Humans;
    AERCharacter* Replacement = nullptr;
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        auto* Character = Cast<AERCharacter>(It->Get()->GetPawn());
        if (!Character) return;
        Humans.Add(Character);
        if (It->Get()->GetPlayerState<AERPlayerState>()->GetPlayerName() == TEXT("CapacityClient99")) Replacement = Character;
    }
    if (Humans.Num() != Limit) return;
    if (Phase == 0)
    {
        Check(GS->ServerMaxPlayers == Limit && Mode && Mode->GetMaxPlayers() == Limit && Mode->GameSession->MaxPlayers == Limit, TEXT("capacity_survives_server_travel"));
        bool Separated = true, Supported = true, TeamCorrect = true;
        FCollisionQueryParams Query(SCENE_QUERY_STAT(ERCapacityFloor), false);
        for (auto* Human : Humans) Query.AddIgnoredActor(Human);
        for (int32 I = 0; I < Humans.Num(); ++I)
        {
            const auto* C = Humans[I];
            TeamCorrect &= C->GetPlayerState<AERPlayerState>()->Team == EERTeam::Alpha;
            FHitResult Floor;
            const FVector P = C->GetActorLocation();
            Supported &= GetWorld()->LineTraceSingleByChannel(Floor, P, P - FVector(0,0,140), ECC_WorldStatic, Query) && Floor.ImpactNormal.Z > .7f;
            for (int32 J = 0; J < I; ++J)
                Separated &= FVector::Dist2D(P, Humans[J]->GetActorLocation()) >= C->GetCapsuleComponent()->GetScaledCapsuleRadius() + Humans[J]->GetCapsuleComponent()->GetScaledCapsuleRadius() - 1;
        }
        Check(TeamCorrect, TEXT("all_six_players_can_choose_same_team"));
        Check(Separated && Supported, TEXT("crowded_team_spawns_have_floor_and_do_not_overlap"));
        Check(GetWorld()->GetMapName().Contains(FPaths::GetBaseFilename(ERMaps::At(Map).Package)), TEXT("voted_map_loaded"));
        TArray<AERTeamPlayerStart*> AlphaStarts;
        int32 Omega = 0;
        for (TActorIterator<AERTeamPlayerStart> Start(GetWorld()); Start; ++Start)
        {
            if (Start->Team == EERTeam::Alpha) AlphaStarts.Add(*Start);
            Omega += Start->Team == EERTeam::Omega;
        }
        Check(AlphaStarts.Num() >= Limit && Omega >= Limit, TEXT("both_teams_have_a_start_per_player"));
        // Layouts carry more starts than this lobby has players, so the overflow search is asked directly.
        const auto* Overflow = Mode ? Mode->FindAdditionalStart(PC, AlphaStarts) : nullptr;
        Check(Overflow && Overflow->ActorHasTag(TEXT("ERAdditionalStart")), TEXT("crowded_base_finds_additional_spawn_position"));
        UE_LOG(LogTemp, Display, TEXT("ER_CAPACITY_INITIAL result=%s humans=%d max=%d"), Failed ? TEXT("FAIL") : TEXT("PASS"), Humans.Num(), Limit);
        Phase = 1;
    }
    else if (Phase == 1 && Replacement)
    {
        Check(Mode->GetNumPlayers() == Limit && !Mode->HasOpenPlayerSlot(), TEXT("disconnect_releases_slot_and_replacement_spawns"));
        UE_LOG(LogTemp, Display, TEXT("ER_CAPACITY_COMPLETE result=%s"), Failed ? TEXT("FAIL") : TEXT("PASS"));
        Phase = 2; Deadline = Now + 3;
    }
    else if (Phase == 2 && Now > Deadline) FPlatformMisc::RequestExitWithStatus(false, Failed ? 1 : 0);
#endif
}
