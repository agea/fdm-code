#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERClaymore.generated.h"
UENUM()
enum class EERClaymoreState : uint8 { Arming, Armed, Triggered, Terminal };

UCLASS(Config=Game)
class FDM_API AERClaymore : public AActor
{
    GENERATED_BODY()
public:
    AERClaymore();
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UStaticMeshComponent> Body;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UStaticMeshComponent> Front;
    UPROPERTY(Replicated) TObjectPtr<class AERPlayerState> SessionOwner;
    UPROPERTY(ReplicatedUsing=OnRep_DeviceMesh) TObjectPtr<class UStaticMesh> DeviceMesh;
    UFUNCTION() void OnRep_DeviceMesh();
    UFUNCTION(NetMulticast,Unreliable) void MulticastDetonation(TSubclassOf<class AERGrenadeExplosion> Effect);
    UPROPERTY() TSubclassOf<class AERGrenadeExplosion> BlastEffect;
    UPROPERTY(Replicated) uint8 PlacedTeam = 0;
    UPROPERTY(Replicated) EERClaymoreState State = EERClaymoreState::Arming;
    UPROPERTY(Replicated) double ArmAt = 0;
    UPROPERTY(Replicated) double ExpiresAt = 0;
    UPROPERTY(Config,EditDefaultsOnly) float ArmDelay = 2;
    UPROPERTY(Config,EditDefaultsOnly) float Lifetime = 60;
    UPROPERTY(Config,EditDefaultsOnly) float PlacementRange = 180;
    UPROPERTY(Config,EditDefaultsOnly,Replicated) float DetectionRange = 200;
    UPROPERTY(Config,EditDefaultsOnly,Replicated) float DetectionHalfAngle = 30;
    UPROPERTY(Config,EditDefaultsOnly) float ConeHalfAngle = 55;
    UPROPERTY(Config,EditDefaultsOnly) float DamageRadius = 800;
    UPROPERTY(Config,EditDefaultsOnly) float PeakDamage = 140;
    UPROPERTY(Config,EditDefaultsOnly) float ArmorRatio = 1.6f;
    UPROPERTY(Config,EditDefaultsOnly) float StructuralRadius = 150;
    UPROPERTY(Config,EditDefaultsOnly) float StructuralDamage = 30;
    static bool Placement(class AERCharacter* Character, FHitResult& Hit, FRotator& Rotation, FString* Failure = nullptr);
    static bool Place(class AERCharacter* Character);
    static void RemoveFor(UWorld* World, class AERPlayerState* Owner = nullptr);
    virtual void BeginPlay() override;
    virtual void Tick(float Delta) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual float TakeDamage(float Amount,const FDamageEvent& Event,AController* Instigator,AActor* Causer) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
private:
    friend class FERClaymoreLifecycleTest;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> LaserLeft;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> LaserRight;
    UPROPERTY() TObjectPtr<class UMaterialInstanceDynamic> LaserMaterial;
    void UpdateLasers();
    bool TriggerTarget(class AERCharacter* Target) const;
    TWeakObjectPtr<AActor> SupportActor;
    FVector SupportPoint;
    bool bCounted = false;
    bool VisibleTarget(class AERCharacter* Target) const;
    bool HasSupport() const;
    void Terminate();
    void Detonate();
};
