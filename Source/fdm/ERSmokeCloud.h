#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERSmokeCloud.generated.h"
enum class EERTeam : uint8;

UCLASS()
class FDM_API AERSmokeCloud : public AActor
{
    GENERATED_BODY()
public:
    AERSmokeCloud();
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UNiagaraComponent> Fumes;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UStaticMeshComponent> Volume;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite) TObjectPtr<class UMaterialInterface> VolumeMaterial;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite) TObjectPtr<class UStaticMesh> VolumeMesh;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite) TObjectPtr<class UNiagaraSystem> System;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite) TObjectPtr<class USoundBase> ReleaseSound;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite) TObjectPtr<class USoundAttenuation> ReleaseAttenuation;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite) TObjectPtr<class USoundConcurrency> ReleaseConcurrency;
    UPROPERTY(Transient) TObjectPtr<class UAudioComponent> ReleaseAudio;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite) FVector SimSize = FVector(800,800,400);
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite) int32 GridResolution = 40;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite) float Duration = 12;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, ReplicatedUsing=OnRep_SmokeColor) FLinearColor SmokeColor = FLinearColor(.33f,.33f,.33f,1);
    UFUNCTION() void OnRep_SmokeColor();
    static FLinearColor ColorForTeam(EERTeam Team);
    UPROPERTY(Replicated) double StartedAtServerTime = 0;
    UPROPERTY(Replicated) FVector ShapePhase = FVector::ZeroVector;
    int32 LastColliderCount = 0;
    uint32 ShotInteractionCount = 0;
    uint32 ProjectileInteractionCount = 0, BlastInteractionCount = 0;
    float LastBlastRadius = 0, LastBlastLifetime = 0;
    bool bPrewarm = false;
    static void Prewarm(UWorld* World, TSubclassOf<AERSmokeCloud> CloudClass, const FVector& ViewPosition);
    void AddShot(const FVector& Start, const FVector& End, float Radius = 12.f, float Lifetime = 2.f);
    float GetActiveBlastRadius() const;
    float GetActiveShotRadius() const;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Props) const override;
    void RebuildContainment();
    bool ContainsSmokeAt(const FVector& Position) const;
    // Authoritative optical depth through occupied smoke, including growth,
    // fade and the same shot/blast pockets used by the volume material.
    float ReconOpticalDepth(const FVector& Start, const FVector& End) const;
    int32 FilledCellCount = 0;
    FVector VolumeOrigin = FVector::ZeroVector;
    UFUNCTION(BlueprintCallable, Category="Respawn & Rage|Editor")
    static FString DescribeSmokeSource(class UBlueprint* Blueprint, UNiagaraSystem* SmokeSystem);
    UFUNCTION(BlueprintCallable, Category="Respawn & Rage|Editor")
    static bool PrepareOwnedSystem(UNiagaraSystem* SmokeSystem);
private:
    struct FShotCut { FVector Start, End; double Until; float Radius, Lifetime; };
    TArray<FShotCut> ShotCuts;
    float RecoveredRadius(const FShotCut& Cut, double Now) const;
    float ReconDensity(const FVector& Position, float Age) const;
    UPROPERTY() TObjectPtr<class UTexture2D> DensityAtlas;
    UPROPERTY() TObjectPtr<class UMaterialInstanceDynamic> VolumeInstance;
    TArray<uint8> Occupancy;
    uint64 GeometryRevision = MAX_uint64;
    double NextGeometryCheck = 0;
    void UploadDensity();
    double ServerNow() const;
};
