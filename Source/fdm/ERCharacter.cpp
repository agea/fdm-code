#include "ERCharacter.h"
#include "ERPostureMesh.h"
#include "ERCorpse.h"
#include "Engine/DamageEvents.h"
#include "ERMatch.h"
#include "ERWeapon.h"
#include "ERWeaponBallistics.h"
#include "ERMovement.h"
#include "ERPlayerController.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Net/UnrealNetwork.h"
#include "DrawDebugHelpers.h"
#include "Animation/AnimSequence.h"
#include "ERSmokeCloud.h"
#include "ERShotEffects.h"
#include "EngineUtils.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalMeshSocket.h"
#if WITH_EDITOR
#include "MeshDescription.h"
#include "SkeletalMeshAttributes.h"
#include "Animation/Skeleton.h"
#endif

bool AERCharacter::CopyTrainingGripSockets(USkeletalMesh* TargetMesh, USkeletalMesh* SourceMesh)
{
#if WITH_EDITOR
    if (!TargetMesh || !SourceMesh || TargetMesh == SourceMesh ||
        !TargetMesh->GetPathName().StartsWith(TEXT("/Game/EclipseRecoil/Characters/"))) return false;
    TargetMesh->Modify();
    for (const FName Name : {FName(TEXT("HandGrip_R")), FName(TEXT("HandGrip_L"))})
    {
        if (TargetMesh->FindSocket(Name)) continue;
        const USkeletalMeshSocket* Source = SourceMesh->FindSocket(Name);
        if (!Source || TargetMesh->GetRefSkeleton().FindBoneIndex(Source->BoneName) == INDEX_NONE) return false;
        TargetMesh->AddSocket(DuplicateObject<USkeletalMeshSocket>(Source, TargetMesh));
    }
    TargetMesh->PostEditChange();
    return true;
#else
    return false;
#endif
}
FTransform AERCharacter::GetWeaponReferenceBone(USkeletalMesh* WeaponMesh, FName Bone)
{
    if (!WeaponMesh) return FTransform::Identity;
    const FReferenceSkeleton& Ref = WeaponMesh->GetRefSkeleton();
    int32 Index = Ref.FindBoneIndex(Bone);
    if (Index == INDEX_NONE) return FTransform::Identity;
    FTransform Result = Ref.GetRefBonePose()[Index];
    while ((Index = Ref.GetParentIndex(Index)) != INDEX_NONE) Result *= Ref.GetRefBonePose()[Index];
    return Result;
}
TArray<FVector> AERCharacter::GetPresentationVertices(USkeletalMesh* PresentationMesh, FName Bone)
{
    TArray<FVector> Result;
#if WITH_EDITOR
    if (!PresentationMesh) return Result;
    FMeshDescription* Description=PresentationMesh->GetMeshDescription(0);
    if (!Description) return Result;
    FSkeletalMeshAttributes Attributes(*Description);
    const auto Positions=Attributes.GetVertexPositions();
    const auto Weights=Attributes.GetVertexSkinWeights();
    int32 DescriptionBone=INDEX_NONE;
    for (const FBoneID ID : Attributes.Bones().GetElementIDs())
        if (Attributes.GetBoneNames()[ID]==Bone) DescriptionBone=ID.GetValue();
    const FTransform Frame=Bone.IsNone() ? FTransform::Identity : GetWeaponReferenceBone(PresentationMesh,Bone);
    for (const FVertexID Vertex : Description->Vertices().GetElementIDs())
    {
        bool Include=Bone.IsNone();
        if (!Include) for (const auto Weight : Weights.Get(Vertex))
            if (Weight.GetBoneIndex()==DescriptionBone && Weight.GetWeight()>.65f) Include=true;
        if (Include) Result.Add(Frame.InverseTransformPosition(FVector(Positions[Vertex])));
    }
#endif
    return Result;
}
bool AERCharacter::SetPalmContactSocket(USkeletalMesh* PresentationMesh, FName Name, FName Bone, FTransform Transform)
{
#if WITH_EDITOR
    if (!PresentationMesh || !PresentationMesh->GetPathName().StartsWith(TEXT("/Game/EclipseRecoil/Characters/")) ||
        (Name!=TEXT("ER_Palm_L") && Name!=TEXT("ER_Palm_R")) || PresentationMesh->GetRefSkeleton().FindBoneIndex(Bone)==INDEX_NONE) return false;
    PresentationMesh->Modify();
    auto* Socket=const_cast<USkeletalMeshSocket*>(PresentationMesh->FindSocket(Name));
    if (!Socket) {Socket=NewObject<USkeletalMeshSocket>(PresentationMesh);Socket->SocketName=Name;PresentationMesh->AddSocket(Socket);}
    Socket->BoneName=Bone;Socket->RelativeLocation=Transform.GetTranslation();Socket->RelativeRotation=Transform.Rotator();Socket->RelativeScale=FVector::OneVector;
    PresentationMesh->RebuildSocketMap();PresentationMesh->MarkPackageDirty();return true;
#else
    return false;
#endif
}
#if WITH_EDITOR
namespace
{
TArray<FVector> PresentationVerticesAtIdle(USkeletalMesh* WeaponMesh,FMeshDescription* Description,UAnimSequence* Animation)
{
        FSkeletalMeshAttributes Attributes(*Description);
        const auto Positions=Attributes.GetVertexPositions();
        TArray<FVector> Deformed;Deformed.SetNum(Description->Vertices().GetArraySize());
        const auto& Ref=WeaponMesh->GetRefSkeleton();
        TArray<FTransform> Bind=Ref.GetRefBonePose(),Current=Bind;
        for (int32 I=0;I<Bind.Num();++I)
        {
            if (Animation && Animation->GetSkeleton())
            {
                const int32 Index=Animation->GetSkeleton()->GetReferenceSkeleton().FindBoneIndex(Ref.GetBoneName(I));
                if (Index!=INDEX_NONE) Animation->GetBoneTransform(Current[I],FSkeletonPoseBoneIndex(Index),FAnimExtractContext(0.0),false);
            }
            const int32 Parent=Ref.GetParentIndex(I);
            if (Parent!=INDEX_NONE) {Bind[I]*=Bind[Parent];Current[I]*=Current[Parent];}
        }
        TArray<int32> Bones;Bones.Init(INDEX_NONE,Attributes.Bones().GetArraySize());
        for (const FBoneID ID : Attributes.Bones().GetElementIDs()) Bones[ID.GetValue()]=Ref.FindBoneIndex(Attributes.GetBoneNames()[ID]);
        const auto Weights=Attributes.GetVertexSkinWeights();
        for (const FVertexID Vertex : Description->Vertices().GetElementIDs())
        {
            const FVector Position(Positions[Vertex]);FVector Point=FVector::ZeroVector;
            if (Animation) for (const auto Weight : Weights.Get(Vertex))
            {
                const int32 Bone=Bones[Weight.GetBoneIndex()];
                if (Bone!=INDEX_NONE) Point+=Current[Bone].TransformPosition(Bind[Bone].InverseTransformPosition(Position))*Weight.GetWeight();
            }
            Deformed[Vertex.GetValue()]=Animation ? Point : Position;
        }
        return Deformed;
}
}
#endif
FVector AERCharacter::GetWeaponSurfaceContact(USkeletalMesh* WeaponMesh, FVector Start, FVector End, UAnimSequence* Animation)
{
#if WITH_EDITOR
    if (WeaponMesh) if (FMeshDescription* Description=WeaponMesh->GetMeshDescription(0))
    {
        const auto Deformed=PresentationVerticesAtIdle(WeaponMesh,Description,Animation);
        FVector Best(UE_BIG_NUMBER);float Distance=UE_BIG_NUMBER;
        for (const FTriangleID Triangle : Description->Triangles().GetElementIDs())
        {
            const auto Vertices=Description->GetTriangleVertices(Triangle);
            FVector Point,Normal;
            if (FMath::SegmentTriangleIntersection(Start,End,Deformed[Vertices[0].GetValue()],Deformed[Vertices[1].GetValue()],Deformed[Vertices[2].GetValue()],Point,Normal) &&
                FVector::DistSquared(Start,Point)<Distance)
            {Best=Point;Distance=FVector::DistSquared(Start,Point);}
        }
        return Best;
    }
#endif
    return FVector(UE_BIG_NUMBER);
}
FVector AERCharacter::GetWeaponMuzzleFromGeometry(USkeletalMesh* WeaponMesh,UAnimSequence* Animation)
{
#if WITH_EDITOR
    if (!WeaponMesh) return FVector::ZeroVector;
    FMeshDescription* Description = WeaponMesh->GetMeshDescription(0);
    if (!Description || Description->Vertices().Num() == 0) return FVector::ZeroVector;
    const auto Positions=PresentationVerticesAtIdle(WeaponMesh,Description,Animation);
    double Front=-DBL_MAX;
    for (const FVertexID Vertex : Description->Vertices().GetElementIDs()) Front=FMath::Max(Front,Positions[Vertex.GetValue()].X);
    FBox TipBounds(ForceInit);
    // Include the evaluated ready pose when measuring visual framing.
    for (const FVertexID Vertex : Description->Vertices().GetElementIDs())
        if (Positions[Vertex.GetValue()].X>=Front-.4) TipBounds+=Positions[Vertex.GetValue()];
    return FVector(Front, TipBounds.GetCenter().Y, TipBounds.GetCenter().Z);
#else
    return FVector::ZeroVector;
#endif
}
bool AERCharacter::AssignWeaponMaterial(USkeletalMesh* WeaponMesh, UMaterialInterface* Material)
{
#if WITH_EDITOR
    if (!WeaponMesh || !Material || !WeaponMesh->GetPathName().StartsWith(TEXT("/Game/EclipseRecoil/Weapons/"))) return false;
    WeaponMesh->Modify();
    TArray<FSkeletalMaterial> Materials = WeaponMesh->GetMaterials();
    for (auto& Slot : Materials) Slot.MaterialInterface = Material;
    WeaponMesh->SetMaterials(Materials);
    WeaponMesh->PostEditChange();
    WeaponMesh->MarkPackageDirty();
    return true;
#else
    return false;
#endif
}
bool AERCharacter::AssignWeaponMaterials(USkeletalMesh* WeaponMesh, const TArray<UMaterialInterface*>& Interfaces)
{
#if WITH_EDITOR
    if (!WeaponMesh || !WeaponMesh->GetPathName().StartsWith(TEXT("/Game/EclipseRecoil/Weapons/Sketchfab/")) || Interfaces.Num()!=WeaponMesh->GetMaterials().Num()) return false;
    TArray<FSkeletalMaterial> Materials=WeaponMesh->GetMaterials();
    for (int32 Index=0;Index<Materials.Num();++Index)
    {
        if (!Interfaces[Index]) return false;
        Materials[Index].MaterialInterface=Interfaces[Index];
    }
    WeaponMesh->Modify();WeaponMesh->SetMaterials(Materials);WeaponMesh->PostEditChange();WeaponMesh->MarkPackageDirty();
    return true;
#else
    return false;
#endif
}

