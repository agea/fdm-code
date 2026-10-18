#include "ERProceduralMap.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
// Panels may touch but never overlap (crate boards, which do, are template-only).
bool PanelsOverlap(const TArray<FERCellPanelDesc>& Panels, FString& Pair)
{
    TArray<FBox> Boxes;
    for (const auto& P : Panels) Boxes.Add(FBox(FVector::ZeroVector, P.Size).TransformBy(P.Transform));
    for (int32 A = 0; A < Boxes.Num(); ++A)
        for (int32 B = A + 1; B < Boxes.Num(); ++B)
            if (Boxes[A].Intersect(Boxes[B]) && Boxes[A].Overlap(Boxes[B]).GetSize().GetMin() > .001)
            { Pair = FString::Printf(TEXT("%d / %d"), Panels[A].Id, Panels[B].Id); return true; }
    return false;
}
}
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
            TestEqual(TEXT("Stable panel count"),A.Panels.Num(),D.Panels.Num());
            TSet<int32> Ids;
            for (int32 I=0;I<A.Panels.Num();++I)
            {
                const auto& P=A.Panels[I];
                if (Ids.Contains(P.Id)) { AddError(TEXT("Duplicate panel identity")); return false; } Ids.Add(P.Id);
                const auto& Q=D.Panels[I];
                if (P.Id!=Q.Id || P.Material!=Q.Material || !P.Transform.Equals(Q.Transform) || P.Size!=Q.Size || P.Holes.Num()!=Q.Holes.Num()) { AddError(TEXT("Non-deterministic panels")); return false; }
                if (!P.bAnchorBottom) { AddError(TEXT("Wall panel without footing")); return false; }
            }
            FString Pair;
            if (PanelsOverlap(A.Panels,Pair)) { AddError(TEXT("Overlapping panels ")+Pair); return false; }
            FERCellWorld World; FString Error;
            TestTrue(TEXT("Layout builds 5cm cells"),World.Build(A.Panels,FERCellSettings(),GetDefault<UERBlockMaterialCatalog>(),Error));
            TestTrue(TEXT("Openings are absent cells, not destroyed ones"),World.GetVoidCount()>0 && World.GetRemovedCount()==0);
            ++C.Seed; TestTrue(TEXT("Different seed"),ERProcedural::Build(C,B,Other));
            bool Different=A.Panels.Num()!=Other.Panels.Num();
            for (int32 I=0; !Different && I<A.Panels.Num(); ++I) Different=!A.Panels[I].Transform.Equals(Other.Panels[I].Transform) || A.Panels[I].Material!=Other.Panels[I].Material || A.Panels[I].Size!=Other.Panels[I].Size;
            TestTrue(TEXT("Seed changes geometry/materials"),Different);
            TestEqual(TEXT("Every room has a hall route"),A.Routes.Num(),Count);
        }
    for (int32 Width : {100,200})
    {
        FERProceduralConfig Edge; Edge.MinRoomSize=Edge.MaxRoomSize=400;Edge.CorridorWidth=150;Edge.BranchLength=150;Edge.DoorWidth=Width;
        FERProceduralLayout L;TestTrue(TEXT("Minimum room/branch and supported door widths"),ERProcedural::Build(Edge,B,L));
    }
    FERProceduralConfig C; FERProceduralLayout Layout;
    TestTrue(TEXT("Initial state"),ERProcedural::Build(C,B,Layout)); const int32 Count=Layout.Panels.Num();
    C.RoomCount=100000; TestFalse(TEXT("Reject unbounded rooms"),ERProcedural::Build(C,B,Layout));
    TestEqual(TEXT("Invalid generation preserves layout"),Layout.Panels.Num(),Count);
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
        TestTrue(TEXT("Initial chunk meshes and collision"),Map->ValidateCells());
        const auto& Layout=Map->GetLayout();
        const auto& Cells=Map->GetCellWorld();
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
        // Random outer-layer cells away from panel ends: exact identity from the
        // exposed face, then the removed cell leaves no collision behind.
        FRandomStream Random(Seed);
        int32 Resolved=0, Tried=0;
        for (int32 I=0;I<160;++I)
        {
            const int32 P=Random.RandRange(0,Cells.GetPanels().Num()-1);
            const auto& Panel=Cells.GetPanels()[P];
            if (Panel.bPane || Panel.Dims.X<6 || Panel.Dims.Z<6) continue;
            const bool bFront=Random.RandBool();
            const FERCellId Id{P,Random.RandRange(2,Panel.Dims.X-3),bFront ? 0 : Panel.Dims.Y-1,Random.RandRange(2,Panel.Dims.Z-3)};
            if (!Cells.IsAlive(Id)) continue;
            ++Tried;
            const FVector N=Panel.Transform.GetRotation().GetAxisY()*(bFront ? -1 : 1);
            const FVector Center=Cells.GetCellCenter(Id), Face=Center+N*Panel.Cell.Y*.5;
            FERCellId Found;
            if (!World->LineTraceSingleByChannel(Hit,Face+N*8,Center,ECC_GameTraceChannel1,Query) || !Map->ResolveCell(Hit,Found) || !(Found==Id)) continue;
            ++Resolved;
            TMap<FERCellId,float> Remove; Remove.Add(Id,TNumericLimits<float>::Max());
            TestTrue(TEXT("Shared destruction removes the cell"),Map->ApplyCellBatch(Remove).Destroyed==1);
            TestFalse(TEXT("Removed cell has no collision"),World->LineTraceSingleByChannel(Hit,Face+N*.5,Center,ECC_GameTraceChannel1,Query));
        }
        TestTrue(FString::Printf(TEXT("Exact identity from exposed faces (%d/%d)"),Resolved,Tried),Tried>40 && Resolved>=Tried*8/10);
        TestTrue(TEXT("Chunks after random removals"),Map->ValidateCells());
        const int32 Components=Map->GetInstanceComponents().Num();
        for (int32 I=0;I<3;++I)
        {
            TestTrue(TEXT("Regenerate"),Map->Regenerate());
            TestEqual(TEXT("No duplicated components"),Map->GetInstanceComponents().Num(),Components);
            TestTrue(TEXT("Regenerated chunks"),Map->ValidateCells() && Cells.GetRemovedCount()==0);
        }
        TestTrue(TEXT("Floor still supports spawn"),World->LineTraceSingleByChannel(Hit,Map->GetSpawnLocation(),Map->GetSpawnLocation()-FVector(0,0,150),ECC_WorldStatic,Query));
        FERProceduralConfig Previous=Map->LayoutConfig;Map->LayoutConfig.RoomCount=11;
        const int64 Count=Cells.GetCellCount();TestFalse(TEXT("Bad regeneration rejected"),Map->Regenerate());
        TestEqual(TEXT("Live cells preserved"),Cells.GetCellCount(),Count);Map->LayoutConfig=Previous;
    }
    World->DestroyWorld(false); return true;
}
#endif
