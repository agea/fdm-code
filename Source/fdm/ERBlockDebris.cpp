#include "ERBlockDebris.h"
#include "ERPerformanceSettings.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
AERBlockDebris::AERBlockDebris()
{
    PrimaryActorTick.bCanEverTick=false;
    Mesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DebrisMesh"));RootComponent=Mesh;
    Mesh->SetMobility(EComponentMobility::Movable);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube"));Mesh->SetStaticMesh(Cube.Object);
    Mesh->SetCollisionObjectType(ECC_PhysicsBody);
    Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    Mesh->SetCollisionResponseToChannel(ECC_WorldStatic,ECR_Block);
    Mesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
    Mesh->SetCanEverAffectNavigation(false);
    Mesh->SetLinearDamping(.25f);Mesh->SetAngularDamping(1);
}
void AERBlockDebris::Initialize(UStaticMesh* Geometry,UMaterialInterface* Surface,const FTransform& Transform,float Lifetime,float SettleAfter)
{
    Life=FMath::Clamp(Lifetime,1.f,20.f);Settle=FMath::Clamp(SettleAfter,.1f,Life);
    Mesh->SetStaticMesh(Geometry);if (Surface) Mesh->SetMaterial(0,Surface);
    FTransform T=Transform;T.SetScale3D(T.GetScale3D()*.92);SetActorTransform(T);
}
void AERBlockDebris::BeginPlay()
{
    Super::BeginPlay();
    if (GetNetMode()==NM_DedicatedServer || !Mesh->GetStaticMesh()) {Destroy();return;}
    Mesh->SetSimulatePhysics(true);Mesh->WakeAllRigidBodies();
    Mesh->SetMassOverrideInKg(NAME_None,10,true);
    GetWorldTimerManager().SetTimer(SettleTimer,this,&AERBlockDebris::StopSimulation,Settle,false);
    const int32 Quality=ERPerformance::Get().Effects;
    SetLifeSpan(FMath::Min(Life,ERPerformance::Get().Defaults ? 20.f:Quality<=1 ? 2.f:Quality==2 ? 5.f:20.f));
}
void AERBlockDebris::StopSimulation()
{
    Mesh->SetSimulatePhysics(false);Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
bool AERBlockDebris::IsSimulating() const { return Mesh->IsSimulatingPhysics(); }
void AERBlockDebris::EndPlay(const EEndPlayReason::Type Reason)
{
    GetWorldTimerManager().ClearTimer(SettleTimer);StopSimulation();Super::EndPlay(Reason);
}
