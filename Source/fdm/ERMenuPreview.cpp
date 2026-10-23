#include "ERMenuPreview.h"
#include "ERLoadout.h"
#include "ERWeapon.h"
#include "ERClaymore.h"
#include "Components/StaticMeshComponent.h"
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
    Capture->SetRelativeLocation(FVector(320, 0, 20));
    Capture->SetRelativeRotation(FRotator(0, 180, 0));
    Capture->FOVAngle = 38;
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
    Light->Intensity = 12000;
    Light->AttenuationRadius = 700;
    Light->SetCastShadows(false);
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
    bPreviewAvailable=Appearance.Mesh!=nullptr;Mesh->SetVisibility(bPreviewAvailable,true);
    Mesh->bPauseAnims=false;
    Appearance.Apply(Mesh, true);
    Mesh->AddLocalRotation(FRotator(0,PresentationYaw,0));
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
    // Explicit frozen presentation pose: no gameplay animation instance or pawn state.
    Mesh->SetPosition(0);Mesh->bPauseAnims=true;
    int32 Stand=0;
    for(int32 I=0;I<Kit.Items.Num();++I)
    {
        const int32 Index=Kit.Items[I].WeaponKey;
        if(Index==0)continue;
        const auto* Data=Kit.Items[I].Data.Get();
        if(!Data || !Data->Mesh) { bPreviewAvailable=false;continue; }
        auto* Part=NewObject<USkeletalMeshComponent>(this);AddInstanceComponent(Part);
        Part->SetSkeletalMeshAsset(Data->Mesh);Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->SetSimulatePhysics(false);Part->RegisterComponent();
        if(I==0 && Mesh->DoesSocketExist(TEXT("HandGrip_R")))
        {
            Part->AttachToComponent(Mesh,FAttachmentTransformRules::SnapToTargetNotIncludingScale,TEXT("HandGrip_R"));
            Part->SetRelativeTransform(Data->GripTransform);
        }
        else
        {
            Part->AttachToComponent(RootComponent,FAttachmentTransformRules::KeepRelativeTransform);
            Part->SetRelativeLocation(FVector(0,75,-55+Stand++*28));Part->SetRelativeRotation(Index==10?FRotator::ZeroRotator:FRotator(0,0,70));
        }
        if(Data->WeaponIdleAnimation) { Part->PlayAnimation(Data->WeaponIdleAnimation,false);Part->SetPosition(0);Part->bPauseAnims=true; }
        Equipment.Add(Part);
    }
    // Simple display supports belong to the cosmetic scene, never to inventory.
    for(int32 Shelf=0;Shelf<Stand;++Shelf)
    {
        auto* Plinth=NewObject<UStaticMeshComponent>(this);AddInstanceComponent(Plinth);
        Plinth->SetStaticMesh(GetDefault<AERClaymore>()->Body->GetStaticMesh());
        Plinth->SetCollisionEnabled(ECollisionEnabled::NoCollision);Plinth->RegisterComponent();
        Plinth->AttachToComponent(RootComponent,FAttachmentTransformRules::KeepRelativeTransform);
        Plinth->SetRelativeLocation(FVector(0,75,-60+Shelf*28));Plinth->SetRelativeScale3D(FVector(.35f,.45f,.025f));Equipment.Add(Plinth);
    }
    Capture->FOVAngle=48;FitPresentation();Capture->ShowOnlyActorComponents(this);Capture->CaptureScene();
}
void AERMenuPreview::Rotate(float Degrees)
{ PresentationYaw=FMath::UnwindDegrees(PresentationYaw+Degrees);Mesh->AddLocalRotation(FRotator(0,Degrees,0));FitPresentation();Capture->CaptureScene(); }

void AERMenuPreview::FitPresentation()
{
    // Fit the actual weapon/display bounds, rather than assuming all barrels fit
    // the old character-only portrait. Hidden Paragon props do not size the view.
    TArray<FVector> Points;
    Points.Add(GetActorTransform().InverseTransformPosition(Mesh->GetComponentLocation()));
    Points.Add(FVector(0,0,110));
    for(auto Part:Equipment)if(Part)
    {
        Part->UpdateBounds();const FBox B=Part->Bounds.GetBox();
        for(int32 Mask=0;Mask<8;++Mask)
            Points.Add(GetActorTransform().InverseTransformPosition(FVector(Mask&1?B.Max.X:B.Min.X,Mask&2?B.Max.Y:B.Min.Y,Mask&4?B.Max.Z:B.Min.Z)));
    }
    const double H=FMath::Tan(FMath::DegreesToRadians(Capture->FOVAngle*.5f));
    const double V=H*640./512.;double Distance=320;
    for(const auto& P:Points)Distance=FMath::Max(Distance,P.X+1.1*FMath::Max(FMath::Abs(P.Y)/H,FMath::Abs(P.Z-20)/V));
    bFramingValid=FMath::IsFinite(Distance) && Distance<2000;
    if(!bFramingValid)bPreviewAvailable=false;
    Capture->SetRelativeLocation(FVector(FMath::Clamp(Distance,320.,2000.),0,20));
}
