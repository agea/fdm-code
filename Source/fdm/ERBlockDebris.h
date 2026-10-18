#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERBlockWorld.h"
#include "ERBlockDebris.generated.h"

// Temporary bounded Chaos rigid body. It never becomes gameplay cover or
// changes player/projectile collision; the detached cells are gone from storage.
UCLASS(NotBlueprintable)
class FDM_API AERBlockDebris : public AActor
{
    GENERATED_BODY()
public:
    AERBlockDebris();
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UStaticMeshComponent> Mesh;
    void Initialize(class UStaticMesh* Geometry,class UMaterialInterface* Surface,const FTransform& Transform,float Lifetime,float SettleAfter);
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    bool IsSimulating() const;
private:
    float Life=8,Settle=2;
    FTimerHandle SettleTimer;
    void StopSimulation();
};
