#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameFramework/Actor.h"
#include "ERWeapon.generated.h"

UENUM(BlueprintType)
enum class EERPayload : uint8 { Bullet, Rocket, HE, Smoke, Stun };

UCLASS(BlueprintType)
class FDM_API UERWeaponData : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString DisplayName = TEXT("Rifle A");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") TObjectPtr<class USkeletalMesh> Mesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") TObjectPtr<class USkeletalMesh> SightMesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") FTransform SightTransform;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") FTransform GripTransform;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") FVector ViewOffset = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") FVector MuzzleOffset = FVector(0, 60, 0);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") TObjectPtr<class UAnimSequence> IdleAnimation;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") TObjectPtr<class UAnimSequence> FireAnimation;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") bool bOneHanded = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") float SoundPitch = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bAutomatic = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Pellets = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EERPayload Payload = EERPayload::Bullet;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float ExplosionRadius = 300;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float FuseDuration = 2.5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float ZoomFOV = 90;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Damage = 36;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float ShotInterval = 0.1f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MagazineSize = 30;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 InitialReserve = 90;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float ReloadDuration = 1.5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float ProjectileSpeed = 30000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float ProjectileLife = 2;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hitscan", meta=(ClampMin="1")) float HitscanRange = 60000;
    // Zero disables tracers; one shows every shot. Server selects cosmetic cadence.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hitscan", meta=(ClampMin="0")) int32 TracerEveryNShots = 3;
    // Cone half-angles in degrees. Movement and burst penalties are additive.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Accuracy", meta=(ClampMin="0", ClampMax="10")) float StandingSpreadDegrees = .18f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Accuracy", meta=(ClampMin="0", ClampMax="1")) float CrouchSpreadMultiplier = .6f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Accuracy", meta=(ClampMin="0", ClampMax="10")) float RunningSpreadDegrees = .55f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Accuracy", meta=(ClampMin="0", ClampMax="10")) float AirborneSpreadDegrees = .9f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Accuracy", meta=(ClampMin="0", ClampMax="10")) float BloomPerShotDegrees = .12f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Accuracy", meta=(ClampMin="0", ClampMax="10")) float MaxBloomDegrees = .75f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Accuracy", meta=(ClampMin="0")) float BloomRecoveryDelay = .15f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Accuracy", meta=(ClampMin=".01")) float BloomRecoveryDegreesPerSecond = 1.2f;
};

UCLASS()
class FDM_API AERWeapon : public AActor
{
    GENERATED_BODY()
public:
    AERWeapon();
    UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadWrite) TObjectPtr<UERWeaponData> Data;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite) TSubclassOf<class AERProjectile> ProjectileClass;
    UFUNCTION(NetMulticast, Unreliable) void MulticastImpact(FVector_NetQuantize Position, FVector_NetQuantizeNormal Normal, float Roll, uint8 Variant);
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Shot feedback") TArray<TObjectPtr<class UMaterialInterface>> ImpactDecalVariants;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Shot feedback") TObjectPtr<class UNiagaraSystem> ImpactSystem;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 MagazineAmmo = 30;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 ReserveAmmo = 90;
    UPROPERTY(Replicated, BlueprintReadOnly) bool bReloading = false;
    UPROPERTY(Replicated, BlueprintReadOnly) float BloomAtLastShot = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) double LastSpreadShotServerTime = -100;
    float GetSpreadDegrees() const;
    float GetBloomDegrees() const;
    // Observations of the actual server traces for development probes.
    FVector LastShotTraceStart = FVector::ZeroVector;
    FVector LastShotTraceEnd = FVector::ZeroVector;
    int32 LastShotTraceCount = 0;
    uint32 AuthorizedShotCount = 0;
    FVector LastShotAimDirection = FVector::ForwardVector;
    FVector LastShotDirection = FVector::ForwardVector;
    float LastShotSpreadDegrees = 0;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    void StartFire();
    void StopFire();
    void Reload();
    void Cancel();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
private:
    FTimerHandle FireTimer, ReloadTimer;
    double NextAllowedShotTime = 0;
    bool bWantsFire = false;
    FRandomStream ShotRandom;
    bool CanFight() const;
    void TryFire();
    void FinishReload();
};

UCLASS()
class FDM_API AERProjectile : public AActor
{
    GENERATED_BODY()
public:
    AERProjectile();
    UPROPERTY(VisibleAnywhere) TObjectPtr<class USphereComponent> Collision;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UProjectileMovementComponent> Movement;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Presentation") TObjectPtr<class UMaterialInterface> ImpactDecalMaterial;
    float Damage = 36;
    UPROPERTY(Replicated) EERPayload Payload = EERPayload::Bullet;
    UPROPERTY(Replicated) TObjectPtr<class USkeletalMesh> ThrownMesh;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class USkeletalMeshComponent> Visual;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UStaticMeshComponent> Effect;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite) TObjectPtr<class UStaticMesh> EffectMesh;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite) TObjectPtr<class UMaterialInterface> SmokeMaterial;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite) TObjectPtr<class UMaterialInterface> ExplosionMaterial;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Presentation") TSubclassOf<class AERGrenadeExplosion> GrenadeExplosionClass;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Presentation") TSubclassOf<class AERSmokeCloud> SmokeCloudClass;
    float ExplosionRadius = 300;
    float FuseDuration = 2.5f;
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    UFUNCTION() void Hit(UPrimitiveComponent* Component, AActor* Other, UPrimitiveComponent* OtherComponent, FVector Impulse, const FHitResult& Result);
    UFUNCTION(NetMulticast, Unreliable) void MulticastImpact(FVector_NetQuantize Position, FVector_NetQuantizeNormal Normal, float Roll);
    UFUNCTION(NetMulticast, Reliable) void MulticastDetonate(FVector_NetQuantize Position, EERPayload Kind);
private:
    bool bImpacted = false;
    FTimerHandle FuseTimer;
    void Detonate();
};
