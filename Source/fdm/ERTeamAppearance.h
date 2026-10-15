#pragma once
#include "CoreMinimal.h"
#include "ERTeamAppearance.generated.h"

// Asset references live in Blueprint defaults. PlayerState supplies the team on
// every peer, including late joiners; the presentation never changes collision.
USTRUCT(BlueprintType)
struct FDM_API FERAppearanceAttachment
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<class UStaticMesh> Mesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<class UMaterialInterface> Material;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Bone;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FTransform Transform;
};

USTRUCT(BlueprintType)
struct FDM_API FERTeamAppearance
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<class USkeletalMesh> Mesh;
    // Skin geometry containing only forearms/hands, rebound to the existing FPS rig.
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<class USkeletalMesh> FirstPersonMesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TSubclassOf<class UAnimInstance> AnimationClass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<class UAnimSequence> CorpsePose;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<TObjectPtr<class UMaterialInterface>> Materials;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FName> HiddenBones;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<int32> HiddenMaterialSlots;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FERAppearanceAttachment> Attachments;
    // Render this corpse from explicit post-physics bone transforms.
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bCopyPhysicsPoseForRendering = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Scale = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float FeetOffset = -92.5f;
    void Apply(class USkeletalMeshComponent* Component, bool bCorpse = false) const;
    bool Matches(class USkeletalMeshComponent* Component, bool bCorpse = false) const;
};

// Every team uses the same ten stable selection indices; references remain in BP defaults.
namespace ERSkins
{
    constexpr int32 Count = 10;
    inline int32 ValidIndex(int32 Value) { return Value >= 0 && Value < Count ? Value : 0; }
    inline const FERTeamAppearance& Select(const TArray<FERTeamAppearance>& Choices, const FERTeamAppearance& Fallback, int32 Index)
    { return Choices.IsValidIndex(ValidIndex(Index)) ? Choices[ValidIndex(Index)] : Fallback; }
}
