#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERBlockTarget.generated.h"

// Standalone lab target, not a combat pawn or a network gameplay actor.
UCLASS()
class FDM_API AERBlockTarget : public AActor
{
    GENERATED_BODY()
public:
    AERBlockTarget();
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UStaticMeshComponent> Mesh;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UTextRenderComponent> Label;
    float Health = 500;
    virtual void BeginPlay() override;
    virtual float TakeDamage(float Amount, const FDamageEvent& Event, AController* DamageInstigator, AActor* Causer) override;
    void ResetHealth();
};
