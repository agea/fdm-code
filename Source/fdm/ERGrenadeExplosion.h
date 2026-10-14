#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERGrenadeExplosion.generated.h"

// Local cosmetic burst spawned by an authoritative detonation multicast.
UCLASS()
class FDM_API AERGrenadeExplosion : public AActor
{
    GENERATED_BODY()
public:
    AERGrenadeExplosion();
    UPROPERTY(VisibleAnywhere) TObjectPtr<class USkeletalMeshComponent> Debris;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UStaticMeshComponent> Flash;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UPointLightComponent> Light;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite) TObjectPtr<class USkeletalMesh> DebrisMesh;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite) TObjectPtr<class UAnimSequence> DebrisAnimation;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite) TObjectPtr<class UStaticMesh> FlashMesh;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite) TObjectPtr<class UMaterialInterface> FlashMaterial;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite) float BurstScale = .75f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite) float PlayRate = 1.5f;
    UFUNCTION(BlueprintCallable, Category="Eclipse Recoil|Editor")
    static bool AssignDebrisMaterials(USkeletalMesh* Mesh, UMaterialInterface* Surface, UMaterialInterface* Side);
    static void PrecacheEffect(TSubclassOf<AERGrenadeExplosion> EffectClass);
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
private:
    UPROPERTY() TArray<TObjectPtr<class UMaterialInstanceDynamic>> DebrisMaterials;
    float Elapsed = 0, Duration = 2;
};
