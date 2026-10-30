#include "ERMenuPreview.h"
#include "ERLoadout.h"
#include "ERWeapon.h"
#include "ERPostureMesh.h"
#include "ERWorldWeaponPose.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/PointLightComponent.h"
#include "Engine/TextureRenderTarget2D.h"
AERMenuPreview::AERMenuPreview()
{
    bReplicates = false;
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    Mesh = CreateDefaultSubobject<UERPostureMesh>(TEXT("Character"));
    Mesh->SetupAttachment(RootComponent);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    // CaptureScene runs before the normal end-of-frame cloth wait. Without
    // this, its dynamic render data can omit an unfinished cloth task and
    // display the skinned coat through the legs instead of simulated cloth.
    Mesh->bWaitForParallelClothTask=true;
    Capture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("Portrait"));
    Capture->SetupAttachment(RootComponent);
    Capture->SetRelativeLocation(FVector(320, 0, 20));
    Capture->SetRelativeRotation(FRotator(0, 180, 0));
    Capture->FOVAngle = 38;
    Capture->ShowFlags.SetMotionBlur(false);
    Capture->ShowFlags.SetBloom(false);
    Capture->ShowFlags.SetTonemapper(true);
    Capture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
    Capture->ShowFlags.SetAtmosphere(false);
    Capture->ShowFlags.SetFog(false);
    Capture->ShowFlags.SetVolumetricFog(false);
    Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
    Capture->bCaptureEveryFrame = false;
    Capture->bCaptureOnMovement = false;
    Capture->PostProcessSettings.bOverride_AutoExposureMethod = true;
    Capture->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
    Capture->PostProcessSettings.bOverride_AutoExposureBias = true;
    Capture->PostProcessSettings.AutoExposureBias = 0;
    Capture->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
    Capture->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure = false;
    Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("PortraitLight"));
    Light->SetupAttachment(RootComponent);
    Light->SetRelativeLocation(FVector(160, -100, 160));
    Light->Intensity = 45000;
    Light->AttenuationRadius = 700;
    Light->SetCastShadows(false);
    FillLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FillLight"));
    FillLight->SetupAttachment(RootComponent);
    FillLight->SetRelativeLocation(FVector(-150,100,80));
    FillLight->Intensity=20000;FillLight->AttenuationRadius=700;
    FillLight->SetCastShadows(false);
    Mesh->bEnableUpdateRateOptimizations=false;
    AddTickPrerequisiteComponent(Mesh);
}
void AERMenuPreview::ShowAppearance(const FERTeamAppearance& Appearance)
{
    if (!Target)
    {
        Target = NewObject<UTextureRenderTarget2D>(GetTransientPackage());
        Target->ClearColor = FLinearColor(.018f, .023f, .03f, 1);
        Target->InitAutoFormat(512, 640);
        Capture->TextureTarget = Target;
    }
    bAnimatePresentation=false;
    Mesh->PreviewWeapon=nullptr;
    bPreviewAvailable=Appearance.Mesh!=nullptr;Mesh->SetVisibility(bPreviewAvailable,true);
    Mesh->bPauseAnims=false;
    Appearance.Apply(Mesh, true);
    // Scene captures are outside the main camera: retain the selected skin detail.
    Mesh->bForceMipStreaming=true;
    Mesh->TickAnimation(0,false);Mesh->RefreshBoneTransforms();Mesh->UpdateComponentToWorld();
    Mesh->bPauseAnims=true;
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Capture->ShowOnlyActors.Empty();
    Capture->ShowOnlyActorComponents(this);
    Capture->CaptureScene();
}

