#include "ERMapTemplate.h"
#include "ERProceduralMap.h"
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FERDustTemplateTest,"EclipseRecoil.Destruction.DustTemplate",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FERDustTemplateTest::RunTest(const FString& Parameters)
{
    auto* T=LoadObject<UERMapTemplate>(nullptr,TEXT("/Game/EclipseRecoil/Art/MapTemplates/DA_DustMirror.DA_DustMirror"));
    if(!TestNotNull(TEXT("Saved Dust town"),T))return false;
    TestTrue(TEXT("plan recipe enabled"),T->Town.Enabled);
    TestEqual(TEXT("reference plan asymmetry"),T->Symmetry,EERTemplateSymmetry::None);
    FERProceduralConfig C;C.TemplateAsset=T;FERBlockRoomConfig B;FERProceduralLayout L,Again;
    TSet<FString> Plans,Dimensions,Widths;int32 MinBuildings=MAX_int32,MaxBuildings=0;
    for(int32 Seed=0;Seed<512;++Seed)
    {
        C.Seed=Seed;
        if(!ERProcedural::Build(C,B,L)){AddError(FString::Printf(TEXT("Invalid town seed %d"),Seed));return false;}
        TestEqual(TEXT("ten Alpha starts"),L.AlphaSpawns.Num(),10);TestEqual(TEXT("ten Omega starts"),L.OmegaSpawns.Num(),10);
        TestEqual(TEXT("six reference courts"),L.TownCourts.Num(),6);TestEqual(TEXT("three narrow arched gates"),L.TownGates.Num(),3);
        TestEqual(TEXT("six ground routes"),L.RoomAccessRoutes.Num(),6);
        TestEqual(TEXT("two stair approaches"),L.StairRoutes.Num(),2);
        TestTrue(TEXT("compact town dimensions"),L.ArenaSize.X>=6600 && L.ArenaSize.X<=7500 && L.ArenaSize.Y>=7300 && L.ArenaSize.Y<=8400);
        FString Shape;
        double Area=0;for(const auto& Box:L.TownBuildings){Shape+=Box.Min.ToString()+Box.Max.ToString();Area+=Box.GetSize().X*Box.GetSize().Y;}
        Plans.Add(Shape);Dimensions.Add(L.ArenaSize.ToString());Widths.Add(FString::SanitizeFloat(L.TownGates[0].Width));
        MinBuildings=FMath::Min(MinBuildings,L.TownBuildings.Num());MaxBuildings=FMath::Max(MaxBuildings,L.TownBuildings.Num());
        TestTrue(TEXT("city occupies most of the plan"),Area/(L.ArenaSize.X*L.ArenaSize.Y)>.45);
        TestTrue(TEXT("bounded runtime art"),L.FixedPieces.Num()<=2048);
        for(const auto& Gate:L.TownGates){TestTrue(TEXT("narrow diagonal standing slot"),Gate.Gap>=110 && Gate.Gap<=130);TestTrue(TEXT("opposite swing angle"),Gate.Angle>=15 && Gate.Angle<=30);TestEqual(TEXT("dogleg through both leaves"),Gate.Passage.Num(),6);}
    }
    TestTrue(TEXT("seed changes city parcels, not just cover"),Plans.Num()>400);
    TestTrue(TEXT("many arena proportions"),Dimensions.Num()>100);TestTrue(TEXT("alleys vary in width"),Widths.Num()>=5);
    AddInfo(FString::Printf(TEXT("ER_DUST_TOWN_VARIATION seeds=512 plans=%d dimensions=%d buildings=%d..%d widths=%d"),Plans.Num(),Dimensions.Num(),MinBuildings,MaxBuildings,Widths.Num()));
    UWorld::InitializationValues V;V.AllowAudioPlayback(false).CreateNavigation(false).CreateAISystem(false).CreateFXSystem(false).EnableTraceCollision(true);
    auto* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&V);
    if(!TestNotNull(TEXT("physics world"),World))return false;
    auto* Map=World->SpawnActor<AERProceduralMap>();Map->bCeilings=false;
    FCollisionQueryParams Q(SCENE_QUERY_STAT(ERDustTownTest),false);FHitResult Hit;const auto Capsule=FCollisionShape::MakeCapsule(34,92.5);
    for(int32 Seed:{0,1,42,7919,-17,INT32_MAX})
    {
        C.Seed=Seed;Map->LayoutConfig=C;
        if(!TestTrue(TEXT("physical town generation"),Map->Regenerate()))continue;
        ERProcedural::Build(C,B,L);ERProcedural::Build(C,B,Again);
        TestTrue(TEXT("valid mappings"),Map->ValidateCells());TestEqual(TEXT("deterministic geometry"),L.FixedPieces.Num(),Again.FixedPieces.Num());
        for(int32 I=0;I<L.FixedPieces.Num();++I)TestTrue(TEXT("repeat seed gives identical city"),L.FixedPieces[I].Transform.Equals(Again.FixedPieces[I].Transform));
        const FVector P=L.TownCrossing;
        TestFalse(TEXT("lower CT route stays open under Short A"),World->SweepSingleByChannel(Hit,P+FVector(-600,0,100),P+FVector(600,0,100),FQuat::Identity,ECC_Pawn,Capsule,Q));
        TestFalse(TEXT("upper route crosses over CT"),World->SweepSingleByChannel(Hit,P+FVector(0,-400,420),P+FVector(0,400,420),FQuat::Identity,ECC_Pawn,Capsule,Q));
        TestTrue(TEXT("bridge top at 320cm"),World->LineTraceSingleByChannel(Hit,P+FVector(0,0,500),P+FVector(0,0,200),ECC_WorldStatic,Q) && FMath::IsNearlyEqual(Hit.ImpactPoint.Z,320.f,1));
        TestTrue(TEXT("independent lower floor"),World->LineTraceSingleByChannel(Hit,P+FVector(0,0,200),P-FVector(0,0,50),ECC_WorldStatic,Q) && FMath::IsNearlyZero(Hit.ImpactPoint.Z,1));
        for(const auto& Path:L.RoomAccessRoutes)for(int32 I=1;I<Path.Num();++I)
            TestFalse(TEXT("all reference ground streets remain connected"),World->SweepSingleByChannel(Hit,Path[I-1],Path[I],FQuat::Identity,ECC_Pawn,Capsule,Q));
        for(const auto& Gate:L.TownGates)
        {
            for(int32 I=1;I<Gate.Passage.Num();++I)
            {
                TestFalse(TEXT("standing player follows diagonal slot"),World->SweepSingleByChannel(Hit,Gate.Passage[I-1],Gate.Passage[I],FQuat::Identity,ECC_Pawn,Capsule,Q));
                TestFalse(TEXT("diagonal slot works in reverse"),World->SweepSingleByChannel(Hit,Gate.Passage[I],Gate.Passage[I-1],FQuat::Identity,ECC_Pawn,Capsule,Q));
            }
            TestTrue(TEXT("straight frontal movement blocked"),World->SweepSingleByChannel(Hit,Gate.Passage[0],Gate.Passage.Last(),FQuat::Identity,ECC_Pawn,Capsule,Q));
            for(int32 Sample=-4;Sample<=4;++Sample)
            {
                const float X=Sample*(Gate.Width-80)*.1f;
                const FVector A=Gate.Frame.TransformPosition(FVector(X,-300,170)),BPoint=Gate.Frame.TransformPosition(FVector(X,300,170));
                TestTrue(TEXT("no frontal sightline through opposite leaves"),World->LineTraceSingleByChannel(Hit,A,BPoint,ECC_Visibility,Q));
                TestTrue(TEXT("no reverse frontal sightline"),World->LineTraceSingleByChannel(Hit,BPoint,A,ECC_Visibility,Q));
            }
        }
        for(const auto& Path:L.StairRoutes)for(int32 I=0;I<Path.Num();++I)
        {
            const FVector S=Path[I]+FVector(0,0,I>0 && I<Path.Num()-1 ? 20:0);
            TestFalse(TEXT("both short approaches have standing headroom"),World->OverlapBlockingTestByChannel(S,FQuat::Identity,ECC_Pawn,Capsule,Q));
            TestTrue(TEXT("every stair waypoint has solid support"),World->LineTraceSingleByChannel(Hit,S,S-FVector(0,0,150),ECC_WorldStatic,Q));
        }
        for(const auto* Starts:{&L.AlphaSpawns,&L.OmegaSpawns})for(const auto& Start:*Starts)
            TestFalse(TEXT("spawn courts have clear capsules"),World->OverlapBlockingTestByChannel(Start.GetLocation(),FQuat::Identity,ECC_Pawn,Capsule,Q));
        for(const auto& Building:L.TownBuildings)
        {
            const FVector Center=Building.GetCenter();
            TestTrue(TEXT("parcels are solid buildings, not low cover bars"),World->OverlapBlockingTestByChannel(FVector(Center.X,Center.Y,200),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeSphere(5),Q));
        }
    }
    const auto Original=T->Town;auto Bad=Original;Bad.DoorGap=FVector2D(60,80);T->Town=Bad;TestFalse(TEXT("unsafe door recipe rejects"),T->ValidateLayout(42));
    T->Town=Original;
    World->DestroyWorld(false);return true;
}
#endif