bool AERCharacter::SetMercenaryAnimationSkeleton(UAnimSequence* Animation, USkeletalMesh* TargetMesh)
{
#if WITH_EDITOR
    if (!Animation || !TargetMesh || !TargetMesh->GetSkeleton() ||
        !(Animation->GetPathName().StartsWith(TEXT("/Game/EclipseRecoil/Characters/")) ||
          Animation->GetPathName().StartsWith(TEXT("/Game/EclipseRecoil/Weapons/Sketchfab/")))) return false;
    Animation->Modify();
    if (Animation->GetSkeleton() != TargetMesh->GetSkeleton() && !Animation->ReplaceSkeleton(TargetMesh->GetSkeleton(), false)) return false;
    Animation->SetPreviewMesh(TargetMesh);
    Animation->PostEditChange();
    return true;
#else
    return false;
#endif
}

UERHealthComponent::UERHealthComponent() { SetIsReplicatedByDefault(true); }
bool UERHealthComponent::IsProtected() const
{
    const auto* State = GetWorld()->GetGameState<AERGameState>();
    return State && State->GetServerWorldTimeSeconds() < ProtectionEndServerTime;
}
float UERHealthComponent::ReceiveDamage(float Amount, AController* Killer, float ArmorRatio)
{
    auto* Character = Cast<AERCharacter>(GetOwner());
    auto* Mode = GetWorld()->GetAuthGameMode<AERGameMode>();
    if (!Character || !Character->HasAuthority() || !Mode || !Mode->IsMatchInProgress() || bDead || IsProtected() || Amount <= 0) return 0;
    Amount = ERWeaponBallistics::AbsorbArmor(Amount, ArmorRatio, Armor);
    const float Before = Health;
    Health = FMath::Clamp(Health - Amount, 0.f, 100.f);
    UE_LOG(LogTemp, Display, TEXT("ER_DAMAGE target=%s hp=%.0f"), *Character->GetName(), Health);
    if (Health <= 0)
    {
        bDead = true;
        if (Character->Weapon) Character->Weapon->Cancel();
        OnRep_Dead();
        Mode->RecordDeath(Character, Killer);
    }
    return Before - Health;
}
void UERHealthComponent::OnRep_Dead() { if (bDead) if (auto* Character = Cast<AERCharacter>(GetOwner())) Character->ApplyDeathVisuals(); }
void UERHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UERHealthComponent, bHelmet); DOREPLIFETIME(UERHealthComponent, Health); DOREPLIFETIME(UERHealthComponent, bDead); DOREPLIFETIME(UERHealthComponent, ProtectionEndServerTime);
    DOREPLIFETIME(UERHealthComponent, Armor);
}
AERCharacter::AERCharacter(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<UERMovement>(ACharacter::CharacterMovementComponentName).SetDefaultSubobjectClass<UERPostureMesh>(ACharacter::MeshComponentName))
{
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> GrayBase(TEXT("/Engine/EngineDebugMaterials/LevelColorationLitMaterial.LevelColorationLitMaterial"));
    ProtectionBaseMaterial = GrayBase.Object;
    bReplicates = true;
    PrimaryActorTick.bCanEverTick = true;
    GetCapsuleComponent()->InitCapsuleSize(34, 92.5f);
    GetCharacterMovement()->MaxWalkSpeed = 1000.f / 3.6f;
    GetCharacterMovement()->JumpZVelocity = 460;
    GetCharacterMovement()->AirControl = 0.2f;
    GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;
    GetCharacterMovement()->MaxWalkSpeedCrouched = 100;
    BaseEyeHeight = 77.5f; CrouchedEyeHeight = 50;
    bUseControllerRotationYaw = true;
    Health = CreateDefaultSubobject<UERHealthComponent>(TEXT("Health"));
    ProneHitVolume = CreateDefaultSubobject<UCapsuleComponent>(TEXT("ProneHitVolume"));
    ProneHitVolume->SetupAttachment(GetCapsuleComponent());
    ProneHitVolume->InitCapsuleSize(22,105);
    ProneHitVolume->SetRelativeLocation(FVector(30,0,-9));
    ProneHitVolume->SetRelativeRotation(FRotator(90,0,0));
    ProneHitVolume->SetCollisionObjectType(ECC_Pawn);
    ProneHitVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
    ProneHitVolume->SetCollisionResponseToChannel(ECC_GameTraceChannel1,ECR_Block);
    ProneHitVolume->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
    ProneHitVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ProneHitVolume->SetGenerateOverlapEvents(false);
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(GetCapsuleComponent());
    Camera->SetRelativeLocation(FVector(0, 0, 77.5f));
    Camera->bUsePawnControlRotation = true;
    Camera->FieldOfView = 90;
    GetMesh()->SetRelativeLocation(FVector(0, 0, -92.5f));
    GetMesh()->SetRelativeRotation(FRotator(0, -90, 0));
    GetMesh()->SetOwnerNoSee(true);
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FirstPersonBody = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonBody"));
    FirstPersonBody->SetupAttachment(Camera);
    FirstPersonBody->SetRelativeLocation(FVector(-10, 0, -152));
    FirstPersonBody->SetRelativeRotation(FRotator(0, -90, 0));
    FirstPersonBody->SetOnlyOwnerSee(true);
    FirstPersonBody->SetCastShadow(false);
    FirstPersonBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FirstPersonRifle = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonRifle"));
    FirstPersonRifle->SetupAttachment(FirstPersonBody, TEXT("HandGrip_R"));
    FirstPersonRifle->SetOnlyOwnerSee(true);
    FirstPersonRifle->SetCastShadow(false);
    FirstPersonRifle->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WorldRifle = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WorldRifle"));
    WorldRifle->SetupAttachment(GetMesh(), TEXT("HandGrip_R"));
    FirstPersonSight = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonSight"));
    FirstPersonSight->SetupAttachment(FirstPersonRifle); FirstPersonSight->SetOnlyOwnerSee(true);
    WorldSight = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WorldSight"));
    WorldSight->SetupAttachment(WorldRifle); WorldSight->SetOwnerNoSee(true);
    for (auto* Sight : {FirstPersonSight.Get(), WorldSight.Get()})
    { Sight->SetCollisionEnabled(ECollisionEnabled::NoCollision); Sight->SetCastShadow(false); }
    WorldRifle->SetOwnerNoSee(true);
    WorldRifle->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FirstPersonMuzzleFlash = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FirstPersonMuzzleFlash"));
    FirstPersonMuzzleFlash->SetupAttachment(FirstPersonRifle, TEXT("Muzzle"));
    FirstPersonMuzzleFlash->SetOnlyOwnerSee(true);
    WorldMuzzleFlash = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WorldMuzzleFlash"));
    WorldMuzzleFlash->SetupAttachment(WorldRifle, TEXT("Muzzle"));
    WorldMuzzleFlash->SetOwnerNoSee(true);
    for (auto* Flash : {FirstPersonMuzzleFlash.Get(), WorldMuzzleFlash.Get()})
    {
        Flash->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Flash->SetCastShadow(false);
        Flash->SetVisibility(false);
    }
    TeamMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TeamMarker"));
    TeamMarker->SetupAttachment(GetCapsuleComponent());
    TeamMarker->SetRelativeLocation(FVector(0,0,110));
    TeamMarker->SetRelativeScale3D(FVector(0.18f));
    TeamMarker->SetOwnerNoSee(true);
    TeamMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WeaponClass = AERWeapon::StaticClass();
}
void AERCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
    Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
    Camera->SetRelativeLocation(FVector(0,0,BaseEyeHeight));
}
void AERCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
    Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
    Camera->SetRelativeLocation(FVector(0,0,BaseEyeHeight));
}
void AERCharacter::BeginPlay()
{
    Super::BeginPlay();
    FirstPersonBodyBaseLocation = FirstPersonBody->GetRelativeLocation();
    UERShotEffects::Prewarm(GetWorld(), TracerSystem, Camera->GetComponentLocation());
    // Compress only the viewmodel's rendered depth, preserving its framing and
    // socket transforms. Keep it inside the capsule clearance near walls.
    // Apply at runtime too: existing Blueprint component defaults are serialized.
    Camera->SetEnableFirstPersonScale(true);
    Camera->SetFirstPersonScale(.2f);
    Camera->SetEnableFirstPersonFieldOfView(true);
    Camera->SetFirstPersonFieldOfView(90.f);
    FirstPersonSight->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
    for (UPrimitiveComponent* Component : {static_cast<UPrimitiveComponent*>(FirstPersonBody.Get()),
        static_cast<UPrimitiveComponent*>(FirstPersonRifle.Get()),
        static_cast<UPrimitiveComponent*>(FirstPersonMuzzleFlash.Get())})
        Component->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
    // Existing Blueprint instances may still serialize the old camera attachment.
    // Align the rifle's GripPoint origin to the mannequin's matching grip socket.
    FirstPersonRifle->AttachToComponent(FirstPersonBody, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("HandGrip_R"));
    WorldRifle->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("HandGrip_R"));
    FirstPersonBody->HideBoneByName(TEXT("head"), EPhysBodyOp::PBO_None);
    if (!FirstPersonVisibleMaterialSlots.IsEmpty()) if (const auto* Mesh = FirstPersonBody->GetSkeletalMeshAsset())
        for (int32 LOD = 0; LOD < Mesh->GetLODNum(); ++LOD)
            for (int32 Slot = 0; Slot < Mesh->GetMaterials().Num(); ++Slot)
                FirstPersonBody->ShowMaterialSection(Slot, INDEX_NONE, FirstPersonVisibleMaterialSlots.Contains(Slot), LOD);
    for (auto* Flash : {FirstPersonMuzzleFlash.Get(), WorldMuzzleFlash.Get()})
    {
        Flash->SetStaticMesh(MuzzleFlashMesh);
        Flash->SetMaterial(0, MuzzleFlashMaterial);
        Flash->SetVisibility(false);
    }
    if (HasAuthority())
    {
        // Reuse anatomical ragdoll bodies for explicit shotgun queries only.
        // Other weapon/world/pawn collision channels keep their existing behaviour.
        GetMesh()->SetCollisionResponseToAllChannels(ECR_Ignore);
        GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        GetMesh()->SetGenerateOverlapEvents(false);
        GetMesh()->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
        Health->ProtectionEndServerTime = GetWorld()->GetTimeSeconds() + 4;
        // Direct map startup can assign the loadout before this pawn's BeginPlay.
        // Keep that weapon and its FPS pose instead of replacing it with the fallback.
        if (!Weapon)
        {
            FActorSpawnParameters Params; Params.Owner = this; Params.Instigator = this;
            Weapon = GetWorld()->SpawnActor<AERWeapon>(WeaponClass, GetActorTransform(), Params);
            if (Weapon) Weapon->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
        }
    }
    // Enforce this at runtime: existing Blueprints serialize the old visible marker.
    TeamMarker->SetHiddenInGame(true);
    TeamMarker->SetVisibility(false);
    TeamMarker->SetCastShadow(false);
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("ERViewmodelWallTest")))
    {
        FTimerHandle ProbeTimer;
        GetWorldTimerManager().SetTimer(ProbeTimer, [this]()
        {
            if (!IsLocallyControlled() || !Controller) return;
            FHitResult Wall;
            const FVector Eye = Camera->GetComponentLocation();
            bool bFoundWall = false;
            for (const FVector Direction : {FVector::ForwardVector, -FVector::ForwardVector, FVector::RightVector, -FVector::RightVector})
            {
                FHitResult Candidate;
                if (GetWorld()->LineTraceSingleByObjectType(Candidate, Eye, Eye + Direction * 20000.f,
                    FCollisionObjectQueryParams(ECC_WorldStatic), FCollisionQueryParams(SCENE_QUERY_STAT(ViewmodelWallTest), false, this)) &&
                    FMath::Abs(Candidate.ImpactNormal.Z) < .2f && (!bFoundWall || Candidate.Distance < Wall.Distance))
                { Wall = Candidate; bFoundWall = true; }
            }
            if (!bFoundWall)
            {
                UE_LOG(LogTemp, Warning, TEXT("ER_VIEWMODEL_WALL_TEST no_wall_found"));
                return;
            }
            SetActorLocation(Wall.ImpactPoint + Wall.ImpactNormal * 36.f - FVector(0, 0, Camera->GetRelativeLocation().Z));
            Controller->SetControlRotation((-Wall.ImpactNormal).Rotation());
            FTimerHandle CaptureTimer;
            GetWorldTimerManager().SetTimer(CaptureTimer, [this, Wall]()
            {
                FMinimalViewInfo View;
                CalcCamera(0.f, View);
                const FVector Muzzle = GetVisualMuzzle(true);
                const FVector RenderedMuzzle = View.TransformWorldToFirstPerson(Muzzle, false);
                const float Clearance = FVector::DotProduct(RenderedMuzzle - Wall.ImpactPoint, Wall.ImpactNormal);
                UE_LOG(LogTemp, Display, TEXT("ER_VIEWMODEL_WALL_TEST clearance=%.2f scale=%.2f body=%d rifle=%d flash=%d result=%s"),
                    Clearance, View.FirstPersonScale, int32(FirstPersonBody->FirstPersonPrimitiveType),
                    int32(FirstPersonRifle->FirstPersonPrimitiveType), int32(FirstPersonMuzzleFlash->FirstPersonPrimitiveType),
                    Clearance > 0.f ? TEXT("PASS") : TEXT("FAIL"));
                if (auto* PC = Cast<APlayerController>(Controller)) PC->ConsoleCommand(TEXT("Shot"));
            }, 1.f, false);
        }, 2.f, false);
    }
