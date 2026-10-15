#include "ERProceduralMap.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "UObject/GarbageCollection.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FERBlockStressTest,"EclipseRecoil.Destruction.StressAndBatchedIdentity",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FERBlockStressTest::RunTest(const FString& Parameters)
{
    UWorld::InitializationValues Values;Values.AllowAudioPlayback(false).CreateNavigation(false).CreateAISystem(false).CreateFXSystem(false).EnableTraceCollision(true);
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Values);
    if (!TestNotNull(TEXT("Stress physics world"),World)) return false;
    auto* Map=World->SpawnActor<AERProceduralMap>();
    if (!Map) {World->DestroyWorld(false);return false;}
    Map->LayoutConfig.RoomCount=10;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(ERBlockStressIdentity),false);FHitResult Hit;
    int32 Cycles=0;
    for (int32 Chunk:{200,400,800})
        for (int32 Round=0;Round<6;++Round)
        {
            Map->Config.ChunkSize=Chunk;Map->LayoutConfig.Seed=7919+Round;
            if (!TestTrue(TEXT("Repeated generation"),Map->Regenerate())) break;
            const int32 Full=Map->GetInstanceCount();
            const auto Before=Map->GetMemoryProfile();
            TestTrue(TEXT("Released duplicate layout blocks"),Map->GetLayout().Blocks.IsEmpty());
            TArray<FERBlockId> Order;
            for (const auto& B:Map->BlockWorld->GetBlocks()) Order.Add(B.Id);
            FRandomStream Random(7919+Round);
            for (int32 I=Order.Num()-1;I>0;--I) Order.Swap(I,Random.RandRange(0,I));
            // Most of the map, in duplicate/triplicate request order: mutation
            // and component updates must still occur only once per unique ID/group.
            TSet<FERBlockGroupKey> Dirty;
            TArray<FERBlockDamageRequest> Requests;
            const int32 Remove=Full*9/10;
            for (int32 I=0;I<Remove;++I)
            {
                const auto* B=Map->BlockWorld->Find(Order[I]);Dirty.Add({B->Chunk,B->Material});
                for (int32 Repeat=0;Repeat<3;++Repeat) Requests.Add({B->Id,0,true});
            }
            const auto Batch=Map->ApplyDamageBatch(Requests);
            TestEqual(TEXT("Exactly the requested unique blocks removed"),Batch.Destroyed,Remove);
            TestEqual(TEXT("Exactly one update per dirty group"),Batch.GroupsRebuilt,Dirty.Num());
            TestEqual(TEXT("Surviving bodies never uploaded again"),Map->UpdateProfile.Uploaded,0);
            TestEqual(TEXT("Collision bodies released exactly"),Map->GetMemoryProfile().CollisionBodies,Before.CollisionBodies-Remove);
            TestTrue(TEXT("All native handle and logical mappings valid"),Map->ValidateMappings());
            // Exhaustive identity checks for every surviving instance after swaps.
            for (int32 I=Remove;I<Full;++I)
            {
                const auto T=Map->GetBlockTransform(Order[I]);const FVector P=T.GetLocation(),N=T.GetRotation().GetAxisY();FERBlockId Id;
                auto Trace=[&](float Side) {return World->LineTraceSingleByChannel(Hit,P+N*11*Side,P-N*11*Side,ECC_GameTraceChannel1,Query) && Map->ResolveHit(Hit,Id) && Id==Order[I];};
                if (!(Trace(1)||Trace(-1))) {AddError(TEXT("Survivor collision reports a different logical ID"));break;}
            }
            for (int32 I=0;I<Remove;I+=17)
            {
                const auto T=Map->GetBlockTransform(Order[I]);const FVector P=T.GetLocation(),N=T.GetRotation().GetAxisY();
                TestFalse(TEXT("No collision inside removed volumes"),World->LineTraceSingleByChannel(Hit,P+N*9,P-N*9,ECC_GameTraceChannel1,Query));
            }
            const int32 Components=Map->GetInstanceComponents().Num();
            TestTrue(TEXT("Regenerate identical configuration"),Map->Regenerate());
            TestEqual(TEXT("No accumulated registered components"),Map->GetInstanceComponents().Num(),Components);
            TestTrue(TEXT("Regenerated mappings"),Map->ValidateMappings());
            // Exclusive allocations are deterministic for identical live geometry;
            // no RSS equality assertion: engine caches/allocator pools are shared.
            const auto Restored=Map->GetMemoryProfile();
            TestEqual(TEXT("Body count restored"),Restored.CollisionBodies,Before.CollisionBodies);
            TestEqual(TEXT("Tracked allocations stable"),Restored.TrackedBytes(),Before.TrackedBytes());
            CollectGarbage(RF_NoFlags);++Cycles;
        }
    UE_LOG(LogTemp,Display,TEXT("ER_BLOCK_STRESS_AUTOMATION cycles=%d"),Cycles);
    World->DestroyWorld(false);return true;
}
#endif