UTextureRenderTarget2D* AERMenuPreview::MakePortrait(const FERTeamAppearance& Appearance)
{
    const auto SavedCamera=Capture->GetRelativeTransform();const float SavedFOV=Capture->FOVAngle;
    Capture->SetRelativeLocation(FVector(320,0,5));Capture->FOVAngle=38;
    const float SavedYaw=PresentationYaw;PresentationYaw=0;
    auto* Previous=Target.Get();Target=nullptr;ShowAppearance(Appearance);PresentationYaw=SavedYaw;
    auto* Portrait=Target.Get();Target=Previous;Capture->TextureTarget=Target;Capture->SetRelativeTransform(SavedCamera);Capture->FOVAngle=SavedFOV;return Portrait;
}
void AERMenuPreview::ShowLoadout(const FERTeamAppearance& Appearance,const FERResolvedLoadout& Kit,const TArray<TObjectPtr<UERWeaponData>>& Arsenal)
{
    for(auto Part:Equipment)if(Part)Part->DestroyComponent();Equipment.Empty();
    ShowAppearance(Appearance);
    if(!bPreviewAvailable || Kit.Items.IsEmpty())return;
    // Resolve the existing retargeted walk from this appearance's authored animation family.
    FString WalkPath=Appearance.CorpsePose ? Appearance.CorpsePose->GetPathName():FString();
    WalkPath.ReplaceInline(TEXT("MF_Rifle_Idle_ADS"),TEXT("MF_Unarmed_Walk_Fwd"));
    WalkAnimation=LoadObject<UAnimSequence>(nullptr,*WalkPath);
    const auto* Data=Kit.Items[0].Data.Get();
    bPreviewAvailable=WalkAnimation && Data && Data->Mesh;
    if(!bPreviewAvailable)return;
    Mesh->PreviewWeapon=Kit.Items[0].Data;
    Mesh->PreviewAppearance=Appearance;
    Mesh->PlayAnimation(WalkAnimation,true);
    Mesh->bPauseAnims=false;
    if(auto* Anim=Mesh->GetSingleNodeInstance())Anim->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
    Mesh->TickAnimation(0,false);Mesh->RefreshBoneTransforms();Mesh->UpdateComponentToWorld();
    auto* Part=NewObject<USkeletalMeshComponent>(this);AddInstanceComponent(Part);
    Part->SetSkeletalMeshAsset(Data->Mesh);Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Part->SetSimulatePhysics(false);Part->SetCastShadow(false);Part->RegisterComponent();
    const FName Grip=Data->bWorldReferencePose ? TEXT("ER_Palm_R"):TEXT("HandGrip_R");
    bPreviewAvailable=Mesh->DoesSocketExist(Grip);
    Part->AttachToComponent(Mesh,FAttachmentTransformRules::SnapToTargetNotIncludingScale,Grip);
    Part->SetRelativeTransform(Data->bWorldReferencePose ? ERWorldWeaponPose::Attachment(*Data,Mesh->GetSocketTransform(Grip).GetScale3D().X):Data->GripTransform);
    if(Data->WeaponIdleAnimation)Part->PlayAnimation(Data->WeaponIdleAnimation,true);
    Equipment.Add(Part);
    Capture->FOVAngle=38;
    Target->ResizeTarget(768,1024);
    FitPresentation();
    bAnimatePresentation=true;
    Capture->ShowOnlyActorComponents(this);
    Tick(0);
}
bool AERMenuPreview::IsWalking() const
{ return bAnimatePresentation && WalkAnimation && Mesh->GetSingleNodeInstance() && !Mesh->bPauseAnims; }
void AERMenuPreview::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if(!bAnimatePresentation)return;
    // A complete orbit takes 45 seconds. The mesh stays planted and walks in place.
    if(!bDragging)PresentationYaw=FMath::UnwindDegrees(PresentationYaw+DeltaSeconds*8.f);
    const FVector View=FRotator(0,PresentationYaw,0).RotateVector(FVector(OrbitDistance,0,12));
    Capture->SetRelativeLocation(View);
    Capture->SetRelativeRotation((-View).Rotation());
    Light->SetRelativeLocation(FRotator(0,PresentationYaw,0).RotateVector(FVector(160,-100,160)));
    FillLight->SetRelativeLocation(FRotator(0,PresentationYaw,0).RotateVector(FVector(-120,150,100)));
    Capture->CaptureScene();
}
void AERMenuPreview::Rotate(float Degrees)
{ PresentationYaw=FMath::UnwindDegrees(PresentationYaw+Degrees);Tick(0); }

void AERMenuPreview::FitPresentation()
{
    // Reserve the full walk envelope and primary weapon throughout the camera orbit.
    // Bone samples exclude hidden vendor props that would dwarf the visible body.
    TArray<FVector> Points;
    for(const FName Bone:{FName(TEXT("head")),FName(TEXT("foot_l")),FName(TEXT("foot_r")),FName(TEXT("hand_l")),FName(TEXT("hand_r"))})
        if(Mesh->DoesSocketExist(Bone))Points.Add(GetActorTransform().InverseTransformPosition(Mesh->GetSocketLocation(Bone)));
    Points.Add(FVector(0,0,95));Points.Add(FVector(0,0,-100));
    for(auto Part:Equipment)if(Part)
    {
        Part->UpdateBounds();const FBox B=Part->Bounds.GetBox();
        for(int32 Mask=0;Mask<8;++Mask)
            Points.Add(GetActorTransform().InverseTransformPosition(FVector(Mask&1?B.Max.X:B.Min.X,Mask&2?B.Max.Y:B.Min.Y,Mask&4?B.Max.Z:B.Min.Z)));
    }
    const double H=FMath::Tan(FMath::DegreesToRadians(Capture->FOVAngle*.5f));
    const double V=H*1024./768.;double Distance=240;
    for(const auto& P:Points)
    {
        const double Radius=FVector2D(P.X,P.Y).Size()+8;
        Distance=FMath::Max(Distance,FMath::Max(Radius+1.06*FMath::Abs(P.Z-12)/V,1.06*Radius/H));
    }
    bFramingValid=FMath::IsFinite(Distance) && Distance<2000;
    if(!bFramingValid)bPreviewAvailable=false;
    OrbitDistance=FMath::Clamp(Distance,240.,2000.);
}
