#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameFramework/Actor.h"
#include "ERBlockCombat.h"
#include "ERWeapon.generated.h"

UENUM(BlueprintType)
enum class EERPayload : uint8 { Bullet, Rocket, HE, Smoke, Stun, Melee, LaunchedHE };

UCLASS(BlueprintType)
class FDM_API UERWeaponData : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString DisplayName = TEXT("Assault Rifle");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") TObjectPtr<class USkeletalMesh> Mesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") TObjectPtr<class USkeletalMesh> SightMesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") FTransform SightTransform;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") FTransform GripTransform;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") bool bFirstPersonPalmGrip = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") FTransform FirstPersonGripTransform;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") bool bFitSupportHand = false;
    // Full-size weapon-local palm contact, before the socket attachment transform.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") FVector SupportHandOffset = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") FRotator SupportPalmRotation = FRotator(0,90,0);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") TArray<FVector> SupportSurfaceContacts;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") FVector ViewOffset = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") FVector MuzzleOffset = FVector(0, 60, 0);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") TObjectPtr<class UAnimSequence> IdleAnimation;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") TObjectPtr<class UAnimSequence> FireAnimation;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") TObjectPtr<class UAnimSequence> ReloadAnimation;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") TObjectPtr<class UAnimSequence> WeaponIdleAnimation;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") TObjectPtr<class UAnimSequence> WeaponFireAnimation;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") TObjectPtr<class UAnimSequence> WeaponReloadAnimation;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") TObjectPtr<class USkeletalMesh> ProjectileMesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") bool bOneHanded = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") float SoundPitch = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bAutomatic = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Pellets = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ballistics") bool bUseBodyHitZones = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ballistics") float PelletSpreadDegrees = 3.2f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EERPayload Payload = EERPayload::Bullet;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float ExplosionRadius = 300;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float ExplosionInnerRadius = 200;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float FuseDuration = 3.5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float ZoomFOV = 90;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Damage = 36;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ballistics") float ArmorRatio = 2;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ballistics") float RangeModifier = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float ShotInterval = 0.1f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MagazineSize = 30;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 InitialReserve = 90;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float ReloadDuration = 1.5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float ProjectileSpeed = 30000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float ProjectileLife = 2;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hitscan", meta=(ClampMin="1")) float HitscanRange = 60000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hitscan|Blocks") FERBlockShotSettings BlockShot;
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
    UPROPERTY(Replicated, BlueprintReadOnly) bool bCookingGrenade = false;
    UPROPERTY(Replicated) double GrenadeRecoveryEndServerTime = 0;
    UPROPERTY() TObjectPtr<class AERProjectile> CookedGrenade;
    static bool IsHandGrenade(EERPayload Kind);
    bool IsGrenadeHandHidden() const;
    void OnGrenadeDetonated(AERProjectile* Projectile);
    UPROPERTY(Replicated, BlueprintReadOnly) float BloomAtLastShot = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) double LastSpreadShotServerTime = -100;
    float GetSpreadDegrees() const;
    float GetBloomDegrees() const;
    // Observations of the actual server traces for development probes.
    FVector LastShotTraceStart = FVector::ZeroVector;
    FVector LastShotTraceEnd = FVector::ZeroVector;
    int32 LastShotTraceCount = 0;
    TArray<FHitResult,TInlineAllocator<6>> LastShotPelletHits;
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
    void ArmGrenade();
    void ReleaseGrenade(bool bThrow);
};

UCLASS()
class FDM_API AERProjectile : public AActor
{
    GENERATED_BODY()
public:
    AERProjectile();
    UPROPERTY(VisibleAnywhere) TObjectPtr<class USphereComponent> Collision;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class USphereComponent> ShotTarget;
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
    float ExplosionInnerRadius = 200;
    float BlastArmorRatio = 1.6f;
    float FuseDuration = 3.5f;
    UPROPERTY(ReplicatedUsing=OnRep_Held) bool bHeld = false;
    UPROPERTY(Replicated) double FuseEndServerTime = 0;
    UFUNCTION() void OnRep_Held();
    void ReleaseHeld(const FVector& Position, const FVector& Velocity);
    static FVector SafeHandPosition(class AERCharacter* Character, bool bOverhead, const AActor* Ignore = nullptr);
    virtual float TakeDamage(float Amount, const FDamageEvent& Event, AController* Instigator, AActor* Causer) override;
    bool HasDetonated() const { return bImpacted; }
    FVector ReleasedFrom = FVector::ZeroVector;
    FVector ReleasedVelocity = FVector::ZeroVector;
    static bool UsesFuse(EERPayload Kind);
    int32 BounceCount = 0;
    virtual void Tick(float DeltaSeconds) override;
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    UFUNCTION() void Hit(UPrimitiveComponent* Component, AActor* Other, UPrimitiveComponent* OtherComponent, FVector Impulse, const FHitResult& Result);
    UFUNCTION() void Bounce(const FHitResult& Result, const FVector& IncomingVelocity);
    UFUNCTION(NetMulticast, Unreliable) void MulticastImpact(FVector_NetQuantize Position, FVector_NetQuantizeNormal Normal, float Roll);
    UFUNCTION(NetMulticast, Reliable) void MulticastDetonate(FVector_NetQuantize Position, EERPayload Kind);
private:
    bool bImpacted = false;
    bool bChainPending = false;
    FTimerHandle FuseTimer;
    FRandomStream BounceRandom;
    FLinearColor SmokeTeamColor = FLinearColor(.30f,.31f,.32f,1.f);
    FVector PreviousSmokePosition = FVector::ZeroVector;
    double NextSmokeInteraction = 0;
    FVector Spin = FVector::ZeroVector;
    FVector RestNormal = FVector::UpVector;
    FQuat RestRotation = FQuat::Identity;
    bool bResting = false;
    void Detonate();
};
