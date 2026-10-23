#include "ERClaymore.h"
#include "Engine/DamageEvents.h"
#include "ERMatch.h"
#include "ERRecon.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ERCharacter.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FERClaymoreLifecycleTest,"EclipseRecoil.Loadout.ClaymoreLifecycle",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FERClaymoreLifecycleTest::RunTest(const FString&)
{
    UWorld::InitializationValues V;V.AllowAudioPlayback(false).CreateNavigation(false).CreateAISystem(false).CreateFXSystem(false).EnableTraceCollision(true);
    auto* W=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&V);
    if(!TestNotNull(TEXT("World"),W))return false;
    W->SetGameState(W->SpawnActor<AERGameState>());
    auto* FloorActor=W->SpawnActor<AActor>();
    auto* Floor=NewObject<UStaticMeshComponent>(FloorActor);FloorActor->SetRootComponent(Floor);FloorActor->AddInstanceComponent(Floor);
    Floor->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
    Floor->SetCollisionProfileName(TEXT("BlockAll"));Floor->SetCollisionObjectType(ECC_WorldStatic);Floor->RegisterComponent();
    Floor->SetWorldLocation(FVector(0,0,-10));Floor->SetWorldScale3D(FVector(200,200,.2f));
    // Reproduce a real player pressing fire while looking ahead, rather than
    // only testing the previous nearly vertical (-89 degree) placement fixture.
    auto* Placer=W->SpawnActor<AERCharacter>(FVector(-1000,1000,95),FRotator::ZeroRotator);
    auto* PlacerController=W->SpawnActor<APlayerController>();PlacerController->Possess(Placer);
    Placer->Health->ProtectionEndServerTime=0;
    FHitResult PlacementHit;FRotator PlacementRotation;FString Failure;
    for(float Pitch : {0.f,-30.f,-60.f,-89.f,80.f})
    {
        PlacerController->SetControlRotation(FRotator(Pitch,0,0));
        TestTrue(TEXT("Ordinary and downward view angles allow ground placement"),AERClaymore::Placement(Placer,PlacementHit,PlacementRotation,&Failure));
        TestTrue(TEXT("Placement is within horizontal reach"),FVector::Dist2D(Placer->GetActorLocation(),PlacementHit.ImpactPoint)<=180.f);
        TestTrue(TEXT("Placed front follows view yaw even looking straight down"),PlacementRotation.Vector().X>.99f);
    }
    PlacerController->SetControlRotation(FRotator(80,0,0));
    TestTrue(TEXT("Camera pitch does not affect fixed placement ahead"),AERClaymore::Placement(Placer,PlacementHit,PlacementRotation,&Failure));
    PlacerController->SetControlRotation(FRotator::ZeroRotator);
    Placer->Health->ProtectionEndServerTime=100;
    TestFalse(TEXT("Spawn protection prevents placement"),AERClaymore::Placement(Placer,PlacementHit,PlacementRotation,&Failure));
    Placer->Health->ProtectionEndServerTime=0;
    auto* ObstacleActor=W->SpawnActor<AActor>();auto* Obstacle=NewObject<UStaticMeshComponent>(ObstacleActor);
    ObstacleActor->SetRootComponent(Obstacle);ObstacleActor->AddInstanceComponent(Obstacle);
    Obstacle->SetStaticMesh(Floor->GetStaticMesh());Obstacle->SetCollisionProfileName(TEXT("BlockAll"));
    Obstacle->SetCollisionObjectType(ECC_WorldStatic);Obstacle->RegisterComponent();
    Obstacle->SetWorldLocation(FVector(-950,1000,90));Obstacle->SetWorldScale3D(FVector(.2f,2,2));
    TestFalse(TEXT("Cannot place through a wall"),AERClaymore::Placement(Placer,PlacementHit,PlacementRotation,&Failure));
    ObstacleActor->Destroy();PlacerController->UnPossess();Placer->Destroy();PlacerController->Destroy();
    auto* Owner=W->SpawnActor<AERPlayerState>();Owner->Team=EERTeam::Alpha;
    auto* Other=W->SpawnActor<AERPlayerState>();Other->Team=EERTeam::Omega;
    auto Make=[&](AERPlayerState* PS,int32 Index)
    {
        const FVector Point(Index*35,0,0);const FTransform T(FRotator::ZeroRotator,Point+FVector(0,0,10));
        auto* M=W->SpawnActorDeferred<AERClaymore>(AERClaymore::StaticClass(),T,PS,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        M->SessionOwner=PS;M->PlacedTeam=uint8(PS->Team);M->SupportActor=FloorActor;M->SupportPoint=Point;
        UGameplayStatics::FinishSpawningActor(M,T);M->DispatchBeginPlay();M->ArmAt=W->GetTimeSeconds();M->Tick(.1f);return M;
    };
    auto* A=Make(Owner,0);auto* B=Make(Owner,1);auto* C=Make(Owner,2);
    TestTrue(TEXT("Armed on server"),A->State==EERClaymoreState::Armed);
    TestEqual(TEXT("Sixty seconds from arm completion"),A->ExpiresAt-W->GetTimeSeconds(),60.);
    auto* Target=W->SpawnActor<AERCharacter>(FVector(300,0,92.5),FRotator::ZeroRotator);
    Target->SetPlayerState(Owner);
    TestEqual(TEXT("Trigger range is two metres"),A->DetectionRange,200.f);
    TestEqual(TEXT("Trigger opening is sixty degrees"),A->DetectionHalfAngle*2,60.f);
    for(const auto& Sample : {FVector(80,0,92.5),FVector(199,0,92.5),FVector(173.205,100,92.5)})
    {
        Target->SetActorLocation(Sample);TestTrue(TEXT("Inside trigger sector including close standing target and edge"),A->TriggerTarget(Target));
    }
    for(const auto& Sample : {FVector(201,0,92.5),FVector(170,105,92.5),FVector(-80,0,92.5),FVector(0,80,92.5),FVector(80,0,350)})
    {
        Target->SetActorLocation(Sample);TestFalse(TEXT("Outside trigger sector"),A->TriggerTarget(Target));
    }
    A->UpdateLasers();
    TestTrue(TEXT("Both activation boundary rays visible when armed"),A->LaserLeft->IsVisible() && A->LaserRight->IsVisible());
    TestEqual(TEXT("Laser has no gameplay collision"),A->LaserLeft->GetCollisionEnabled(),ECollisionEnabled::NoCollision);
    for(auto* Laser : {A->LaserLeft.Get(),A->LaserRight.Get()})
    {
        TestTrue(TEXT("Laser follows thirty degree boundary"),FMath::IsNearlyEqual(FVector::DotProduct(Laser->GetUpVector(),A->GetActorForwardVector()),FMath::Cos(PI/6),.001f));
        TestTrue(TEXT("Laser length matches two metres"),FMath::IsNearlyEqual(Laser->GetComponentScale().Z*100,200.f,.01f));
    }
    if(TestNotNull(TEXT("Emissive beam material"),A->LaserMaterial.Get()))
    {
        TestTrue(TEXT("Alpha laser matches team color"),A->LaserMaterial->K2_GetVectorParameterValue(TEXT("Color")).Equals(ERRecon::TeamColor(EERTeam::Alpha)));
        A->PlacedTeam=uint8(EERTeam::Omega);A->UpdateLasers();
        TestTrue(TEXT("Omega laser matches team color"),A->LaserMaterial->K2_GetVectorParameterValue(TEXT("Color")).Equals(ERRecon::TeamColor(EERTeam::Omega)));
        A->PlacedTeam=uint8(EERTeam::Alpha);
    }
    Target->SetActorLocation(FVector(300,0,92.5));
    TestTrue(TEXT("Front target visible"),A->VisibleTarget(Target));
    Target->SetActorLocation(FVector(-300,0,92.5));TestFalse(TEXT("Rear target excluded from damage cone"),A->VisibleTarget(Target));
    Target->SetActorLocation(FVector(300,0,92.5));
    auto* WallActor=W->SpawnActor<AActor>();auto* Wall=NewObject<UStaticMeshComponent>(WallActor);WallActor->SetRootComponent(Wall);WallActor->AddInstanceComponent(Wall);
    Wall->SetStaticMesh(Floor->GetStaticMesh());Wall->SetCollisionProfileName(TEXT("BlockAll"));Wall->SetCollisionObjectType(ECC_WorldStatic);Wall->RegisterComponent();
    Wall->SetWorldLocation(FVector(150,0,90));Wall->SetWorldScale3D(FVector(.2f,3,2));
    A->UpdateLasers();
    TestTrue(TEXT("Both lasers stop at intact cover"),A->LaserLeft->GetComponentScale().Z*100<200 && A->LaserRight->GetComponentScale().Z*100<200);
    TestFalse(TEXT("Intact cover blocks trigger and damage"),A->VisibleTarget(Target));
    Target->SetActorLocation(FVector(190,0,92.5));
    TestFalse(TEXT("Cover blocks activation within two metres"),A->TriggerTarget(Target));
    Target->SetActorLocation(FVector(80,0,92.5));
    Wall->DestroyComponent();A->Tick(.1f);TestTrue(TEXT("Ally cannot trigger"),A->State==EERClaymoreState::Armed);
    Target->Destroy();
    const double AExpiry=A->ExpiresAt;B->ExpiresAt+=10;C->ExpiresAt+=20;
    Owner->LoadoutPreset="sniper";Owner->LoadoutVariant="gunner";
    TestEqual(TEXT("Selection cannot reset first timer"),A->ExpiresAt,AExpiry);
    TestEqual(TEXT("Three independent deadlines"),C->ExpiresAt-B->ExpiresAt,10.);
    TestEqual(TEXT("All mines counted"),Owner->ActiveClaymores,3);
    A->ExpiresAt=W->GetTimeSeconds();A->Detonate();
    TestTrue(TEXT("Expiry beats trigger without explosion"),A->State==EERClaymoreState::Terminal);
    TestTrue(TEXT("Other mine remains armed"),B->State==EERClaymoreState::Armed);
    B->TakeDamage(1,FDamageEvent(),nullptr,nullptr);B->TakeDamage(1,FDamageEvent(),nullptr,nullptr);
    TestTrue(TEXT("Neutralization idempotent"),B->State==EERClaymoreState::Terminal);
    // No pool, per-player cap, or oldest-instance replacement even at high counts.
    for(int32 I=3;I<259;++I)Make(Owner,I);
    TestTrue(TEXT("Existing third mine survives256 more placements"),C->State==EERClaymoreState::Armed);
    TestTrue(TEXT("Counter supports hundreds"),Owner->ActiveClaymores==257);
    const double Started=FPlatformTime::Seconds();
    for(int32 Pass=0;Pass<100;++Pass)for(TActorIterator<AERClaymore> It(W);It;++It)if(It->State==EERClaymoreState::Armed)It->Tick(.1f);
    UE_LOG(LogTemp,Display,TEXT("ER_CLAYMORE_STRESS active=%d average_detection_pass_ms=%.3f context=empty_static_floor"),Owner->ActiveClaymores,(FPlatformTime::Seconds()-Started)*10.);
    auto* Foreign=Make(Other,260);
    AERClaymore::RemoveFor(W,Owner);TestTrue(TEXT("Owner cleanup preserves others"),Foreign->State==EERClaymoreState::Armed);
    Other->Team=EERTeam::Alpha;Foreign->Tick(.1f);TestTrue(TEXT("Team change disarms"),Foreign->State==EERClaymoreState::Terminal);
    auto* Unsupported=Make(Owner,261);
    const double Deadline=Unsupported->ExpiresAt;
    // Cell destruction may replace a component while the same floor remains solid.
    Floor->DestroyComponent();
    Floor=NewObject<UStaticMeshComponent>(FloorActor);FloorActor->SetRootComponent(Floor);FloorActor->AddInstanceComponent(Floor);
    Floor->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
    Floor->SetCollisionProfileName(TEXT("BlockAll"));Floor->SetCollisionObjectType(ECC_WorldStatic);Floor->RegisterComponent();
    Floor->SetWorldLocation(FVector(0,0,-10));Floor->SetWorldScale3D(FVector(200,200,.2f));
    Unsupported->Tick(.1f);TestTrue(TEXT("Intact support survives component replacement"),Unsupported->State==EERClaymoreState::Armed);
    TestEqual(TEXT("Support rebuild does not reset lifetime"),Unsupported->ExpiresAt,Deadline);
    Floor->SetCollisionEnabled(ECollisionEnabled::NoCollision);Unsupported->Tick(.1f);
    TestTrue(TEXT("Lost support disarms"),Unsupported->State==EERClaymoreState::Terminal);
    W->DestroyWorld(false);return true;
}
#endif