#endif
}
const FERTeamAppearance& AERCharacter::GetTeamAppearance() const
{
    const auto* State = GetPlayerState<AERPlayerState>();
    const bool Alpha = !State || State->Team != EERTeam::Omega;
    return ERSkins::Select(Alpha ? AlphaSkins : OmegaSkins, Alpha ? AlphaAppearance : OmegaAppearance, State ? State->ActiveSkin : 0);
}

void AERCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    Cast<UERMovement>(GetCharacterMovement())->ApplyPosturePresentation();
    UpdateWeaponPresentation();
    DamageViewKick = FMath::VInterpTo(DamageViewKick, FVector::ZeroVector, DeltaTime, 18.f);
    DeathViewDrop = FMath::FInterpTo(DeathViewDrop, Health->bDead ? -55.f : 0.f, DeltaTime, 5.f);
#if !UE_BUILD_SHIPPING
    if (!bFPSPoseAudited && IsLocallyControlled() && GetWorld()->GetTimeSeconds() > 2 && FParse::Param(FCommandLine::Get(), TEXT("ERFPSAudit")))
    {
        bFPSPoseAudited = true;
        const FTransform View = Camera->GetComponentTransform();
        for (const FName Name : {FName(TEXT("HandGrip_R")), FName(TEXT("HandGrip_L")), FName(TEXT("upperarm_l")), FName(TEXT("lowerarm_l")), FName(TEXT("hand_l"))})
        {
            const FTransform Pose = FirstPersonBody->GetSocketTransform(Name);
            const FVector P = View.InverseTransformPositionNoScale(Pose.GetLocation());
            UE_LOG(LogTemp, Display, TEXT("ER_FPS_POSE mesh=%s bone=%s camera=%.4f,%.4f,%.4f scale=%s"), *FirstPersonBody->GetSkeletalMeshAsset()->GetName(), *Name.ToString(), P.X, P.Y, P.Z, *Pose.GetScale3D().ToString());
        }
        const FTransform Rifle = FirstPersonRifle->GetComponentTransform().GetRelativeTransform(View);
        UE_LOG(LogTemp, Display, TEXT("ER_FPS_RIFLE camera=%s"), *Rifle.ToString());
    }
