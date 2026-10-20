#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERTeamAppearance.h"
#include "ERMenuPreview.generated.h"

// Local, collision-free dressing room. Never a replicated gameplay pawn.
UCLASS(Transient, NotBlueprintable)
class AERMenuPreview : public AActor
{
    GENERATED_BODY()
public:
    AERMenuPreview();
    void ShowAppearance(const FERTeamAppearance& Appearance);
    UPROPERTY() TObjectPtr<class UTextureRenderTarget2D> Target;
private:
    UPROPERTY() TObjectPtr<class USkeletalMeshComponent> Mesh;
    UPROPERTY() TObjectPtr<class USceneCaptureComponent2D> Capture;
    UPROPERTY() TObjectPtr<class UPointLightComponent> Light;
};
