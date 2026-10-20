#include "ERLobbyRules.h"
#include "ERMatch.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FERLobbyQuorum, "EclipseRecoil.Menu.LobbyQuorumAndVotes", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FERLobbyQuorum::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("initial server capacity"), ERServer::DefaultMaxPlayers, 16);
    TestEqual(TEXT("zero capacity becomes one"), ERServer::ValidMaxPlayers(0), 1);
    TestEqual(TEXT("negative capacity becomes one"), ERServer::ValidMaxPlayers(-7), 1);
    TestEqual(TEXT("capacity above former cap retained"), ERServer::ValidMaxPlayers(33), 33);
    TestEqual(TEXT("typed capacity is not limited by slider range"), ERServer::ValidMaxPlayers(128), 128);
    TestFalse(TEXT("half of sixteen insufficient"), ERLobby::HasQuorum(16, 8));
    TestTrue(TEXT("nine of sixteen starts"), ERLobby::HasQuorum(16, 9));
    TestFalse(TEXT("empty lobby cannot start"), ERLobby::HasQuorum(0, 0));
    TestTrue(TEXT("solo player ready can start"), ERLobby::HasQuorum(1, 1));
    TestFalse(TEXT("one of two insufficient"), ERLobby::HasQuorum(2, 1));
    TestTrue(TEXT("two of three is majority"), ERLobby::HasQuorum(3, 2));
    TestFalse(TEXT("half of four insufficient"), ERLobby::HasQuorum(4, 2));
    TestTrue(TEXT("three of four sufficient"), ERLobby::HasQuorum(4, 3));
    TestEqual(TEXT("zero votes admits all maps"), ERLobby::LeadingMaps(TArray<int32>{-1,-1}, 2), TArray<int32>({0,1}));
    TestEqual(TEXT("tied leaders only"), ERLobby::LeadingMaps(TArray<int32>{0,1}, 3), TArray<int32>({0,1}));
    TestEqual(TEXT("plurality without majority wins"), ERLobby::LeadingMaps(TArray<int32>{2,2,1,0,-1}, 3), TArray<int32>({2}));
    TestEqual(TEXT("out of range votes ignored"), ERLobby::LeadingMaps(TArray<int32>{-99,99,1}, 2), TArray<int32>({1}));
    return true;
}
#endif
