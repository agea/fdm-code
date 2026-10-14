#pragma once
#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "ERPostureMesh.generated.h"

// Procedural prone presentation layered over each team's existing rifle locomotion.
UCLASS()
class FDM_API UERPostureMesh : public USkeletalMeshComponent
{
    GENERATED_BODY()
public:
    virtual void FinalizeBoneTransform() override;
};
