#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ERWounds.generated.h"

USTRUCT()
struct FERWound
{
    GENERATED_BODY()
    UPROPERTY() FName Bone;
    UPROPERTY() FVector Position = FVector::ZeroVector;
    UPROPERTY() FRotator Rotation = FRotator::ZeroRotator;
    UPROPERTY() uint8 Variant = 0;
    UPROPERTY() float Size = 4.f;
};

// Cosmetic, bounded state: bone-local coordinates survive animation and late joins.
UCLASS()
class FDM_API UERWounds : public UActorComponent
{
    GENERATED_BODY()
public:
    UERWounds();
    void Record(class USkeletalMeshComponent* Mesh, const FHitResult& Hit, bool bArmorOnly = false);
    void CopyFrom(const UERWounds* Source);
    void SetBody(class USkinnedMeshComponent* Mesh);
    const TArray<FERWound>& GetMarks() const { return Marks; }
    static float RollForVariant(uint8 Variant) { return Variant==1 ? 180.f : 0.f; }
    static constexpr int32 MaxMarks = 32;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
private:
    UPROPERTY(ReplicatedUsing=OnRep_Marks) TArray<FERWound> Marks;
    UPROPERTY() TArray<TSoftObjectPtr<class UMaterialInterface>> Materials;
    UPROPERTY(Transient) TObjectPtr<class USkinnedMeshComponent> Body;
    UPROPERTY(Transient) TArray<TObjectPtr<class UDecalComponent>> Decals;
    UFUNCTION() void OnRep_Marks();
    void ClearDecals();
};
