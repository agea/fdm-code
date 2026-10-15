#include "ERBlockTarget.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AERBlockTarget::AERBlockTarget()
{
    PrimaryActorTick.bCanEverTick = false;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Target"));
    Mesh->SetupAttachment(RootComponent);
    Mesh->SetRelativeScale3D(FVector(1, .2, 1));
    Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube"));
    Mesh->SetStaticMesh(Cube.Object);
    Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("HealthLabel"));
    Label->SetupAttachment(RootComponent);
    Label->SetRelativeLocation(FVector(0, -11, 65));
    Label->SetRelativeRotation(FRotator(0, -90, 0));
    Label->SetWorldSize(16);
    Label->SetHorizontalAlignment(EHTA_Center);
    Label->SetTextRenderColor(FColor::Cyan);
    Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
void AERBlockTarget::BeginPlay() { Super::BeginPlay(); ResetHealth(); }
void AERBlockTarget::ResetHealth()
{
    Health = 500;
    Label->SetText(FText::FromString(TEXT("TARGET HP 500")));
}
float AERBlockTarget::TakeDamage(float Amount, const FDamageEvent& Event, AController* Instigator, AActor* Causer)
{
    if (!HasAuthority() || GetNetMode() != NM_Standalone || !FMath::IsFinite(Amount) || Amount <= 0) return 0;
    const float Applied = FMath::Min(Amount, Health);
    Health -= Applied;
    Label->SetText(FText::FromString(FString::Printf(TEXT("TARGET HP %.1f"), Health)));
    UE_LOG(LogTemp, Display, TEXT("ER_BLOCK_TARGET %s hp=%.1f damage=%.1f"), *GetName(), Health, Applied);
    return Applied;
}
