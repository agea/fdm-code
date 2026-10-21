#include "EROnlineProbe.h"
#include "EROnline.h"
#include "ERPlayerController.h"
#include "ERMatch.h"
#include "ERNetDriverEOS.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Engine/World.h"

void UEROnlineProbe::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Collection.InitializeDependency<UEROnline>();
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("EROnlineServiceProbe")))
        Ticker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UEROnlineProbe::Tick), .1f);
#endif
}
void UEROnlineProbe::Check(bool Value, const TCHAR* Label)
{
    bFailed |= !Value;
    UE_LOG(LogTemp, Display, TEXT("ER_EOS_PROBE_%s %s"), Value ? TEXT("PASS") : TEXT("FAIL"), Label);
}
void UEROnlineProbe::FindComplete(bool Success)
{
    Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
    bSearchRunning = false;
    if (!Success) { Check(false, TEXT("service_search")); return; }
    for (const auto& Result : Query->SearchResults)
    {
        if (Result.GetSessionIdStr() != SessionId) continue;
        FString Address, Version;
        Result.Session.SessionSettings.Get(FName(TEXT("RR_VERSION")), Version);
        Check(Sessions->GetResolvedConnectString(Result, NAME_GamePort, Address) && Address.StartsWith(TEXT("[EOS:")), TEXT("service_search_resolves_p2p_endpoint"));
        Check(Version == CastChecked<UERGameInstance>(GetGameInstance())->BuildVersion, TEXT("service_search_preserves_game_version"));
        bFound = true;
        break;
    }
    SearchAgainAt = FPlatformTime::Seconds() + 2;
    UE_LOG(LogTemp, Display, TEXT("ER_EOS_PROBE_SEARCH found_own_session=%d results=%d"), bFound, Query->SearchResults.Num());
}
bool UEROnlineProbe::Tick(float Delta)
{
#if !UE_BUILD_SHIPPING
    auto* GI = Cast<UERGameInstance>(GetGameInstance());
    auto* World = GetWorld();
    auto* PC = GI ? Cast<AERPlayerController>(GI->GetFirstLocalPlayerController()) : nullptr;
    if (!GI || !World || !PC || World->bIsTearingDown) return true;
    auto* Online = GI->GetSubsystem<UEROnline>();
    const double Now = FPlatformTime::Seconds();
    if (Phase && (bFailed || Now > Deadline))
    {
        if (Now > Deadline) Check(false, TEXT("operation_timeout"));
        UE_LOG(LogTemp, Display, TEXT("ER_EOS_PROBE_COMPLETE result=FAIL phase=%d status=%s"), Phase, *Online->Status);
        Online->Leave();
        FPlatformMisc::RequestExitWithStatus(false, 1);
        return false;
    }
    if (Phase == 0)
    {
        if (World->GetTimeSeconds() < 2) return true;
        GI->MaxPlayers = 2;
        GI->AlphaBots = GI->OmegaBots = 0;
        GI->LocalName = TEXT("EOSProbe");
        Phase = 1; Deadline = Now + 240;
        UE_LOG(LogTemp, Display, TEXT("ER_EOS_PROBE_BEGIN authenticate_and_host"));
        Online->Host();
    }
    else if (Phase == 1 || Phase == 5)
    {
        if (!Online->IsBusy() && !Online->IsOnline())
        { Check(false, TEXT("authenticated_session_creation")); return true; }
        const auto* GS = World->GetGameState<AERGameState>();
        if (World->GetNetMode() != NM_ListenServer || !GS || !GS->bLobby || !PC->GetPlayerState<AERPlayerState>()) return true;
        const auto* Driver = Cast<UERNetDriverEOS>(World->GetNetDriver());
        Check(Driver && !Driver->bIsPassthrough, TEXT("real_eos_p2p_listen_driver"));
        Check(GS->bOnline && GS->ServerMaxPlayers == 2, TEXT("online_lobby_and_capacity"));
        auto* EOS = Online::GetSubsystem(World, FName(TEXT("EOS")));
        Check(EOS && EOS->GetIdentityInterface()->GetLoginStatus(0) == ELoginStatus::LoggedIn, TEXT("epic_account_and_connect_logged_in"));
        Sessions = EOS ? EOS->GetSessionInterface() : nullptr;
        auto* Session = Sessions.IsValid() ? Sessions->GetNamedSession(NAME_GameSession) : nullptr;
        Check(Session && Session->SessionSettings.NumPublicConnections == 2, TEXT("real_service_session_exists"));
        if (!Session || bFailed) return true;
        SessionId = Session->GetSessionIdStr();
        if (Phase == 5)
        {
            Check(true, TEXT("rehost_after_destroy_and_cancel"));
            Phase = 6; Deadline = Now + 90;
            PC->ServerLobbyVote(1);
            PC->ServerLobbyReady(true);
        }
        else { Phase = 2; Deadline = Now + 90; SearchAgainAt = Now; }
    }
    else if (Phase == 2)
    {
        if (bFound)
        {
            const auto* Session = Sessions->GetNamedSession(NAME_GameSession);
            Check(Session && Session->RegisteredPlayers.Num() == 1, TEXT("host_registered_with_service"));
            GI->ReturnToMenu(); Phase = 3; Deadline = Now + 60;
        }
        else if (!bSearchRunning && Now >= SearchAgainAt)
        {
            Query = MakeShared<FOnlineSessionSearch>();
            Query->bIsLanQuery = false; Query->MaxSearchResults = 100;
            Query->QuerySettings.Set(FName(TEXT("RR_GAME")), FString(TEXT("RespawnAndRage")), EOnlineComparisonOp::Equals);
            Query->QuerySettings.Set(FName(TEXT("RR_VERSION")), GI->BuildVersion, EOnlineComparisonOp::Equals);
            FindHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(FOnFindSessionsCompleteDelegate::CreateUObject(this, &UEROnlineProbe::FindComplete));
            bSearchRunning = true;
            if (!Sessions->FindSessions(0, Query.ToSharedRef()) && bSearchRunning)
            { Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle); bSearchRunning = false; Check(false, TEXT("service_search_started")); }
        }
    }
    else if (Phase == 3)
    {
        if (Online->IsBusy() || World->GetNetMode() != NM_Standalone) return true;
        Check(!Sessions->GetNamedSession(NAME_GameSession), TEXT("leave_destroys_service_session"));
        Online->Host(); Online->Cancel();
        Phase = 4; Deadline = Now + 90;
    }
    else if (Phase == 4)
    {
        if (Online->IsBusy()) return true;
        Check(!Online->IsOnline() && !Sessions->GetNamedSession(NAME_GameSession) && World->GetNetMode() == NM_Standalone, TEXT("cancel_inflight_create_cleans_up_without_travel"));
        Online->Host(); Phase = 5; Deadline = Now + 90;
    }
    else if (Phase == 6)
    {
        if (!World->GetMapName().Contains(TEXT("L_Depot")) || !PC->GetPawn()) return true;
        const auto* GS = World->GetGameState<AERGameState>();
        const auto* Driver = Cast<UERNetDriverEOS>(World->GetNetDriver());
        Check(Online->IsOnline() && GS && GS->bOnline && Driver && !Driver->bIsPassthrough, TEXT("online_ready_travel_to_gameplay_keeps_p2p"));
        GI->ReturnToMenu(); Phase = 7; Deadline = Now + 60;
    }
    else if (Phase == 7)
    {
        if (Online->IsBusy() || World->GetNetMode() != NM_Standalone) return true;
        Check(!Online->IsOnline() && !Sessions->GetNamedSession(NAME_GameSession), TEXT("final_session_cleanup"));
        Online->Host(); Phase = 8; Deadline = Now + 90;
    }
    else if (Phase == 8)
    {
        if (!Online->IsBusy() && !Online->IsOnline()) { Check(false, TEXT("quit_fixture_hosted")); return true; }
        if (World->GetNetMode() != NM_ListenServer || !Online->HasSession()) return true;
        Check(true, TEXT("quit_requested_with_live_eos_session"));
        UE_LOG(LogTemp, Display, TEXT("ER_EOS_PROBE_COMPLETE result=%s"), bFailed ? TEXT("FAIL") : TEXT("PASS"));
        GI->QuitGame();
        return false;
    }
#endif
    return true;
}
void UEROnlineProbe::Deinitialize()
{
    FTSTicker::GetCoreTicker().RemoveTicker(Ticker);
    if (Sessions.IsValid()) Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
    Query.Reset(); Sessions.Reset();
    Super::Deinitialize();
}