#endif
    const auto* Movement = Cast<UERMovement>(GetCharacterMovement());
    if (HasAuthority() && Movement->bHighClimb && Weapon)
    {
        if(Weapon->bCookingGrenade) Weapon->Cancel();
        else Weapon->StopFire();
    }
    const bool bGrenade=AppliedWeaponData && AERWeapon::IsHandGrenade(AppliedWeaponData->Payload);
    const bool bReadyToThrow=bGrenade && Weapon && Weapon->bCookingGrenade && !Health->bDead;
    GrenadeThrowPoseBlend=FMath::FInterpTo(GrenadeThrowPoseBlend,bReadyToThrow?1.f:0.f,DeltaTime,14.f);
    const double LocalNow=GetWorld()->GetTimeSeconds();
    const bool bHideGrenade=bGrenade && ((Weapon && Weapon->IsGrenadeHandHidden()) ||
        (IsLocallyControlled() && ((bLocalGrenadeInputHeld && LocalNow<LocalGrenadeCookDeadline) || LocalNow<LocalGrenadeHideUntil)));
    FirstPersonRifle->SetVisibility(!Health->bDead && !Movement->bHighClimb && !bHideGrenade);
    WorldRifle->SetVisibility(!Health->bDead && (bReadyToThrow || !(Weapon && Weapon->IsGrenadeHandHidden())));
    FirstPersonSight->SetVisibility(!Health->bDead && !Movement->bHighClimb && AppliedWeaponData && AppliedWeaponData->SightMesh);
    WorldSight->SetVisibility(!Health->bDead && AppliedWeaponData && AppliedWeaponData->SightMesh);
    const bool bFlash = GetWorld()->GetTimeSeconds() < MuzzleFlashUntil && !Health->bDead && !Movement->bHighClimb;
    FirstPersonMuzzleFlash->SetVisibility(bFlash);
    WorldMuzzleFlash->SetVisibility(bFlash);
    CameraKick = Health->bDead || Movement->bHighClimb ? 0.f : FMath::FInterpTo(CameraKick, 0.f, DeltaTime, 22.f);
    const bool bHideLeft = Movement->bHighClimb || (AppliedWeaponData && AppliedWeaponData->bOneHanded && !(Weapon && Weapon->bReloading));
    if (bLeftArmHidden != bHideLeft)
    {
        bLeftArmHidden = bHideLeft;
        if (bLeftArmHidden) FirstPersonBody->HideBoneByName(TEXT("upperarm_l"), EPhysBodyOp::PBO_None);
        else FirstPersonBody->UnHideBoneByName(TEXT("upperarm_l"));
    }
    const bool bHideRight=bHideGrenade;
    if(bRightArmHidden!=bHideRight)
    {
        bRightArmHidden=bHideRight;
        if(bHideRight) FirstPersonBody->HideBoneByName(TEXT("upperarm_r"),EPhysBodyOp::PBO_None);
        else FirstPersonBody->UnHideBoneByName(TEXT("upperarm_r"));
    }
    // Presentation only: preserve the physical capsule and server eye/muzzle.
    if (IsLocallyControlled())
    {
        const float Speed = GetVelocity().Size2D();
        const bool bStepping = Movement->IsMovingOnGround() && !Movement->IsTraversing() && !Health->bDead && Speed > 5;
        FVector Target = FVector::ZeroVector;
        if (bStepping)
        {
            StepPhase = FMath::Fmod(StepPhase + DeltaTime * Speed / 140.f * 2.f * PI, 4.f * PI);
            const float Strength = FMath::Clamp(Speed / (1000.f / 3.6f), 0.f, 1.f) * (bIsCrouched ? .5f : 1.f);
            Target = FVector(0, .25f * FMath::Sin(StepPhase*.5f), .70f * FMath::Sin(StepPhase)) * Strength;
        }
        StepOffset = FMath::VInterpTo(StepOffset, Target, DeltaTime, 14.f);
        if (!bStepping && StepOffset.IsNearlyZero(.001f)) { StepOffset = FVector::ZeroVector; StepPhase = 0; }
    }
    if (const auto* State = GetPlayerState<AERPlayerState>())
    {
        if (State->Team != EERTeam::None && (AppliedBodyTeam != static_cast<uint8>(State->Team) || AppliedSkin != State->ActiveSkin))
        {
            RestoreProtectionMaterials();
            const auto& Appearance = GetTeamAppearance();
            const auto& Materials = State->Team == EERTeam::Alpha ? AlphaBodyMaterials : OmegaBodyMaterials;
            if (Appearance.Mesh)
            {
                Appearance.Apply(GetMesh());
                WorldRifle->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, TEXT("HandGrip_R"));
                UE_LOG(LogTemp, Display, TEXT("ER_TEAM_APPEARANCE team=%s mesh=%s anim=%s hidden=%d bot=%d"),
                    State->Team == EERTeam::Alpha ? TEXT("Alpha") : TEXT("Omega"), *Appearance.Mesh->GetPathName(),
                    *GetPathNameSafe(Appearance.AnimationClass.Get()), Appearance.HiddenMaterialSlots.Num(), State->IsABot());
            }
            else for (int32 Slot = 0; Slot < Materials.Num(); ++Slot)
                if (Materials[Slot]) GetMesh()->SetMaterial(Slot, Materials[Slot]);
            if (Appearance.FirstPersonMesh)
            {
                // These assets share the fitted FPS bind rig, grips and weapon poses.
                // Preserve the existing component frame and the weapon-specific offset.
                FirstPersonBody->SetSkeletalMeshAsset(Appearance.FirstPersonMesh);
                FirstPersonBody->SetRelativeScale3D(FVector::OneVector);
                FirstPersonBody->SetRelativeLocation(FirstPersonBodyBaseLocation +
                    (AppliedWeaponData ? AppliedWeaponData->ViewOffset : FVector::ZeroVector));
                FirstPersonBody->SetAnimationMode(EAnimationMode::AnimationSingleNode);
                FirstPersonBody->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
                FirstPersonBody->SetForcedLOD(1);
                FirstPersonBody->EmptyOverrideMaterials();
                for (int32 LOD = 0; LOD < Appearance.FirstPersonMesh->GetLODNum(); ++LOD)
                    FirstPersonBody->ShowAllMaterialSections(LOD);
                for (int32 Slot = 0; Slot < Appearance.Materials.Num(); ++Slot)
                    if (Appearance.Materials[Slot]) FirstPersonBody->SetMaterial(Slot, Appearance.Materials[Slot]);
                FirstPersonBody->HideBoneByName(TEXT("head"), EPhysBodyOp::PBO_None);
                if (bLeftArmHidden) FirstPersonBody->HideBoneByName(TEXT("upperarm_l"), EPhysBodyOp::PBO_None);
                if (AppliedWeaponData && AppliedWeaponData->IdleAnimation)
                    FirstPersonBody->PlayAnimation(AppliedWeaponData->IdleAnimation, true);
                UE_LOG(LogTemp, Display, TEXT("ER_FPS_PLAYER_FOREARMS team=%s mesh=%s frame=%s"),
                    State->Team == EERTeam::Alpha ? TEXT("Alpha") : TEXT("Omega"),
                    *Appearance.FirstPersonMesh->GetPathName(), *FirstPersonBody->GetRelativeLocation().ToString());
            }
            else
            {
                const auto& FPSMaterials = State->Team == EERTeam::Alpha ? AlphaFirstPersonMaterials : OmegaFirstPersonMaterials;
                for (int32 Slot = 0; Slot < FPSMaterials.Num(); ++Slot)
                    if (FPSMaterials[Slot]) FirstPersonBody->SetMaterial(Slot, FPSMaterials[Slot]);
            }
            AppliedBodyTeam = static_cast<uint8>(State->Team);
            AppliedSkin = State->ActiveSkin;
            bProtectionMaterialsDirty = true;
            if (!Appearance.Mesh && !Materials.IsEmpty()) UE_LOG(LogTemp, Display, TEXT("ER_TARGET_COLORS team=%s slots=%d"), State->Team == EERTeam::Alpha ? TEXT("Alpha") : TEXT("Omega"), Materials.Num());
        }
    }
    UpdateProtectionPresentation();
}
UMaterialInterface* AERCharacter::GetUnprotectedMaterial(UMeshComponent* Mesh, int32 Slot) const
{
    for (const auto& State : ProtectionSurfaces)
        if (State.Mesh == Mesh && State.Originals.IsValidIndex(Slot)) return State.Originals[Slot];
    return Mesh->GetMaterial(Slot);
}
void AERCharacter::RestoreProtectionMaterials()
{
    for (const auto& State : ProtectionSurfaces)
        if (IsValid(State.Mesh))
            for (int32 Slot=0; Slot<State.Originals.Num(); ++Slot) State.Mesh->SetMaterial(Slot,State.Originals[Slot]);
    ProtectionSurfaces.Reset();
}
void AERCharacter::UpdateProtectionPresentation()
{
    if (GetNetMode() == NM_DedicatedServer) return;
    const bool Protected = !Health->bDead && Health->IsProtected();
    if (!bProtectionMaterialsDirty && Protected == bProtectionVisualsApplied) return;
    bProtectionMaterialsDirty = false;
    bProtectionVisualsApplied = Protected;
    RestoreProtectionMaterials();
    if (!Protected || !ProtectionBaseMaterial) return;
    if (!ProtectionSurface)
    {
        ProtectionSurface=UMaterialInstanceDynamic::Create(ProtectionBaseMaterial,this);
        ProtectionSurface->SetVectorParameterValue(TEXT("Color"),FLinearColor(.32f,.32f,.32f));
    }
    TInlineComponentArray<UMeshComponent*> Meshes(this);
    for (auto* Mesh : Meshes)
    {
        // Body, first-person arms and skin gear; weapons retain their materials.
        if (Mesh != GetMesh() && Mesh != FirstPersonBody && !Mesh->ComponentHasTag(TEXT("ERSkinGear"))) continue;
        auto& State=ProtectionSurfaces.AddDefaulted_GetRef();
        State.Mesh=Mesh;
        for (int32 Slot=0; Slot<Mesh->GetNumMaterials(); ++Slot)
        {
            State.Originals.Add(Mesh->GetMaterial(Slot));
            Mesh->SetMaterial(Slot,ProtectionSurface);
        }
    }
}
void AERCharacter::CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult)
{
    Super::CalcCamera(DeltaTime, OutResult);
    // The viewmodel now occupies a shallow depth range; retain nearby fingers.
    OutResult.PerspectiveNearClipPlane = 1.f;
    if (const auto* PC = Cast<AERPlayerController>(Controller); PC && PC->bZoomHeld && !Health->bDead && Weapon && Weapon->Data)
        OutResult.FOV = Weapon->Data->ZoomFOV;
    if (IsLocallyControlled() && !Health->bDead)
    {
        const FVector Right = FRotationMatrix(FRotator(0,OutResult.Rotation.Yaw,0)).GetUnitAxis(EAxis::Y);
        OutResult.Location += Right * StepOffset.Y + FVector::UpVector * StepOffset.Z;
        OutResult.Rotation.Pitch += CameraKick;
        const FVector LocalKick = FRotationMatrix(OutResult.Rotation).GetTransposed().TransformVector(DamageViewKick);
        OutResult.Location += DamageViewKick * .8f;
        OutResult.Rotation.Pitch -= LocalKick.X * .45f;
        OutResult.Rotation.Roll += LocalKick.Y * .9f;
    }
    if (IsLocallyControlled() && Health->bDead) OutResult.Location.Z += DeathViewDrop;
}
void AERCharacter::EndPlay(const EEndPlayReason::Type Reason)
{
    if (HasAuthority())
    {
        for (auto Item : Inventory) if (IsValid(Item)) Item->Destroy();
        if (IsValid(Weapon)) Weapon->Destroy();
    }
    Super::EndPlay(Reason);
}
float AERCharacter::TakeDamage(float Damage, const FDamageEvent& Event, AController* Killer, AActor* Causer)
{
    const float Before = Health->Health;
    FVector Direction = Killer && Killer->GetPawn() ? GetActorLocation() - Killer->GetPawn()->GetActorLocation() : FVector::ForwardVector;
    if (Event.IsOfType(FPointDamageEvent::ClassID)) Direction = static_cast<const FPointDamageEvent&>(Event).ShotDirection;
    LastHitDirection = Direction.GetSafeNormal();
    float Ratio = 2.f;
    if (const auto* Gun = Cast<AERWeapon>(Causer); Gun && Gun->Data)
    {
        Ratio = Gun->Data->ArmorRatio;
        if (Gun->Data->Payload == EERPayload::Bullet && Event.IsOfType(FPointDamageEvent::ClassID))
        {
            const auto& Point = static_cast<const FPointDamageEvent&>(Event);
            if(Gun->Data->bUseBodyHitZones)
            {
                const auto Zone=ERWeaponBallistics::ZoneForBone(Point.HitInfo.BoneName);
                Damage*=ERWeaponBallistics::ZoneMultiplier(Zone);
                if(Zone==ERWeaponBallistics::EHitZone::Legs ||
                    (Zone==ERWeaponBallistics::EHitZone::Head && !Health->bHelmet)) Ratio=2.f;
            }
            Damage = ERWeaponBallistics::AtRange(Damage, Gun->Data->RangeModifier,
                FVector::Distance(Gun->LastShotTraceStart, Point.HitInfo.ImpactPoint));
        }
    }
    else if (const auto* Projectile = Cast<AERProjectile>(Causer); Projectile &&
        (Projectile->Payload == EERPayload::HE || Projectile->Payload == EERPayload::LaunchedHE))
            Ratio = ERWeaponBallistics::GrenadeArmorRatio(Projectile->BlastArmorRatio);
    Health->ReceiveDamage(Damage, Killer, Ratio);
    if (Before > Health->Health) ClientDamageFeedback(Before - Health->Health, LastHitDirection);
    return Before - Health->Health;
}
void AERCharacter::ClientDamageFeedback_Implementation(float Amount, FVector_NetQuantizeNormal Direction)
{
    if (!IsLocallyControlled() || !Controller || !Controller->IsPlayerController()) return;
    DamageViewKick = (DamageViewKick + FVector(Direction) * FMath::Clamp(Amount / 25.f, .5f, 1.2f)).GetClampedToMaxSize(1.5f);
    if (HitSound) UGameplayStatics::PlaySoundAtLocation(this, HitSound,
        Camera->GetComponentLocation() - FVector(Direction) * 140.f, .65f, 1.f, 0.f, HitAttenuation);
    UE_LOG(LogTemp, Display, TEXT("ER_DAMAGE_FEEDBACK amount=%.1f direction=%s sound=%d"), Amount, *FVector(Direction).ToString(), HitSound != nullptr);
}
void AERCharacter::FellOutOfWorld(const UDamageType& Type)
{
    if (HasAuthority() && !Health->bDead)
    {
        Health->ProtectionEndServerTime = 0;
        Health->ReceiveDamage(100, nullptr);
    }
}
void AERCharacter::ApplyDeathVisuals()
{
    if (bDeathVisualsApplied) return;
    bDeathVisualsApplied = true;
    if (HasAuthority()) for (auto Item : Inventory) if (Item) Item->Cancel();
    if (HasAuthority() && CorpseClass)
    {
        FTransform Transform(GetActorRotation(), GetActorLocation());
        if (auto* Corpse = GetWorld()->SpawnActorDeferred<AERCorpse>(CorpseClass, Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
        {
            if (const auto* State = GetPlayerState<AERPlayerState>()) { Corpse->Team = State->Team; Corpse->SkinIndex = State->ActiveSkin; }
            Corpse->ImpactDirection = LastHitDirection;
            UGameplayStatics::FinishSpawningActor(Corpse, Transform);
        }
    }
    GetCharacterMovement()->StopMovementImmediately();
    GetCharacterMovement()->DisableMovement();
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FirstPersonBody->SetVisibility(false); FirstPersonRifle->SetVisibility(false); WorldRifle->SetVisibility(false); TeamMarker->SetVisibility(false);
    FirstPersonSight->SetVisibility(false); WorldSight->SetVisibility(false);
    GetMesh()->SetVisibility(false, true);
    FirstPersonMuzzleFlash->SetVisibility(false); WorldMuzzleFlash->SetVisibility(false);
    CameraKick = 0; MuzzleFlashUntil = 0;
}
void AERCharacter::ServerStartFire_Implementation() { if (!Health->bDead && Weapon && !Cast<UERMovement>(GetCharacterMovement())->bHighClimb) Weapon->StartFire(); }
void AERCharacter::SetGrenadeInputHeld(bool bHeld)
{
    if(!IsLocallyControlled()) return;
    if(!bHeld)
    {
        if(bLocalGrenadeInputHeld) LocalGrenadeHideUntil=GetWorld()->GetTimeSeconds()+.35;
        bLocalGrenadeInputHeld=false;return;
    }
    if(Health->bDead || !Weapon || !Weapon->Data || !AERWeapon::IsHandGrenade(Weapon->Data->Payload) ||
        Weapon->MagazineAmmo<=0 || Weapon->bReloading || Weapon->IsGrenadeHandHidden() || Cast<UERMovement>(GetCharacterMovement())->bHighClimb) return;
    bLocalGrenadeInputHeld=true;
    LocalGrenadeCookDeadline=GetWorld()->GetTimeSeconds()+Weapon->Data->FuseDuration;
}
void AERCharacter::ServerStopFire_Implementation() { if (Weapon) Weapon->StopFire(); }
void AERCharacter::ServerReload_Implementation() { if (!Health->bDead && Weapon && !Cast<UERMovement>(GetCharacterMovement())->bHighClimb) Weapon->Reload(); }
bool AERCharacter::UsesReferenceArsenal() const
{
    // Both catalogs remain available for comparison and existing saved assets.
    return HasAuthority() ? ReferenceArsenal.Num() == 10 && !GetWorld()->URL.HasOption(TEXT("ERLegacyArsenal")) : Inventory.Num() == 10;
}
void AERCharacter::InitializeLoadout(int32 Primary)
{
    if (!HasAuthority() || Arsenal.Num() < 11 || !Inventory.IsEmpty()) return;
    if (Weapon) Weapon->Destroy();
    const bool bReference = UsesReferenceArsenal();
    const TArray<int32> Indices = bReference ? TArray<int32>{0,1,2,3,4,5,6,7,8,9} : TArray<int32>{FMath::Clamp(Primary, 0, 6), 7, 8, 9, 10};
    Health->Armor = bReference ? 100.f : 0.f;
    for (int32 Index : Indices)
    {
        auto* Item = GetWorld()->SpawnActorDeferred<AERWeapon>(WeaponClass, GetActorTransform(), this, this,
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        if (!Item) continue;
        Item->Data = bReference ? ReferenceArsenal[Index] : Arsenal[Index];
        UGameplayStatics::FinishSpawningActor(Item, Item->GetActorTransform());
        Item->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
        Inventory.Add(Item);
    }
    Weapon = Inventory.IsEmpty() ? nullptr : Inventory[bReference ? 4 : 0];
    ForceNetUpdate();
    UE_LOG(LogTemp, Display, TEXT("ER_LOADOUT_SPAWN primary=%d slots=%d"), Primary, Inventory.Num());
}
void AERCharacter::ServerSelectWeapon_Implementation(int32 Slot)
{
    if (Health->bDead || !Inventory.IsValidIndex(Slot) || !Inventory[Slot] || Weapon == Inventory[Slot] ||
        (Weapon && Weapon->bCookingGrenade) || Cast<UERMovement>(GetCharacterMovement())->IsTraversing()) return;
    if (Weapon) Weapon->Cancel();
    Weapon = Inventory[Slot];
    MuzzleFlashUntil = 0; CameraKick = 0;
    ForceNetUpdate();
    UE_LOG(LogTemp, Display, TEXT("ER_WEAPON_SELECTED slot=%d name=%s ammo=%d reserve=%d"),
        Slot, *Weapon->Data->DisplayName, Weapon->MagazineAmmo, Weapon->ReserveAmmo);
}
FVector AERCharacter::GetVisualMuzzle(bool bFirstPerson) const
{
    const auto* Component = bFirstPerson ? FirstPersonRifle.Get() : WorldRifle.Get();
    return Weapon && Weapon->Data && Weapon->Data->Mesh ? Component->GetComponentTransform().TransformPosition(Weapon->Data->MuzzleOffset) :
        Component->GetSocketLocation(TEXT("Muzzle"));
}
FVector AERCharacter::GetRenderedMuzzle(bool bFirstPerson) const
{
    const FVector Muzzle = GetVisualMuzzle(bFirstPerson);
    if (bFirstPerson)
        if (const auto* PC = Cast<APlayerController>(Controller); PC && PC->PlayerCameraManager)
            return PC->PlayerCameraManager->GetCameraCacheView().TransformWorldToFirstPerson(Muzzle, false);
    return Muzzle;
}
void AERCharacter::UpdateWeaponPresentation()
{
    auto* Data = Weapon ? Weapon->Data.Get() : nullptr;
    if (!Data || !Data->Mesh) return;
    // The fitted pistol animates its hidden shoulder to place the forearm.
    // Skeleton translation retargeting would discard that authored movement.
    if (auto* Instance=FirstPersonBody->GetSingleNodeInstance())
        Instance->GetRequiredBones().SetDisableRetargeting(Data->bFirstPersonPalmGrip);
    if (Data == AppliedWeaponData)
    {
        if (!Weapon->bReloading)
        {
            for (auto* Component : {FirstPersonRifle.Get(), WorldRifle.Get()})
                if (Data->WeaponIdleAnimation && !Component->IsPlaying()) Component->PlayAnimation(Data->WeaponIdleAnimation, true);
            if (Data->IdleAnimation && !FirstPersonBody->IsPlaying()) FirstPersonBody->PlayAnimation(Data->IdleAnimation, true);
        }
        if (bAppliedReloading != Weapon->bReloading)
        {
            bAppliedReloading = Weapon->bReloading;
            for (auto* Component : {FirstPersonRifle.Get(), WorldRifle.Get()})
            {
                UAnimSequence* Clip = bAppliedReloading ? Data->WeaponReloadAnimation.Get() : Data->WeaponIdleAnimation.Get();
                if (Clip)
                {
                    Component->PlayAnimation(Clip, !bAppliedReloading);
                    Component->SetPlayRate(bAppliedReloading ? Clip->GetPlayLength() / FMath::Max(.01f, Data->ReloadDuration) : 1.f);
                }
            }
            UAnimSequence* Arms = bAppliedReloading ? Data->ReloadAnimation.Get() : Data->IdleAnimation.Get();
            if (Arms)
            {
                FirstPersonBody->PlayAnimation(Arms, !bAppliedReloading);
                FirstPersonBody->SetPlayRate(bAppliedReloading ? Arms->GetPlayLength() / FMath::Max(.01f, Data->ReloadDuration) : 1.f);
            }
        }
        return;
    }
    AppliedWeaponData = Data;
    bAppliedReloading = false;
    FirstPersonRifle->AttachToComponent(FirstPersonBody,FAttachmentTransformRules::KeepRelativeTransform,
        Data->bFirstPersonPalmGrip ? TEXT("ER_Palm_R") : TEXT("HandGrip_R"));
    FirstPersonBody->SetRelativeLocation(FirstPersonBodyBaseLocation + Data->ViewOffset);
    for (auto* Sight : {FirstPersonSight.Get(), WorldSight.Get()})
    { Sight->SetSkeletalMeshAsset(Data->SightMesh); Sight->SetRelativeTransform(Data->SightTransform); }
    for (auto* Component : {FirstPersonRifle.Get(), WorldRifle.Get()})
    {
        Component->SetSkeletalMeshAsset(Data->Mesh);
        Component->EmptyOverrideMaterials();
        Component->SetRelativeTransform(Component==FirstPersonRifle && Data->bFirstPersonPalmGrip ? Data->FirstPersonGripTransform : Data->GripTransform);
        Component->SetAnimationMode(EAnimationMode::AnimationSingleNode);
        Component->SetAnimation(nullptr);
        if (Data->WeaponIdleAnimation) Component->PlayAnimation(Data->WeaponIdleAnimation, true);
    }
    if (Data->IdleAnimation) FirstPersonBody->PlayAnimation(Data->IdleAnimation, true);
    if (Data->bFitSupportHand)
    {
        // A stationary/culled mesh can reuse its evaluated idle. Equipping a
        // different support target must invalidate that presentation immediately.
        GetMesh()->TickAnimation(0.f, false);
        GetMesh()->RefreshBoneTransforms();
    }
    if (auto* PC = Cast<AERPlayerController>(Controller)) PC->bZoomHeld = false;
    MuzzleFlashUntil = 0;
    UE_LOG(LogTemp, Display, TEXT("ER_WEAPON_MODEL name=%s mesh=%s"), *Data->DisplayName, *Data->Mesh->GetPathName());
}
void AERCharacter::MulticastShot_Implementation(FVector_NetQuantize Start, FVector_NetQuantize End, bool bTracer)
{
    ++ShotFeedbackCount;
    if (GetNetMode() == NM_DedicatedServer || Health->bDead || Cast<UERMovement>(GetCharacterMovement())->bHighClimb) return;
    const bool bFirstPerson = IsLocallyControlled() && Controller && Controller->IsPlayerController();
    const auto* Data = Weapon ? Weapon->Data.Get() : nullptr;
    if (Data && Data->WeaponFireAnimation)
        for (auto* Component : {FirstPersonRifle.Get(), WorldRifle.Get()})
        {
            Component->PlayAnimation(Data->WeaponFireAnimation, false);
            Component->SetPlayRate(FMath::Max(1.f, Data->WeaponFireAnimation->GetPlayLength() / FMath::Max(.01f, Data->ShotInterval)));
        }
    if(Data && Data->Payload==EERPayload::Bullet)
        for(TActorIterator<AERSmokeCloud> It(GetWorld());It;++It) It->AddShot(Start,End);
    if (bTracer && Data && Data->Payload == EERPayload::Bullet)
        AERAttachedTracer::Spawn(this, TracerSystem, End, bFirstPerson);
    if (Data && Data->Payload == EERPayload::Bullet && ShotFeedbackCount % 2 == 1)
    {
        const FVector Forward = GetBaseAimRotation().Vector();
        const FVector Right = FRotationMatrix(GetBaseAimRotation()).GetUnitAxis(EAxis::Y);
        FVector Port = GetVisualMuzzle(bFirstPerson) - Forward * (Data->bOneHanded ? 8.f : 24.f) + Right * 4.f;
        if (bFirstPerson)
            if (auto* PC = Cast<APlayerController>(Controller); PC && PC->PlayerCameraManager)
                Port = PC->PlayerCameraManager->GetCameraCacheView().TransformWorldToFirstPerson(Port, false);
        AERSpentCasing::Eject(GetWorld(), CasingClass, Port, Right, Forward, bFirstPerson ? .2f : 1.f);
    }
    const auto* Animation = Data && Data->Mesh ? Data->FireAnimation.Get() : FirstPersonFireAnimation.Get();
    const float Pitch = Data ? Data->SoundPitch : 1.f;
    const bool bGunshot = !Data || Data->Payload == EERPayload::Bullet || Data->Payload == EERPayload::Rocket || Data->Payload == EERPayload::LaunchedHE;
    if (bFirstPerson)
    {
        // Animate the body and its socket-attached rifle together. Both hands keep their grip.
        if (Animation)
        {
            FirstPersonBody->PlayAnimation(const_cast<UAnimSequence*>(Animation), false);
            FirstPersonBody->SetPlayRate(Data && Data->WeaponFireAnimation ? FMath::Max(1.f,Animation->GetPlayLength()/FMath::Max(.01f,Data->ShotInterval)) : 1.f);
            if (auto* Instance = FirstPersonBody->GetSingleNodeInstance()) Instance->SetPosition(0.f, false);
        }
        if (bGunshot) CameraKick = FMath::Min(CameraKick + .35f, .65f);
        if (FireSound && bGunshot) UGameplayStatics::PlaySound2D(this, FireSound, .35f, Pitch);
    }
    else if (FireSound && bGunshot)
    {
        UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetVisualMuzzle(false),
            .6f, Pitch, 0.f, FireAttenuation);
    }
    // Align the glow along the rifle's model-space barrel (+Y), without rotating the rifle.
    const FVector FlashScale(.012f, .025f, .012f);
    for (auto* Flash : {FirstPersonMuzzleFlash.Get(), WorldMuzzleFlash.Get()})
    {
        Flash->SetWorldRotation(Flash->GetAttachParent()->GetComponentQuat() *
            (Data && Data->Mesh ? FRotator(0,-90,0).Quaternion() : FQuat::Identity));
        Flash->SetWorldLocation(GetVisualMuzzle(Flash == FirstPersonMuzzleFlash) +
            (Data && Data->Mesh ? Flash->GetAttachParent()->GetComponentTransform().TransformVectorNoScale(FVector(4, 0, 0)) :
            Flash->GetAttachParent()->GetComponentTransform().TransformVectorNoScale(FVector(0, 4, 0))));
        Flash->SetWorldScale3D(FlashScale);
        Flash->SetVisibility(!(Data && Data->bOneHanded && Data->Payload != EERPayload::Bullet));
    }
    MuzzleFlashUntil = GetWorld()->GetTimeSeconds() + (Data && Data->bOneHanded && Data->Payload != EERPayload::Bullet ? 0 : .018);
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("ERShotAudit")))
    {
        UE_LOG(LogTemp, Display, TEXT("ER_SHOT_FEEDBACK pawn=%s local=%d count=%u animation=%d sound=%d flash=%d"),
            *GetName(), bFirstPerson, ShotFeedbackCount, FirstPersonFireAnimation != nullptr, FireSound != nullptr,
            MuzzleFlashMesh != nullptr && MuzzleFlashMaterial != nullptr);
    }
#endif
}
void AERCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME(AERCharacter, Weapon);
    DOREPLIFETIME(AERCharacter, Inventory);
}
