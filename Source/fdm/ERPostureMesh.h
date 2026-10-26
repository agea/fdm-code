#pragma once
#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "ERPostureMesh.generated.h"

// Procedural prone and weapon support-hand fitting over the team locomotion.
UCLASS()
class FDM_API UERPostureMesh : public USkeletalMeshComponent
{
    GENERATED_BODY()
public:
    virtual void TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* ThisTickFunction) override;
    virtual void FinalizeBoneTransform() override;
    float GetWorldAimBlend() const {return WorldAimBlend;}
private:
    float WorldAimBlend=0;
};
