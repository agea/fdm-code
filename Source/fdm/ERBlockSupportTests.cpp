#include "ERBlockSupport.h"
#include "ERBlockRoom.h"
#include "ERProceduralMap.h"
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FERBlockSupportTest,"EclipseRecoil.Destruction.StructuralSupport",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FERBlockSupportTest::RunTest(const FString& Parameters)
{
    FERBlockSupportGraph Graph;
    TArray<FERBlockRecord> B;
    auto Add=[&](FVector P) {auto& R=B.AddDefaulted_GetRef();R.Id={0,B.Num()-1,0};R.LocalTransform=FTransform(FQuat::Identity,P,FVector(.5));};
    Add(FVector(0,0,25));Add(FVector(0,0,75));Add(FVector(50,0,75)); // Sideways supported header.
    auto Run=[&]() { Graph.Start();int32 Steps=0;while (!Graph.Step(B,1,.5) && ++Steps<1000) {TestTrue(TEXT("work budget"),Graph.LastWork<=1);}TestTrue(TEXT("completes"),Graph.IsReady()); };
    TestTrue(TEXT("face graph"),Graph.Build(B));Run();TestEqual(TEXT("anchored cantilever"),Graph.GetUnsupported().Num(),0);
    B[0].State=EERBlockState::Destroyed;Run();TestEqual(TEXT("two connected unsupported"),Graph.GetUnsupported().Num(),2);TestEqual(TEXT("one component"),Graph.GetComponentCount(),1);
    B[2].bStructuralAnchor=true;TestTrue(TEXT("explicit anchor"),Graph.Build(B));Run();TestEqual(TEXT("explicit anchor retains header"),Graph.GetUnsupported().Num(),0);
    B[2].bStructuralAnchor=false;B[2].bIndestructible=true;Graph.Build(B);Run();TestEqual(TEXT("indestructible anchor"),Graph.GetUnsupported().Num(),0);
    B.Empty();Add(FVector(0,0,25));Add(FVector(50,50,25));Add(FVector(0,50,75)); // Edge-only and corner-only contacts.
    Graph.Build(B,false);B[0].bStructuralAnchor=true;Graph.Build(B,false);Run();
    TestEqual(TEXT("edge and corner do not support"),Graph.GetUnsupported().Num(),2);TestEqual(TEXT("separate components"),Graph.GetComponentCount(),2);
    B[1].State=EERBlockState::Detached;Run();TestEqual(TEXT("detached skipped"),Graph.GetUnsupported().Num(),1);
    B[2].LocalTransform.SetRotation(FRotator(0,45,0).Quaternion());TestFalse(TEXT("nonaxis geometry rejected"),Graph.Build(B));
    auto* Logical=NewObject<UERBlockWorld>();auto* Catalog=NewObject<UERBlockMaterialCatalog>();
    Logical->GenerateRoom(FERBlockRoomConfig{},Catalog);Graph.Build(Logical->GetBlocks());Graph.Start();
    int32 Steps=0;while (!Graph.Step(Logical->GetBlocks(),64,1) && ++Steps<1000) {}
    TestEqual(TEXT("clipped corners and full wall anchored"),Graph.GetUnsupported().Num(),0);
    // New damage during an in-flight job is eventually rechecked, not used as a
    // reason to restart traversal every shot and starve completion.
    Graph.Start();Graph.Step(Logical->GetBlocks(),64,1);
    for (const auto& R:Logical->GetBlocks()) if (R.Id.Row==0) Logical->DestroyBlock(R.Id);
    Steps=0;while (!Graph.Step(Logical->GetBlocks(),64,1) && ++Steps<1000) {}
    Graph.Start();Steps=0;while (!Graph.Step(Logical->GetBlocks(),64,1) && ++Steps<1000) {}
    TestEqual(TEXT("all upper courses unsupported after recheck"),Graph.GetUnsupported().Num(),462);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FERBlockCollapseTest,"EclipseRecoil.Destruction.CollapseAndIdentity",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FERBlockCollapseTest::RunTest(const FString& Parameters)
{
    UWorld::InitializationValues Values;Values.AllowAudioPlayback(false).CreateNavigation(false).CreateAISystem(false).CreateFXSystem(false).EnableTraceCollision(true);
    auto* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Values);
    if (!TestNotNull(TEXT("world"),World)) return false;
    auto* Room=World->SpawnActor<AERBlockRoom>();Room->Collapse.MaxDebris=0;Room->Collapse.WorkPerStep=64;Room->Collapse.RemovalsPerStep=4;
    Room->Regenerate();
    TArray<FERBlockDamageRequest> Ring;
    for (int32 C=3;C<=7;++C) for (int32 R=5;R<=9;++R)
        if (C==3 || C==7 || R==5 || R==9) Ring.Add({{0,C,R},0,true});
    Room->ApplyDamageBatch(Ring);
    TestEqual(TEXT("16 direct removals"),Room->BlockWorld->GetDestroyedCount(),16);
    int32 Steps=0;
    while (Room->IsSupportPending() && Steps++<1000)
    {
        if (Steps==2) Room->ApplyBlockDamage({2,4,3},0,true);
        Room->StepSupport();TestTrue(TEXT("step work bounded"),Room->LastSupportWork<=64);TestTrue(TEXT("detach bounded"),Room->LastDetached<=4);
        TestTrue(TEXT("native mappings after every step"),Room->ValidateMappings());
    }
    TestTrue(TEXT("coalesced damage completes"),!Room->IsSupportPending());
    TestEqual(TEXT("isolated patch detaches"),Room->BlockWorld->GetDetachedCount(),9);
    TestTrue(TEXT("anchored remainder stays"),Room->BlockWorld->Find({0,2,6})->IsAttached());
    auto Hits=Room->BlockWorld->QuerySphere(FVector(0,-260,180),500);
    for (auto Id:Hits) TestTrue(TEXT("spatial query excludes removed"),Room->BlockWorld->Find(Id)->IsAttached());
    TestFalse(TEXT("no repeat damage detached"),Room->ApplyBlockDamage({0,4,6},100));
    FHitResult Hit;FCollisionQueryParams Query(SCENE_QUERY_STAT(ERCollapseTest),false);
    TestFalse(TEXT("detached has no static/query collision"),World->LineTraceSingleByChannel(Hit,FVector(-25,0,162.5),FVector(-25,-350,162.5),ECC_GameTraceChannel1,Query));
    Room->Regenerate();TestTrue(TEXT("regeneration clears pending"),!Room->IsSupportPending());TestEqual(TEXT("regeneration clears detached"),Room->BlockWorld->GetDetachedCount(),0);
    Room->Collapse.RemovalsPerStep=10000;Room->Collapse.WorkPerStep=100000;
    Ring.Reset();for (const auto& B:Room->BlockWorld->GetBlocks()) if (B.Id.Row==0) Ring.Add({B.Id,0,true});
    Room->ApplyDamageBatch(Ring);Steps=0;while (Room->IsSupportPending() && Steps++<1000) {Room->StepSupport();TestTrue(TEXT("hard work cap"),Room->LastSupportWork<=16384);TestTrue(TEXT("hard removal cap"),Room->LastDetached<=256);}
    TestEqual(TEXT("entire upper shell removed"),Room->BlockWorld->GetDetachedCount(),462);TestEqual(TEXT("all wall bodies gone"),Room->GetMemoryProfile().CollisionBodies,0);
    auto* Map=World->SpawnActor<AERProceduralMap>();Map->Regenerate();Map->Collapse.MaxDebris=0;
    TestTrue(TEXT("procedural baseline supported"),Map->GetSupportBuildMs()>0);
    Map->RequestSupportCheck();Steps=0;while(Map->IsSupportPending() && Steps++<10000) Map->StepSupport();
    TestEqual(TEXT("doors/windows leave supported headers"),Map->BlockWorld->GetDetachedCount(),0);TestTrue(TEXT("procedural mappings"),Map->ValidateMappings());
    Room->Destroy();Map->Destroy();World->DestroyWorld(false);return true;
}
#endif
