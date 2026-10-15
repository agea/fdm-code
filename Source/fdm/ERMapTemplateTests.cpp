#include "ERMapTemplate.h"
#include "ERProceduralMap.h"
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "ERCharacter.h"
#include "ERMovement.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FERMapTemplateTest,"EclipseRecoil.Destruction.MapTemplate",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FERMapTemplateTest::RunTest(const FString& Parameters)
{
    auto* Template=LoadObject<UERMapTemplate>(nullptr,TEXT("/Game/EclipseRecoil/Art/MapTemplates/DA_Killhouse.DA_Killhouse"));
    if(!TestNotNull(TEXT("Saved template asset"),Template))return false;
    TestEqual(TEXT("balanced template mode"),Template->Symmetry,EERTemplateSymmetry::HalfTurn);
    TestNotNull(TEXT("Factory shell mesh"),Template->ShellWallMesh.Get());TestTrue(TEXT("composition variation enabled"),Template->Variation.Enabled);
    FERProceduralConfig C;C.TemplateAsset=Template;FERBlockRoomConfig B;FERProceduralLayout A,Again;
    TSet<FString> Dimensions,Compositions,BaseFrames;TSet<int32> ModuleTypes,RoomCounts;
    int32 MinBlocks=MAX_int32,MaxBlocks=0;
    for(int32 Seed=0;Seed<512;++Seed)
    {
        C.Seed=Seed;
        if(!ERProcedural::Build(C,B,A)){AddError(FString::Printf(TEXT("Invalid procedural composition seed=%d"),Seed));return false;}
        Dimensions.Add(A.ArenaSize.ToString());RoomCounts.Add(A.Rooms.Num());
        FString Choices;for(auto Kind:A.ModuleKinds){Choices+=FString::FromInt(uint8(Kind));ModuleTypes.Add(uint8(Kind));}Compositions.Add(Choices);
        BaseFrames.Add(A.Bases[0].Frame.ToString()+A.Bases[0].Size.ToString());
        MinBlocks=FMath::Min(MinBlocks,A.Blocks.Num());MaxBlocks=FMath::Max(MaxBlocks,A.Blocks.Num());
        TestTrue(TEXT("through rooms included"),A.Rooms.Num()>=2);TestTrue(TEXT("bounded block count"),A.Blocks.Num()<=50000);
    }
    TestTrue(TEXT("many arena dimensions"),Dimensions.Num()>100);TestTrue(TEXT("many compositions"),Compositions.Num()>200);
    TestTrue(TEXT("base frames and proportions vary"),BaseFrames.Num()>100);TestEqual(TEXT("seven obstacle families"),ModuleTypes.Num(),7);
    TestTrue(TEXT("room count varies"),RoomCounts.Num()>1);TestTrue(TEXT("amount of cover varies"),MaxBlocks-MinBlocks>1000);
    AddInfo(FString::Printf(TEXT("ER_TEMPLATE_VARIATION seeds=512 dimensions=%d compositions=%d base_frames=%d room_counts=%d blocks=%d..%d"),Dimensions.Num(),Compositions.Num(),BaseFrames.Num(),RoomCounts.Num(),MinBlocks,MaxBlocks));
    UWorld::InitializationValues V;V.AllowAudioPlayback(false).CreateNavigation(false).CreateAISystem(false).CreateFXSystem(false).EnableTraceCollision(true);
    auto* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&V);if(!World)return false;
    auto* Map=World->SpawnActor<AERProceduralMap>();Map->LayoutConfig=C;
    auto* Pawn=World->SpawnActor<AERCharacter>(FVector(-20000,0,100),FRotator::ZeroRotator);
    auto* Move=Cast<UERMovement>(Pawn->GetCharacterMovement());
    FCollisionQueryParams Q(SCENE_QUERY_STAT(ERTemplateTest),false);FHitResult Hit;const auto Capsule=FCollisionShape::MakeCapsule(34,92.5);
    Q.AddIgnoredActor(Pawn);
    uint32 FirstDigest=0;
    auto Key=[](FVector P,FVector S,int32 Material)
    {
        return FString::Printf(TEXT("%d:%d:%d/%d:%d:%d/%d"),FMath::RoundToInt(P.X*1000),FMath::RoundToInt(P.Y*1000),FMath::RoundToInt(P.Z*1000),FMath::RoundToInt(S.X*1000),FMath::RoundToInt(S.Y*1000),FMath::RoundToInt(S.Z*1000),Material);
    };
    for(int32 Seed:{0,1,42,7919,-17,INT32_MAX})
    {
        C.Seed=Seed;
        if(!TestTrue(TEXT("Template build"),ERProcedural::Build(C,B,A)))continue;
        TestTrue(TEXT("Repeat seed"),ERProcedural::Build(C,B,Again));TestEqual(TEXT("Stable record count"),A.Blocks.Num(),Again.Blocks.Num());
        TestTrue(TEXT("stable dimensions"),A.ArenaSize.Equals(Again.ArenaSize));
        TSet<FString> Symmetry;
        for(const auto& R:A.Blocks)Symmetry.Add(Key(R.LocalTransform.GetLocation(),R.LocalTransform.GetScale3D(),uint8(R.Material)));
        for(const auto& R:A.Blocks){auto P=R.LocalTransform.GetLocation();P.X=-P.X;P.Y=-P.Y;if(!Symmetry.Contains(Key(P,R.LocalTransform.GetScale3D(),uint8(R.Material)))){AddError(TEXT("Unpaired destructible geometry"));break;}}
        Symmetry.Empty();
        for(const auto& R:A.FixedPieces)Symmetry.Add(Key(R.Transform.GetLocation(),R.Transform.GetScale3D(),uint8(R.Material)));
        for(const auto& R:A.FixedPieces){auto P=R.Transform.GetLocation();P.X=-P.X;P.Y=-P.Y;if(!Symmetry.Contains(Key(P,R.Transform.GetScale3D(),uint8(R.Material)))){AddError(TEXT("Unpaired fixed geometry"));break;}}
        TestEqual(TEXT("Ten Alpha spawns"),A.AlphaSpawns.Num(),10);TestEqual(TEXT("Ten Omega spawns"),A.OmegaSpawns.Num(),10);
        for(int32 I=0;I<A.AlphaSpawns.Num();++I)
        {
            auto Expected=A.AlphaSpawns[I].GetLocation();Expected.X=-Expected.X;Expected.Y=-Expected.Y;
            TestTrue(TEXT("opposing team spawns have equal geometry distance"),Expected.Equals(A.OmegaSpawns[I].GetLocation(),.001));
        }
        TSet<FERBlockId> IDs;
        for(int32 I=0;I<A.Blocks.Num();++I)
        {
            const auto& R=A.Blocks[I];TestFalse(TEXT("unique block ID"),IDs.Contains(R.Id));IDs.Add(R.Id);
            TestTrue(TEXT("deterministic blocks"),R.Id==Again.Blocks[I].Id && R.LocalTransform.Equals(Again.Blocks[I].LocalTransform));
            TestTrue(TEXT("project destructible material"),R.Material==EERBlockMaterial::Wood || R.Material==EERBlockMaterial::Concrete);
        }
        Map->LayoutConfig=C;TestTrue(TEXT("Template collision generation"),Map->Regenerate());TestTrue(TEXT("mapping"),Map->ValidateMappings());
        const uint32 Digest=Map->GetLayoutDigest();if(Seed==0)FirstDigest=Digest;else TestTrue(TEXT("seed varies geometry digest"),FirstDigest!=Digest);
        for(const auto* Starts:{&A.AlphaSpawns,&A.OmegaSpawns})for(const auto& S:*Starts)
        {
            const FVector P=S.GetLocation();TestFalse(TEXT("spawn capsule clear"),World->OverlapBlockingTestByChannel(P,FQuat::Identity,ECC_Pawn,Capsule,Q));
            TestTrue(TEXT("spawn has static floor"),World->LineTraceSingleByChannel(Hit,P,P-FVector(0,0,120),ECC_WorldStatic,Q));
            TestFalse(TEXT("spawn reaches circulation lane"),World->SweepSingleByChannel(Hit,P,FVector(P.X<0 ? A.Routes[1][0].X:A.Routes[1][3].X,P.Y,P.Z),FQuat::Identity,ECC_Pawn,Capsule,Q));
        }
        const auto& Route=A.Routes[1];
        for(int32 I=0;I<Route.Num();++I)TestFalse(TEXT("complete patrol circulation"),World->SweepSingleByChannel(Hit,Route[I],Route[(I+1)%Route.Num()],FQuat::Identity,ECC_Pawn,Capsule,Q));
        for(const auto& Path:A.RoomAccessRoutes)for(int32 I=1;I<Path.Num();++I)
            TestFalse(TEXT("room has standing through access"),World->SweepSingleByChannel(Hit,Path[I-1],Path[I],FQuat::Identity,ECC_Pawn,Capsule,Q));
        for(const auto& Path:A.StairRoutes)for(int32 I=0;I<Path.Num();++I)
        {
            FVector P=Path[I];
            // A capsule standing midway along a30cm tread overlaps the next
            // riser while CharacterMovement is stepping up. Check the swept
            // standing clearance20cm above each tread, as in the base lab.
            if(I>0 && I<Path.Num()-1)P.Z+=20;
            TestFalse(FString::Printf(TEXT("stair headroom seed=%d point=%s"),Seed,*P.ToString()),World->OverlapBlockingTestByChannel(P,FQuat::Identity,ECC_Pawn,Capsule,Q));
            TestTrue(TEXT("stair route supported"),World->LineTraceSingleByChannel(Hit,P,P-FVector(0,0,140),ECC_WorldStatic,Q));
            if(I){FVector Raised=Path[I-1];Raised.Z=P.Z;TestFalse(FString::Printf(TEXT("stair sweep seed=%d point=%s"),Seed,*P.ToString()),World->SweepSingleByChannel(Hit,Raised,P,FQuat::Identity,ECC_Pawn,Capsule,Q));}
        }
        for(const auto& Base:A.Bases)
        {
            const float Front=Base.Size.Y*.5;
            for(int32 Side=0;Side<4;++Side)
            {
                const FVector Local=Side==0 ? FVector(0,Front-55,Base.RoofZ+94.6):Side==1 ? FVector(Base.Size.X*.5-55,0,Base.RoofZ+94.6):Side==2 ? FVector(0,-Front+55,Base.RoofZ+94.6):FVector(-Base.Size.X*.5+55,0,Base.RoofZ+94.6);
                const float Yaw=Base.Frame.Rotator().Yaw+90-Side*90;
                Pawn->TeleportTo(Base.Point(Local),FRotator(0,Yaw,0));Move->SetMovementMode(MOVE_Walking);Move->StopMovementImmediately();
                FVector Target;float Height=0;Move->bClimbHeld=false;
                TestFalse(TEXT("100cm parapet requires climb input"),Move->FindLedge(Target,Height));Move->bClimbHeld=true;
                const bool Found=Move->FindLedge(Target,Height);
                TestTrue(FString::Printf(TEXT("thin parapet reachable seed=%d side=%d"),Seed,Side),Found);
                if(Found)
                {
                    TestTrue(TEXT("parapet height measured from roof"),Height>95 && Height<101);
                    const FVector End=Base.Frame.InverseTransformPosition(Target);
                    const float Along=Side%2 ? FMath::Abs(End.X):FMath::Abs(End.Y),Boundary=Side%2 ? Base.Size.X*.5:Front;
                    TestTrue(TEXT("landing stays on20cm parapet"),Along>Boundary-20 && Along<Boundary);
                }
            }
            Pawn->TeleportTo(FVector(-20000,0,100),FRotator::ZeroRotator);Move->bClimbHeld=false;
            TestTrue(TEXT("field-facing centre remains covered"),World->LineTraceSingleByChannel(Hit,Base.Point(FVector(0,Front+40,150)),Base.Point(FVector(0,Front-40,150)),ECC_GameTraceChannel1,Q));
            for(float X:{-Base.Size.X*.25,Base.Size.X*.25})
                TestFalse(TEXT("front firing windows remain open"),World->LineTraceSingleByChannel(Hit,Base.Point(FVector(X,Front+40,150)),Base.Point(FVector(X,Front-40,150)),ECC_GameTraceChannel1,Q));
            for(float X=-Base.Size.X*.5+60;X<Base.Size.X*.5-40;X+=80)for(float Y=-Base.Size.Y*.5+60;Y<Base.Size.Y*.5-40;Y+=80)
            {
                if(FMath::Abs(X-Base.StairX)<150 && Y>Base.StairStartY-10 && Y<Base.StairStartY+Base.StepCount*30+10)continue;
                const FVector P=Base.Point(FVector(X,Y,Base.RoofZ));
                const bool Support=World->LineTraceSingleByChannel(Hit,P+FVector(0,0,2),P-FVector(0,0,4),ECC_WorldStatic,Q);
                TestTrue(TEXT("roof surface has no unintended hole"),Support);
                auto* ISM=Support ? Cast<UInstancedStaticMeshComponent>(Hit.GetComponent()):nullptr;
                TestTrue(TEXT("walkable roof visual equals solid collision"),ISM && ISM->GetStaticMesh()==Map->BlockMesh);
            }
        }
        const auto R=Map->BlockWorld->GetBlocks()[0];const FVector P=R.LocalTransform.GetLocation();
        const FVector N=R.LocalTransform.GetScale3D().X<R.LocalTransform.GetScale3D().Y ? FVector(1,0,0):FVector(0,1,0);
        FERBlockId Resolved;
        TestTrue(TEXT("panel hit resolves logical identity"),World->LineTraceSingleByChannel(Hit,P+N*45,P-N*45,ECC_GameTraceChannel1,Q) && Map->ResolveHit(Hit,Resolved) && Resolved==R.Id);
        TestTrue(TEXT("panel damage removes block"),Map->ApplyBlockDamage(R.Id,0,true));TestTrue(TEXT("mapping after damage"),Map->ValidateMappings());
        TestFalse(TEXT("destroyed panel collision absent"),World->LineTraceSingleByChannel(Hit,P+N*9,P-N*9,ECC_GameTraceChannel1,Q));
        const int32 Components=Map->GetInstanceComponents().Num();TestTrue(TEXT("regenerate damaged template"),Map->Regenerate());
        TestEqual(TEXT("fixed components not leaked"),Map->GetInstanceComponents().Num(),Components);TestEqual(TEXT("same seed restores digest"),Map->GetLayoutDigest(),Digest);
    }
    C.Floors=2;const auto Count=A.Blocks.Num();TestFalse(TEXT("unsupported floor count rejected atomically"),ERProcedural::Build(C,B,A));TestEqual(TEXT("old data retained"),A.Blocks.Num(),Count);
    C.Floors=1;auto* Invalid=DuplicateObject<UERMapTemplate>(Template,GetTransientPackage());C.TemplateAsset=Invalid;Invalid->Variation.BaseDepth=FVector2D(200,400);
    TestFalse(TEXT("unsafe variant dimensions rejected"),ERProcedural::Build(C,B,A));
    Invalid->Variation.Enabled=false;Invalid->Pieces[0].WalkableSurface=true;Invalid->Pieces[0].Destructible=false;Invalid->Pieces[0].FixedMesh=Template->Variation.CargoMesh;
    TestFalse(TEXT("open industrial art cannot become a walkable slab"),ERProcedural::Build(C,B,A));
    Invalid->Pieces[0].Size.X=INFINITY;TestFalse(TEXT("nonfinite template rejected"),ERProcedural::Build(C,B,A));
    World->DestroyWorld(false);return true;
}
#endif
