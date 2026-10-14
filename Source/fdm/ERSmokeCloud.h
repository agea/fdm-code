#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERSmokeCloud.generated.h"

UCLASS()
class FDM_API AERSmokeCloud : public AActor
{
    GENERATED_BODY()
public:
    AERSmokeCloud();
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UNiagaraComponent> Fumes;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite) TObjectPtr<class UNiagaraSystem> System;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite) FVector SimSize = FVector(600,600,400);
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite) int32 GridResolution = 40;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite) float Duration = 12;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite) FLinearColor SmokeColor = FLinearColor(.33f,.33f,.33f,1);
    UPROPERTY(Replicated) double StartedAtServerTime = 0;
    int32 LastColliderCount = 0;
    uint32 ShotInteractionCount = 0;
    bool bPrewarm = false;
    static void Prewarm(UWorld* World, TSubclassOf<AERSmokeCloud> CloudClass, const FVector& ViewPosition);
    void AddShot(const FVector& Start, const FVector& End);
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Props) const override;
    UFUNCTION(BlueprintCallable, Category="Eclipse Recoil|Editor")
    static FString DescribeSmokeSource(class UBlueprint* Blueprint, UNiagaraSystem* SmokeSystem);
    UFUNCTION(BlueprintCallable, Category="Eclipse Recoil|Editor")
    static bool PrepareOwnedSystem(UNiagaraSystem* SmokeSystem);
private:
    struct FShotCut { FVector Position; double Until; };
    TArray<FShotCut> ShotCuts;
    UPROPERTY() TObjectPtr<class AExponentialHeightFog> FogSupport;
    bool bUsesFogPass = false;
    double ServerNow() const;
};
