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
    virtual void Tick(float DeltaSeconds) override;
    bool IsWalking() const;
    float GetOrbitYaw() const { return PresentationYaw; }
    void ShowAppearance(const FERTeamAppearance& Appearance);
    void ShowLoadout(const FERTeamAppearance& Appearance,const struct FERResolvedLoadout& Kit,const TArray<TObjectPtr<class UERWeaponData>>& Arsenal);
    void Rotate(float Degrees);
    void SetDragging(bool Dragging) { bDragging=Dragging; }
    bool bPreviewAvailable = false;
    bool bFramingValid = false;
    class UTextureRenderTarget2D* MakePortrait(const FERTeamAppearance& Appearance);
    UPROPERTY() TArray<TObjectPtr<class UMeshComponent>> Equipment;
    UPROPERTY() TObjectPtr<class UTextureRenderTarget2D> Target;
private:
    friend class AERFieldClothingProbe;
    float PresentationYaw = -25;
    float OrbitDistance = 300;
    bool bDragging = false;
    bool bAnimatePresentation = false;
    UPROPERTY() TObjectPtr<class UAnimSequence> WalkAnimation;
    void FitPresentation();
    UPROPERTY() TObjectPtr<class UERPostureMesh> Mesh;
    UPROPERTY() TObjectPtr<class USceneCaptureComponent2D> Capture;
    UPROPERTY() TObjectPtr<class UPointLightComponent> Light;
    UPROPERTY() TObjectPtr<class UPointLightComponent> FillLight;
};
