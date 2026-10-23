#include "ERProceduralMap.h"
#include "ERMapCatalog.h"
#include "ERLadder.h"
#include "ERCharacter.h"
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Materials/Material.h"
#include "Components/InstancedStaticMeshComponent.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace ERProcedural { bool BuildTemplate(const FERProceduralConfig&,const FERBlockRoomConfig&,FERProceduralLayout&,const FERCellSettings&); }
namespace
{
UERMapTemplate* GreyboxFireTraining(const TCHAR* Id)
{
    auto* T=NewObject<UERMapTemplate>();T->TemplateId=Id;T->Archetype=EERMapArchetype::FireTraining;T->Symmetry=EERTemplateSymmetry::HalfTurn;T->ArenaSize=FVector(2460,5540,1120);
    for(int32 Width:{1600,2000,2400}){FERWarehouseWidthKit K;K.WidthCm=Width;T->FireTraining.Widths.Add(K);}
    T->FireTraining.SteelSurface=NewObject<UMaterial>(T,TEXT("TestSteel"));
    for(int32 I=0;I<3;++I)T->FireTraining.ContainerSurfaces.Add(NewObject<UMaterial>(T,*FString::Printf(TEXT("TestPaint%d"),I)));
    return T;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FERFireTrainingLayoutTest,"EclipseRecoil.Destruction.FireTrainingLayout",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FERFireTrainingLayoutTest::RunTest(const FString& Parameters)
{
    auto* T=GreyboxFireTraining(TEXT("test.fire-training"));
    FERProceduralConfig C;C.TemplateAsset=T;FERBlockRoomConfig B;FERProceduralLayout L,Again;
    TSet<int32> Widths,Bands,Bays,Attempts,Naves,TotalWidths;int32 Plazas=0;float MaxArea=0,MaxLength=0;TMap<FName,int32> Features;TSet<FString> Shapes;
    float MaxElevated=0,MaxOverhead=0,MinOpen=1,SumElevated=0;
    auto Key=[](FVector Center,FVector Size){return FString::Printf(TEXT("%d:%d:%d/%d:%d:%d"),FMath::RoundToInt(Center.X*100),FMath::RoundToInt(Center.Y*100),FMath::RoundToInt(Center.Z*100),FMath::RoundToInt(Size.X*100),FMath::RoundToInt(Size.Y*100),FMath::RoundToInt(Size.Z*100));};
    TArray<int32> Seeds;for(int32 I=0;I<512;++I)Seeds.Add(I);Seeds.Append({7919,-17,INT32_MIN,INT32_MAX});
    for(int32 Seed:Seeds)
    {
        C.Seed=Seed;
        if(!ERProcedural::Build(C,B,L)){AddError(FString::Printf(TEXT("Fire Training rejected seed=%d"),Seed));continue;}
        Widths.Add(FMath::RoundToInt(L.WarehouseNaveWidth));Naves.Add(L.WarehouseNaves);TotalWidths.Add(FMath::RoundToInt(L.WarehouseWidth));
        TestEqual(TEXT("total width is whole naves"),L.WarehouseWidth,L.WarehouseNaveWidth*L.WarehouseNaves);
        TestTrue(TEXT("no tube-shaped hall"),420.f*L.WarehouseBays<=L.WarehouseWidth*T->FireTraining.MaxAspect+1);
        MaxArea=FMath::Max(MaxArea,L.WarehouseWidth*420*L.WarehouseBays/1e4f);MaxLength=FMath::Max(MaxLength,420.f*L.WarehouseBays);
        Plazas+=L.Plazas.Num()/2;
        for(const auto& P:L.Plazas)for(const auto& Box:L.Containers)
            TestFalse(TEXT("plazas stay empty of cargo"),FBox(-Box.Size*.5,Box.Size*.5).TransformBy(Box.Frame).Intersect(P.ExpandBy(-1)));
        Bands.Add(L.WarehouseBands);Bays.Add(L.WarehouseBays);Attempts.Add(L.LayoutAttempt);
        Shapes.Add(FString::Printf(TEXT("%d/%d/%d"),FMath::RoundToInt(L.WarehouseWidth),L.WarehouseBays,L.WarehouseBands));
        for(const FName& F:L.FeatureNames)Features.FindOrAdd(F)++;
        const float Area=L.WarehouseWidth*420*L.WarehouseBays/1e4f;
        if(L.LayoutAttempt<ERProcedural::FireTrainingAttempts-1)
            TestTrue(TEXT("similar floor area across width/length variants"),Area>=T->FireTraining.FloorArea.X && Area<=T->FireTraining.FloorArea.Y);
        TestEqual(TEXT("eave follows wall bands"),L.IndoorCeiling,300.f*L.WarehouseBands);
        TestEqual(TEXT("10 starts per team"),L.AlphaSpawns.Num(),10);
        for(int32 I=0;I<L.AlphaSpawns.Num();++I){auto P=L.AlphaSpawns[I].GetLocation();P.X=-P.X;P.Y=-P.Y;TestTrue(TEXT("half-turn starts"),P.Equals(L.OmegaSpawns[I].GetLocation()));}
        // One open volume: local decks only, inside every budget, never a mezzanine.
        TestTrue(TEXT("elevated projection within budget"),L.ElevatedRatio<=T->FireTraining.ElevatedMaxRatio+1e-3f);
        TestTrue(TEXT("overhead closure within budget"),L.OverheadRatio<=T->FireTraining.OverheadMaxRatio+1e-3f);
        TestTrue(TEXT("large connected region open to the roof"),L.OpenRegionRatio>=T->FireTraining.OpenRegionMinRatio);
        MaxElevated=FMath::Max(MaxElevated,L.ElevatedRatio);MaxOverhead=FMath::Max(MaxOverhead,L.OverheadRatio);MinOpen=FMath::Min(MinOpen,L.OpenRegionRatio);SumElevated+=L.ElevatedRatio;
        const float Length=420.f*L.WarehouseBays;
        for(const auto& D:L.ElevatedDecks)
        {
            TestTrue(TEXT("decks are local, not a continuous floor"),D.GetSize().X<L.WarehouseWidth*.5f || D.GetSize().Y<Length*.15f);
            TestTrue(TEXT("deck headroom under the arched roof"),ERProcedural::WarehouseRoofHeight(L.WarehouseNaveWidth,L.WarehouseNaves,L.IndoorCeiling,D.Min.X)>=D.Max.Z+250 && ERProcedural::WarehouseRoofHeight(L.WarehouseNaveWidth,L.WarehouseNaves,L.IndoorCeiling,D.Max.X)>=D.Max.Z+250);
        }
        for(int32 A:L.ElevatedAccesses)TestTrue(TEXT("two independent accesses per elevated area"),A>=2);
        for(int32 I=0;I<L.Ladders.Num();I+=2)TestTrue(TEXT("ladders are half-turn pairs"),(L.Ladders[I].Frame*FTransform(FRotator(0,180,0))).Equals(L.Ladders[I+1].Frame));
        TestTrue(TEXT("most start-to-start lines meet cover"),L.SpawnCoverRatio>=.75f);
        TSet<FString> Solids;
        for(const auto& P:L.Panels){const auto Box=FBox(FVector::ZeroVector,P.Size).TransformBy(P.Transform);Solids.Add(Key(Box.GetCenter(),Box.GetSize()));}
        for(const auto& P:L.Panels){const auto Box=FBox(FVector::ZeroVector,P.Size).TransformBy(P.Transform);auto V=Box.GetCenter();V.X=-V.X;V.Y=-V.Y;if(!Solids.Contains(Key(V,Box.GetSize()))){AddError(TEXT("unpaired cell geometry"));return false;}}
        TestTrue(TEXT("timber construction is destructible"),L.Panels.ContainsByPredicate([](const auto& P){return P.Material==EERBlockMaterial::Wood;}));
        for(const auto& P:L.Panels)if(P.Strength==8)TestTrue(TEXT("containers use the cargo palette"),T->FireTraining.ContainerSurfaces.Contains(P.Surface));
        // Bands raise only the envelope: no walkable fixed slab spans the hall.
        for(const auto& P:L.FixedPieces)
        {
            const FVector S=P.Transform.GetScale3D()*100;
            TestFalse(TEXT("no automatic upper floor"),P.Transform.GetLocation().Z>100 && S.Z<=25 && S.X*S.Y>L.WarehouseWidth*Length*.25f);
        }
    }
    AddInfo(FString::Printf(TEXT("ER_FIRE_TRAINING_LAYOUT seeds=%d shapes=%d widths=%d naves=%d total_widths=%d bands=%d bays=%d attempts=%d plazas=%d max_area=%.0f max_length=%.0f max_elevated=%.3f mean_elevated=%.3f max_overhead=%.3f min_open=%.3f"),
        Seeds.Num(),Shapes.Num(),Widths.Num(),Naves.Num(),TotalWidths.Num(),Bands.Num(),Bays.Num(),Attempts.Num(),Plazas,MaxArea,MaxLength,MaxElevated,SumElevated/Seeds.Num(),MaxOverhead,MinOpen));
    for(const auto& F:Features)AddInfo(FString::Printf(TEXT("ER_FIRE_TRAINING_FEATURE %s=%d"),*F.Key.ToString(),F.Value));
    TestEqual(TEXT("three kit widths sampled"),Widths.Num(),3);
    TestEqual(TEXT("two to four wall bands sampled"),Bands.Num(),3);
    TestEqual(TEXT("one to four naves sampled"),Naves.Num(),4);
    TestTrue(TEXT("large halls reserve empty plazas"),Plazas>50);
    TestTrue(TEXT("lengths vary with the width"),Bays.Num()>=5);
    for(const TCHAR* Name:{TEXT("Catwalk"),TEXT("CatwalkBridge"),TEXT("Corridor"),TEXT("ScreenedCatwalk"),TEXT("RoomDeck"),TEXT("Lookout"),TEXT("Container"),TEXT("ContainerStack")})
        TestTrue(FString::Printf(TEXT("feature sampled: %s"),Name),Features.Contains(FName(Name)));
    C.Seed=42;TestTrue(TEXT("first deterministic build"),ERProcedural::Build(C,B,L));TestTrue(TEXT("repeat"),ERProcedural::Build(C,B,Again));
    TestEqual(TEXT("panel count stable"),L.Panels.Num(),Again.Panels.Num());TestEqual(TEXT("attempt stable"),L.LayoutAttempt,Again.LayoutAttempt);
    for(int32 I=0;I<L.Panels.Num() && I<Again.Panels.Num();++I)TestTrue(TEXT("panel identity stable"),L.Panels[I].Id==Again.Panels[I].Id && L.Panels[I].Transform.Equals(Again.Panels[I].Transform));
    // The final attempt is the verified fallback for every accepted recipe.
    C.LayoutAttempt=ERProcedural::FireTrainingAttempts-1;FERProceduralLayout Fallback;
    TestTrue(TEXT("reference layout builds"),ERProcedural::BuildTemplate(C,B,Fallback,FERCellSettings()));
    TestEqual(TEXT("reference layout is the narrow kit"),FMath::RoundToInt(Fallback.WarehouseWidth),1600);
    TestTrue(TEXT("reference layout keeps a catwalk"),Fallback.FeatureNames.Contains(FName(TEXT("Catwalk"))));
    C.LayoutAttempt=0;
    const int32 Count=L.Panels.Num();T->FireTraining.ContinuousUpperFloors=true;
    TestFalse(TEXT("continuous upper floors are rejected"),ERProcedural::Build(C,B,L));TestEqual(TEXT("rejection is atomic"),L.Panels.Num(),Count);
    T->FireTraining.ContinuousUpperFloors=false;T->FireTraining.Widths[1].RoofBay=NewObject<UStaticMesh>(T);
    TestFalse(TEXT("an incomplete width kit is never stretched or mixed"),ERProcedural::Build(C,B,L));
    TestTrue(TEXT("Fire Training belongs to the match rotation"),ERMaps::Rotation().Contains(FName(TEXT("/Game/EclipseRecoil/Maps/L_FireTraining"))));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FERFireTrainingPhysicsTest,"EclipseRecoil.Destruction.FireTrainingPhysics",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FERFireTrainingPhysicsTest::RunTest(const FString& Parameters)
{
    auto* Asset=LoadObject<UERMapTemplate>(nullptr,TEXT("/Game/EclipseRecoil/Art/MapTemplates/DA_FireTraining.DA_FireTraining"));
    if(!TestNotNull(TEXT("Fire Training template asset"),Asset))return false;
    TestEqual(TEXT("asset archetype"),Asset->Archetype,EERMapArchetype::FireTraining);
    TestEqual(TEXT("complete kit for three widths"),Asset->FireTraining.Widths.Num(),3);
    UWorld::InitializationValues V;V.AllowAudioPlayback(false).CreateNavigation(false).CreateAISystem(false).CreateFXSystem(false).EnableTraceCollision(true);
    auto* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&V);if(!World)return false;
    auto* Map=World->SpawnActor<AERProceduralMap>();Map->LayoutConfig.TemplateAsset=Asset;Map->bCeilings=true;
    auto* Pawn=World->SpawnActor<AERCharacter>(FVector(-20000,0,100),FRotator::ZeroRotator);
    FCollisionQueryParams Q(SCENE_QUERY_STAT(ERFireTrainingTest),false);Q.AddIgnoredActor(Pawn);FHitResult Hit;
    const auto Capsule=FCollisionShape::MakeCapsule(34,92.5);
    TSet<int32> Widths;
    for(int32 Seed:{3,42,73,511,7919,1001,2024,90210})
    {
        Map->LayoutConfig.Seed=Seed;
        if(!TestTrue(FString::Printf(TEXT("physical generation seed=%d"),Seed),Map->Regenerate()))continue;
        const auto& L=Map->GetLayout();TestTrue(TEXT("valid cell chunks"),Map->ValidateCells());Widths.Add(FMath::RoundToInt(L.WarehouseWidth));
        // Kit modules keep unit scale; width/length/height come from repetition.
        TInlineComponentArray<UInstancedStaticMeshComponent*> Groups(Map);int32 KitInstances=0;
        for(auto* Group:Groups)if(Group->GetStaticMesh() && Group->GetStaticMesh()->GetName().StartsWith(TEXT("SM_ER_WH_")))
            for(int32 I=0;I<Group->GetInstanceCount();++I)
            {
                FTransform X;Group->GetInstanceTransform(I,X,false);++KitInstances;
                TestTrue(TEXT("kit instance at unit scale"),X.GetScale3D().Equals(FVector::OneVector,1e-3));
            }
        TestTrue(TEXT("envelope assembled from repeated modules"),KitInstances>L.WarehouseBays*4);
        const float HalfW=L.WarehouseWidth*.5f,HL=210.f*L.WarehouseBays;
        // The envelope closes on every side, up to the eave, with real collision.
        for(float Z:{100.f,L.IndoorCeiling-60})for(float Y:{-HL*.5f,0.f,HL*.5f})
        {
            TestTrue(TEXT("side wall blocks pawns"),World->SweepSingleByChannel(Hit,{HalfW-100,Y,Z},{HalfW+100,Y,Z},FQuat::Identity,ECC_Pawn,FCollisionShape::MakeSphere(20),Q));
            TestTrue(TEXT("side wall stops bullets"),World->LineTraceSingleByChannel(Hit,{-HalfW+100,-Y,Z},{-HalfW-100,-Y,Z},ECC_GameTraceChannel1,Q));
        }
        TestTrue(TEXT("end wall blocks pawns"),World->SweepSingleByChannel(Hit,{0,-HL+100,150},{0,-HL-100,150},FQuat::Identity,ECC_Pawn,FCollisionShape::MakeSphere(20),Q));
        TestTrue(TEXT("roof stops shots below the ridge"),World->LineTraceSingleByChannel(Hit,{0,0,L.IndoorCeiling-50},{0,0,L.IndoorCeiling+400},ECC_GameTraceChannel1,Q) && Hit.ImpactPoint.Z<L.IndoorCeiling+230);
        // No mezzanine: probe the plan for walkable surfaces above the floor.
        int32 Raised=0,Samples=0;
        for(float X=-HalfW+50;X<HalfW;X+=100)for(float Y=-HL+50;Y<HL;Y+=100)
        {
            ++Samples;
            if(World->LineTraceSingleByChannel(Hit,{X,Y,L.IndoorCeiling-30},{X,Y,-50},ECC_WorldStatic,Q) && Hit.ImpactPoint.Z>150)++Raised;
        }
        TestTrue(FString::Printf(TEXT("open hall: raised coverage %.2f"),float(Raised)/Samples),float(Raised)/Samples<=Asset->FireTraining.OverheadMaxRatio+.05f);
        for(const auto& Desc:L.Ladders)
        {
            auto* Ladder=World->SpawnActor<AERLadder>(AERLadder::StaticClass(),Desc.Frame);
            Ladder->Height=Desc.Height;Ladder->DeckHeight=Desc.DeckHeight;
            // Ladder heights are local to the actor, whose base may be a deck.
            for(float Z=0;Z<=Desc.DeckHeight;Z+=25)
            {
                TestTrue(TEXT("real destructible rungs cover climb"),Ladder->HasRung(Z));
                TestFalse(FString::Printf(TEXT("climbing capsule clear seed=%d ladder=%s z=%.0f"),Seed,*Desc.Frame.GetLocation().ToString(),Z),World->OverlapBlockingTestByChannel(Ladder->ClimbPosition(Z+2.5f,92.5f),FQuat::Identity,ECC_Pawn,Capsule,Q));
            }
            const FVector Exit=Ladder->Landing(92.5f);
            TestFalse(TEXT("ladder landing clears standing capsule"),World->OverlapBlockingTestByChannel(Exit,FQuat::Identity,ECC_Pawn,Capsule,Q));
            TestTrue(TEXT("ladder landing has a real floor"),World->LineTraceSingleByChannel(Hit,Exit,Exit-FVector(0,0,120),ECC_WorldStatic,Q));
            Ladder->Destroy();
        }
        for(const auto* Starts:{&L.AlphaSpawns,&L.OmegaSpawns})for(const auto& S:*Starts)
        {TestFalse(TEXT("spawn capsule clear"),World->OverlapBlockingTestByChannel(S.GetLocation(),FQuat::Identity,ECC_Pawn,Capsule,Q));TestTrue(TEXT("spawn supported"),World->LineTraceSingleByChannel(Hit,S.GetLocation(),S.GetLocation()-FVector(0,0,120),ECC_WorldStatic,Q));}
        for(const auto* Paths:{&L.CrossingRoutes,&L.RoomAccessRoutes})for(const auto& Route:*Paths)for(int32 I=1;I<Route.Num();++I)
            TestFalse(FString::Printf(TEXT("route standing clearance seed=%d %s->%s"),Seed,*Route[I-1].ToString(),*Route[I].ToString()),World->SweepSingleByChannel(Hit,Route[I-1],Route[I],FQuat::Identity,ECC_Pawn,Capsule,Q));
        for(const auto& Route:L.StairRoutes)for(int32 I=1;I<Route.Num();++I)
            TestTrue(TEXT("every stair tread has solid support"),World->LineTraceSingleByChannel(Hit,Route[I],Route[I]-FVector(0,0,145),ECC_WorldStatic,Q));
        const auto& R=L.Routes[1];
        for(int32 I=0;I<R.Num();++I)
            TestFalse(TEXT("patrol loop clear"),World->SweepSingleByChannel(Hit,R[I],R[(I+1)%R.Num()],FQuat::Identity,ECC_Pawn,Capsule,Q));
        // Timber rooms are real destructible cells: shoot the wall beside the door.
        bool Timber=false;
        for(const auto& Route:L.RoomAccessRoutes)if(Route.Num()==4 && Route[0].Z<150 && Route[2].X>Route[1].X)
        {
            const FVector From=Route[1]+FVector(0,150,-50);
            if(World->LineTraceSingleByChannel(Hit,From,From+FVector(120,0,0),ECC_GameTraceChannel1,Q)){FERCellId Id;Timber|=Map->ResolveCell(Hit,Id);}
        }
        TestTrue(TEXT("timber walls resolve to destructible cells"),Timber);
    }
    TestTrue(TEXT("physical seeds cover several widths"),Widths.Num()>=2);
    World->DestroyWorld(false);return true;
}
#endif
