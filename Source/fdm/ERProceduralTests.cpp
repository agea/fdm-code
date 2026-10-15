#include "ERProceduralMap.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FERProceduralLayoutTest,"EclipseRecoil.Destruction.ProceduralLayout",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FERProceduralLayoutTest::RunTest(const FString& Parameters)
{
    FERBlockRoomConfig B;
    for (int32 Count : {6,7,8,9,10})
        for (int32 Seed : {0,1,42,7919,-17})
        {
            FERProceduralConfig C; C.Seed=Seed; C.RoomCount=Count;
            FERProceduralLayout A,D,Other;
            TestTrue(TEXT("Generate bounded layout"),ERProcedural::Build(C,B,A));
            TestTrue(TEXT("Repeat seed"),ERProcedural::Build(C,B,D));
            TestEqual(TEXT("Requested rooms"),A.Rooms.Num(),Count);
            TestEqual(TEXT("Door pairs plus exterior windows"),A.Openings.Num(),Count*3);
            TestEqual(TEXT("Stable block count"),A.Blocks.Num(),D.Blocks.Num());
            TSet<FERBlockId> Ids;
            TMap<FIntVector,TArray<int32>> Buckets;
            for (int32 I=0;I<A.Blocks.Num();++I)
            {
                const auto& R=A.Blocks[I];
                if (Ids.Contains(R.Id)) { AddError(TEXT("Duplicate identity")); return false; } Ids.Add(R.Id);
                if (!(R.Id==D.Blocks[I].Id) || R.Material!=D.Blocks[I].Material || !R.LocalTransform.Equals(D.Blocks[I].LocalTransform)) { AddError(TEXT("Non-deterministic blocks")); return false; }
                const FVector P=R.LocalTransform.GetLocation();
                const FIntVector Cell(FMath::FloorToInt(P.X/100),FMath::FloorToInt(P.Y/100),FMath::FloorToInt(P.Z/100));
                const FVector Extent=R.LocalTransform.TransformVector(FVector(50)).GetAbs();
                for (int32 X=-1;X<=1;++X) for (int32 Y=-1;Y<=1;++Y) for (int32 Z=-1;Z<=1;++Z)
                    if (const auto* Neighbours=Buckets.Find(Cell+FIntVector(X,Y,Z))) for (int32 J:*Neighbours)
                    {
                        const auto& N=A.Blocks[J];
                        const FVector Sum=Extent+N.LocalTransform.TransformVector(FVector(50)).GetAbs();
                        const FVector Delta=(P-N.LocalTransform.GetLocation()).GetAbs();
                        if (Delta.X<Sum.X-.001 && Delta.Y<Sum.Y-.001 && Delta.Z<Sum.Z-.001) { AddError(FString::Printf(TEXT("Overlapping blocks %s / %s"),*R.Id.ToString(),*N.Id.ToString())); return false; }
                    }
                Buckets.FindOrAdd(Cell).Add(I);
            }
            ++C.Seed; TestTrue(TEXT("Different seed"),ERProcedural::Build(C,B,Other));
            bool Different=A.Blocks.Num()!=Other.Blocks.Num();
            for (int32 I=0; !Different && I<A.Blocks.Num(); ++I) Different=!A.Blocks[I].LocalTransform.Equals(Other.Blocks[I].LocalTransform) || A.Blocks[I].Material!=Other.Blocks[I].Material;
            TestTrue(TEXT("Seed changes geometry/materials"),Different);
            TestEqual(TEXT("Every room has a hall route"),A.Routes.Num(),Count);
        }
    for (int32 Width : {100,200})
    {
        FERProceduralConfig Edge; Edge.MinRoomSize=Edge.MaxRoomSize=400;Edge.CorridorWidth=150;Edge.BranchLength=150;Edge.DoorWidth=Width;
        FERProceduralLayout L;TestTrue(TEXT("Minimum room/branch and supported door widths"),ERProcedural::Build(Edge,B,L));
    }
    FERProceduralConfig C; FERProceduralLayout Layout;
    TestTrue(TEXT("Initial state"),ERProcedural::Build(C,B,Layout)); const int32 Count=Layout.Blocks.Num();
    C.RoomCount=100000; TestFalse(TEXT("Reject unbounded rooms"),ERProcedural::Build(C,B,Layout));
    TestEqual(TEXT("Invalid generation preserves layout"),Layout.Blocks.Num(),Count);
    C=FERProceduralConfig();C.DoorWidth=50;TestFalse(TEXT("Reject obstructed door width"),ERProcedural::Build(C,B,Layout));
    C=FERProceduralConfig();C.CorridorWidth=100;TestFalse(TEXT("Reject narrow corridor"),ERProcedural::Build(C,B,Layout));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FERProceduralCollisionTest,"EclipseRecoil.Destruction.ProceduralCollision",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FERProceduralCollisionTest::RunTest(const FString& Parameters)
{
    UWorld::InitializationValues Values; Values.AllowAudioPlayback(false).CreateNavigation(false).CreateAISystem(false).CreateFXSystem(false).EnableTraceCollision(true);
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Values);
    if (!TestNotNull(TEXT("Physics world"),World)) return false;
    auto* Map=World->SpawnActor<AERProceduralMap>();
    if (!Map) { World->DestroyWorld(false); return false; }
    FCollisionQueryParams Query(SCENE_QUERY_STAT(ERProceduralTest),false);
    const auto Capsule=FCollisionShape::MakeCapsule(34,92.5f);
    FHitResult Hit;
    for (int32 Seed : {42,1,7919})
    {
        Map->LayoutConfig.Seed=Seed; Map->LayoutConfig.RoomCount=Seed==42 ? 10 : 6;
        TestTrue(TEXT("Render and collision generation"),Map->Regenerate());
        TestTrue(TEXT("Initial identity mapping"),Map->ValidateMappings());
        const auto& Layout=Map->GetLayout();
        TestFalse(TEXT("Spawn capsule is free"),World->OverlapBlockingTestByChannel(Map->GetSpawnLocation(),FQuat::Identity,ECC_Pawn,Capsule,Query));
        for (const auto& Route:Layout.Routes)
        {
            for (int32 I=1;I<Route.Num();++I)
                TestFalse(TEXT("Room route passes both doors"),World->SweepSingleByChannel(Hit,Route[I-1],Route[I],FQuat::Identity,ECC_Pawn,Capsule,Query));
            TestFalse(TEXT("Hall connects every branch"),World->SweepSingleByChannel(Hit,Route.Last(),FVector(0,0,100),FQuat::Identity,ECC_Pawn,Capsule,Query));
        }
        for (const auto& O:Layout.Openings)
        {
            const auto& Wall=Layout.Walls[O.Wall];
            const FVector Center=Wall.Start+Wall.Direction*(O.Along+O.Width*.5f)+FVector(0,0,O.Sill+O.Height*.5f);
            const FVector Normal=FVector::CrossProduct(Wall.Direction,FVector::UpVector);
            TestFalse(TEXT("Aperture contains no collision"),World->LineTraceSingleByChannel(Hit,Center+Normal*40,Center-Normal*40,ECC_GameTraceChannel1,Query));
        }
        FRandomStream Random(Seed);
        for (int32 I=0;I<120;++I)
        {
            const auto& R=Map->BlockWorld->GetBlocks()[Random.RandRange(0,Map->BlockWorld->GetBlocks().Num()-1)];
            if (R.State==EERBlockState::Destroyed) continue;
            const FVector P=R.LocalTransform.GetLocation(), N=R.LocalTransform.GetRotation().GetAxisY();
            FERBlockId Resolved;
            // At a corner/T-junction one outward start can be inside the
            // perpendicular neighbour. Trace the opposite exposed face then;
            // never ignore the actor/component containing either logical block.
            auto TraceFace=[&](float Side)
            {
                return World->LineTraceSingleByChannel(Hit,P+N*11*Side,P-N*11*Side,ECC_GameTraceChannel1,Query) &&
                    Map->ResolveHit(Hit,Resolved) && Resolved==R.Id;
            };
            TestTrue(TEXT("Exact identity from an exposed face"),TraceFace(1) || TraceFace(-1));
            const FERBlockId Id=R.Id;
            TestTrue(TEXT("Shared destruction removes map block"),Map->ApplyBlockDamage(Id,0,true));
            TestFalse(TEXT("Removed collision absent"),World->LineTraceSingleByChannel(Hit,P+N*9,P-N*9,ECC_GameTraceChannel1,Query));
        }
        TestTrue(TEXT("Mappings after random removals"),Map->ValidateMappings());
        const int32 Components=Map->GetInstanceComponents().Num();
        for (int32 I=0;I<3;++I)
        {
            TestTrue(TEXT("Regenerate"),Map->Regenerate());
            TestEqual(TEXT("No duplicated components"),Map->GetInstanceComponents().Num(),Components);
            TestTrue(TEXT("Regenerated identity"),Map->ValidateMappings());
        }
        TestTrue(TEXT("Floor still supports spawn"),World->LineTraceSingleByChannel(Hit,Map->GetSpawnLocation(),Map->GetSpawnLocation()-FVector(0,0,150),ECC_WorldStatic,Query));
        FERProceduralConfig Previous=Map->LayoutConfig;Map->LayoutConfig.RoomCount=11;
        const int32 Instances=Map->GetInstanceCount();TestFalse(TEXT("Bad regeneration rejected"),Map->Regenerate());
        TestEqual(TEXT("Live instances preserved"),Map->GetInstanceCount(),Instances);Map->LayoutConfig=Previous;
    }
    World->DestroyWorld(false); return true;
}
#endif
