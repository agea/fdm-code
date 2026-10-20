#include "ERMenuPreview.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/PointLightComponent.h"
#include "Engine/TextureRenderTarget2D.h"
AERMenuPreview::AERMenuPreview()
{
    bReplicates = false;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Character"));
    Mesh->SetupAttachment(RootComponent);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    Capture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("Portrait"));
    Capture->SetupAttachment(RootComponent);
    Capture->SetRelativeLocation(FVector(320, 0, 5));
    Capture->SetRelativeRotation(FRotator(0, 180, 0));
    Capture->FOVAngle = 38;
    Capture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
    Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
    Capture->bCaptureEveryFrame = true;
    Capture->PostProcessSettings.bOverride_AutoExposureMethod = true;
    Capture->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
    Capture->PostProcessSettings.bOverride_AutoExposureBias = true;
    Capture->PostProcessSettings.AutoExposureBias = 0;
    Capture->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
    Capture->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure = false;
    Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("PortraitLight"));
    Light->SetupAttachment(RootComponent);
    Light->SetRelativeLocation(FVector(160, -100, 160));
    Light->Intensity = 12000;
    Light->AttenuationRadius = 700;
    Light->SetCastShadows(false);
}
void AERMenuPreview::ShowAppearance(const FERTeamAppearance& Appearance)
{
    if (!Target)
    {
        Target = NewObject<UTextureRenderTarget2D>(this);
        Target->ClearColor = FLinearColor(.018f, .023f, .03f, 1);
        Target->InitAutoFormat(512, 640);
        Capture->TextureTarget = Target;
    }
    Appearance.Apply(Mesh, true);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Capture->ShowOnlyActors.Empty();
    Capture->ShowOnlyActorComponents(this);
    Capture->CaptureScene();
}
