#include "ERProceduralMap.h"
#include "ERBlockCombat.h"
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FERExpansionLayoutTest,"EclipseRecoil.Destruction.MultiFloorLayout",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FERExpansionLayoutTest::RunTest(const FString& Parameters)
{
    FERBlockRoomConfig B;FERProceduralConfig C;C.RoomCount=6;C.AdditionalMaterials=true;C.WindowGlass=true;
    for(int32 Floors:{2,3})for(int32 Seed:{42,7919,INT32_MAX})
    {
        C.Floors=Floors;C.Seed=Seed;FERProceduralLayout A,Again;
        TestTrue(TEXT("multifloor build"),ERProcedural::Build(C,B,A));TestTrue(TEXT("repeat"),ERProcedural::Build(C,B,Again));
        TestEqual(TEXT("rooms on every floor"),A.Rooms.Num(),Floors*6);TestEqual(TEXT("stair connections"),A.StairRoutes.Num(),Floors-1);
        TestEqual(TEXT("equal panel count"),A.Panels.Num(),Again.Panels.Num());TSet<int32> IDs;TSet<EERBlockMaterial> Materials;
        for(int32 I=0;I<A.Panels.Num();++I)
        {
            const auto& P=A.Panels[I];TestFalse(TEXT("unique ID across floors/panes"),IDs.Contains(P.Id));IDs.Add(P.Id);Materials.Add(P.Material);
            TestTrue(TEXT("stable generated data"),P.Id==Again.Panels[I].Id && P.Transform.Equals(Again.Panels[I].Transform) && P.Size==Again.Panels[I].Size);
            TestTrue(TEXT("every wall and pane rests on its slab or frame"),P.bAnchorBottom);
        }
        TestEqual(TEXT("all seven materials"),Materials.Num(),7);
        FERCellWorld Cells;FString Error;
        TestTrue(TEXT("multifloor cells within budget"),Cells.Build(A.Panels,FERCellSettings(),GetDefault<UERBlockMaterialCatalog>(),Error));
        AddInfo(FString::Printf(TEXT("ER_MULTIFLOOR_CELLS floors=%d seed=%d panels=%d chunks=%d cells=%lld"),Floors,Seed,Cells.GetPanels().Num(),Cells.GetChunks().Num(),Cells.GetCellCount()));
        TArray<FERLayoutRect> Slabs=A.Rooms;Slabs.Append(A.Corridors);
        for(int32 I=0;I<Slabs.Num();++I)for(int32 J=I+1;J<Slabs.Num();++J)
        {
            const auto& X=Slabs[I];const auto& Y=Slabs[J];if(X.Z!=Y.Z)continue;
            TestFalse(TEXT("no overlapping coplanar slabs"),FMath::Min(X.Max.X,Y.Max.X)>FMath::Max(X.Min.X,Y.Min.X)+.01 && FMath::Min(X.Max.Y,Y.Max.Y)>FMath::Max(X.Min.Y,Y.Min.Y)+.01);
        }
    }
    C.Floors=4;FERProceduralLayout Invalid;TestFalse(TEXT("floor cap"),ERProcedural::Build(C,B,Invalid));
    auto* Catalog=NewObject<UERBlockMaterialCatalog>();TestTrue(TEXT("extended catalog"),Catalog->IsValidCatalog());
    Catalog->Materials[3].MaxHealth=-1;TestFalse(TEXT("invalid extra profile rejected"),Catalog->IsValidCatalog());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FERExpansionCollisionTest,"EclipseRecoil.Destruction.MultiFloorCollision",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FERExpansionCollisionTest::RunTest(const FString& Parameters)
{
    UWorld::InitializationValues V;V.AllowAudioPlayback(false).CreateNavigation(false).CreateAISystem(false).CreateFXSystem(false).EnableTraceCollision(true);
    auto* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&V);if(!World)return false;
    auto* Map=World->SpawnActor<AERProceduralMap>();Map->LayoutConfig.Floors=3;Map->LayoutConfig.RoomCount=6;Map->LayoutConfig.AdditionalMaterials=true;Map->LayoutConfig.WindowGlass=true;
    TestTrue(TEXT("three-story physical map"),Map->Regenerate());
    FCollisionQueryParams Q(SCENE_QUERY_STAT(ERExpansion),false);FHitResult Hit;const auto Capsule=FCollisionShape::MakeCapsule(34,92.5);
    for(const auto& Route:Map->GetLayout().StairRoutes)
    {
        for(auto P:Route)
        {
            if(World->OverlapBlockingTestByChannel(P+FVector(0,0,20),FQuat::Identity,ECC_Pawn,Capsule,Q))AddError(FString::Printf(TEXT("Stair capsule/headroom blocked at %s"),*P.ToString()));
            TestTrue(TEXT("route supported by floor/stair"),World->LineTraceSingleByChannel(Hit,P,P-FVector(0,0,130),ECC_WorldStatic,Q));
            TestTrue(TEXT("support near route feet"),Hit.ImpactPoint.Z>P.Z-125);
        }
        // Raised sweeps between tread centres model step-up headroom, then
        // genuine character movement is covered by the saved-map/LAN probes.
        for(int32 I=1;I<Route.Num();++I)
        {
            FVector A=Route[I-1],B=Route[I];const double Z=FMath::Max(A.Z,B.Z)+20;A.Z=B.Z=Z;
            if(World->SweepSingleByChannel(Hit,A,B,FQuat::Identity,ECC_Pawn,Capsule,Q))AddError(FString::Printf(TEXT("Stair step-up clearance %s -> %s hit %s at %s"),*A.ToString(),*B.ToString(),*GetNameSafe(Hit.GetComponent()),*Hit.ImpactPoint.ToString()));
        }
    }
    const auto& Cells=Map->GetCellWorld();
    for(auto Material:{EERBlockMaterial::Wood,EERBlockMaterial::Stone,EERBlockMaterial::Metal,EERBlockMaterial::Glass})
    {
        const int32 Panel=Cells.GetPanels().IndexOfByPredicate([&](const auto& P){return P.Material==Material && (P.bPane || (P.Dims.X>8 && P.Dims.Z>8));});
        TestTrue(TEXT("new material instantiated"),Panel!=INDEX_NONE);if(Panel==INDEX_NONE)continue;
        const auto& P=Cells.GetPanels()[Panel];
        // An exposed outer-face cell (openings are absent cells; neighbours may abut a face).
        const FVector N=-P.Transform.GetRotation().GetAxisY();
        FERCellId Id{Panel,0,0,0},Found;bool bHit=false;
        for(int32 Z=FMath::Min(2,P.Dims.Z-1);Z<P.Dims.Z && !bHit;Z+=3)for(int32 X=P.Dims.X/2;X<P.Dims.X && !bHit;X+=3)
        {
            Id={Panel,X,0,Z};if(!Cells.IsAlive(Id))continue;
            const FVector C=Cells.GetCellCenter(Id);
            bHit=World->LineTraceSingleByChannel(Hit,C+N*12,C,ECC_GameTraceChannel1,Q)&&Map->ResolveCell(Hit,Found)&&Found==Id;
        }
        TestTrue(TEXT("new material physical hit and identity"),bHit);
        TMap<FERCellId,float> Damage;Damage.Add(Id,1);
        TestTrue(TEXT("new material can take damage"),Map->ApplyCellBatch(Damage).Damaged==1);
        TestTrue(TEXT("profile health exact"),Cells.GetHealth(Id)==Map->MaterialCatalog->Find(Material)->MaxHealth-1);
    }
    Map->RequestSupportCheck();int32 Steps=0;while(Map->IsSupportPending() && Steps++<10000)Map->StepSupport();
    TestEqual(TEXT("upper floors have structural footing"),Cells.GetDetachedCount(),int64(0));
    TestTrue(TEXT("multifloor chunks"),Map->ValidateCells());World->DestroyWorld(false);return true;
}
#endif
