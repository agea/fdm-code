#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERMatch.h"
#include "ERTeamAppearance.h"
#include "Components/SkeletalMeshComponent.h"
#include "ERCorpse.generated.h"

UCLASS()
class FDM_API UERCorpseMeshComponent : public USkeletalMeshComponent
{
    GENERATED_BODY()
public:
    virtual void FinalizeBoneTransform() override;
    void FreezeAfterPhysicsPose() { bFreezeAfterNextPose = true; }
private:
    bool bFreezeAfterNextPose = false;
};

UCLASS()
class FDM_API AERCorpse : public AActor
{
    GENERATED_BODY()
public:
    AERCorpse();
    UFUNCTION(BlueprintCallable, Category="Team appearance|Editor")
    static bool BuildTeamRagdollPhysics(class UPhysicsAsset* Asset, class USkeletalMesh* Mesh, float LimbSwing = 65.f, float LegSwing = 65.f);
    bool HasValidRagdollPose() const;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<class USkeletalMeshComponent> Body;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite) FERTeamAppearance AlphaAppearance;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite) FERTeamAppearance OmegaAppearance;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite) TArray<FERTeamAppearance> AlphaSkins;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite) TArray<FERTeamAppearance> OmegaSkins;
    UPROPERTY(Replicated) int32 SkinIndex = 0;
    const FERTeamAppearance& GetTeamAppearance() const;
    UFUNCTION(BlueprintCallable, Category="Team appearance|Editor")
    static bool BuildFieldGear(class UStaticMesh* Mesh, bool bHelmet, bool bOutwardFacing = false);
    UFUNCTION(BlueprintCallable, Category="Team appearance|Editor")
    static FERAppearanceAttachment MakeFieldAttachment(class UStaticMesh* Gear, class UMaterialInterface* Material, class USkeletalMesh* Character, FName Bone, FVector Offset, FVector Scale);
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite) TArray<TObjectPtr<class UMaterialInterface>> AlphaMaterials;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite) TArray<TObjectPtr<class UMaterialInterface>> OmegaMaterials;
    UPROPERTY(Replicated) EERTeam Team = EERTeam::None;
    UPROPERTY(Replicated) FVector_NetQuantizeNormal ImpactDirection = FVector::ForwardVector;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
private:
    UPROPERTY() TObjectPtr<class UPoseableMeshComponent> PoseBody;
    bool bFinishPosePending = false;
    FTimerHandle SettleTimer;
    void Settle();
};
