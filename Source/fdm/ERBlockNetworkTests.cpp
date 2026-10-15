#include "ERBlockRoom.h"
#include "ERBlockNetPage.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FERBlockNetTransactionTest,"EclipseRecoil.Destruction.NetworkTransactions",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FERBlockNetTransactionTest::RunTest(const FString& Parameters)
{
    UWorld::InitializationValues V;V.AllowAudioPlayback(false).CreateNavigation(false).CreateAISystem(false).CreateFXSystem(false).EnableTraceCollision(true);
    auto* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&V);if(!World)return false;
    auto* Server=World->SpawnActor<AERBlockRoom>();Server->Regenerate();
    auto* Client=World->SpawnActor<AERBlockRoom>();Client->SetRole(ROLE_SimulatedProxy);Client->bNetworkBegunPlay=true;
    Client->Network.Epoch=1;Client->Network.Sequence=1;Client->Network.Blocks=Server->Config;Client->Network.BlockCount=504;Client->Network.LayoutDigest=Server->GetLayoutDigest();Client->Network.PageVersions.Init(1,4);
    Client->OnRep_Network();TestFalse(TEXT("initial collision not released before snapshot"),Client->IsMapReady());
    TArray<AERBlockNetPage*> Pages;
    for(int32 I=0;I<4;++I)
    {
        auto* P=World->SpawnActor<AERBlockNetPage>();P->Room=Client;P->State.Epoch=1;P->State.Index=I;P->State.Version=1;
        for(int32 J=I*128;J<FMath::Min((I+1)*128,504);++J){const auto& R=Server->BlockWorld->GetBlocks()[J];P->State.Health.Add(R.Health);P->State.States.Add(uint8(R.State));}
        Pages.Add(P);
    }
    // Reverse/out-of-order arrival, holding a complete immutable snapshot.
    for(int32 I:{3,1,0}){Client->ReceiveNetworkPage(Pages[I]);TestFalse(TEXT("partial snapshot stays gated"),Client->IsMapReady());}
    Client->ReceiveNetworkPage(Pages[2]);TestTrue(TEXT("complete snapshot ready"),Client->IsMapReady());TestEqual(TEXT("all initial values identical"),Client->GetStateDigest(),Server->GetStateDigest());
    TestFalse(TEXT("proxy cannot mutate"),Client->ApplyBlockDamage({0,0,0},100,true));TestFalse(TEXT("proxy cannot regenerate"),Client->Regenerate());
    Pages[0]->State.Version=2;Pages[0]->State.Health[0]=0;Pages[0]->State.States[0]=uint8(EERBlockState::Destroyed);Client->ReceiveNetworkPage(Pages[0]);
    TestEqual(TEXT("future page not applied without manifest"),Client->BlockWorld->GetRemovedCount(),0);
    Client->Network.Sequence=2;Client->Network.PageVersions[0]=Client->Network.PageVersions[1]=2;Client->OnRep_Network();
    TestEqual(TEXT("partial transaction cannot open half a breach"),Client->BlockWorld->GetRemovedCount(),0);
    Pages[1]->State.Version=2;Pages[1]->State.Health[0]*=.5;Pages[1]->State.States[0]=uint8(EERBlockState::Damaged);Client->ReceiveNetworkPage(Pages[1]);
    TestEqual(TEXT("coherent transaction applied"),Client->BlockWorld->GetDestroyedCount(),1);TestTrue(TEXT("post-transaction native mappings"),Client->ValidateMappings());
    auto* Stale=World->SpawnActor<AERBlockNetPage>();Stale->Room=Client;Stale->State=Pages[0]->State;Stale->State.Epoch=0;Client->ReceiveNetworkPage(Stale);
    TestEqual(TEXT("old epoch ignored"),Client->BlockWorld->GetDestroyedCount(),1);
    // Invalid payload is rejected before any member of that transaction changes.
    const uint32 Before=Client->GetStateDigest();Pages[0]->State.Version=3;Pages[0]->State.States[1]=255;Client->Network.Sequence=3;Client->Network.PageVersions[0]=3;
    AddExpectedError(TEXT("ER_BLOCK_NET_INCOMPATIBLE state"),EAutomationExpectedErrorFlags::Contains,1);
    Client->OnRep_Network();TestTrue(TEXT("invalid payload detected"),Client->bNetworkError);TestEqual(TEXT("invalid transaction preserves state"),Client->GetStateDigest(),Before);
    Client->Network.Epoch=2;Client->Network.Sequence=1;Client->Network.PageVersions.Init(1,4);Client->OnRep_Network();TestFalse(TEXT("new epoch waits for new snapshot"),Client->IsMapReady());
    for(auto* P:Pages)
    {
        P->State.Epoch=2;P->State.Version=1;
        for(int32 J=0;J<P->State.Health.Num();++J){const auto& R=Server->BlockWorld->GetBlocks()[P->State.Index*128+J];P->State.Health[J]=R.Health;P->State.States[J]=uint8(R.State);}
        Client->ReceiveNetworkPage(P);
    }
    TestTrue(TEXT("new epoch restored"),Client->IsMapReady());TestEqual(TEXT("stale removals cleared"),Client->BlockWorld->GetRemovedCount(),0);
    Client->Network.Epoch=3;Client->Network.Protocol=99;
    AddExpectedError(TEXT("ER_BLOCK_NET_INCOMPATIBLE descriptor"),EAutomationExpectedErrorFlags::Contains,1);Client->OnRep_Network();TestTrue(TEXT("protocol mismatch fails closed"),Client->bNetworkError);
    World->DestroyWorld(false);return true;
}
#endif
