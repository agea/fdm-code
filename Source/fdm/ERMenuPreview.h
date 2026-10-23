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
    void ShowLoadout(const FERTeamAppearance& Appearance,const struct FERResolvedLoadout& Kit,const TArray<TObjectPtr<class UERWeaponData>>& Arsenal);
    void Rotate(float Degrees);
    bool bPreviewAvailable = false;
    bool bFramingValid = false;
    class UTextureRenderTarget2D* MakePortrait(const FERTeamAppearance& Appearance);
    UPROPERTY() TArray<TObjectPtr<class UMeshComponent>> Equipment;
    UPROPERTY() TObjectPtr<class UTextureRenderTarget2D> Target;
private:
    float PresentationYaw = 0;
    void FitPresentation();
    UPROPERTY() TObjectPtr<class USkeletalMeshComponent> Mesh;
    UPROPERTY() TObjectPtr<class USceneCaptureComponent2D> Capture;
    UPROPERTY() TObjectPtr<class UPointLightComponent> Light;
};
