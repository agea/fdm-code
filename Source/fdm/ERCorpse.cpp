#include "ERCorpse.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneComponent.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"

void UERCorpseMeshComponent::FinalizeBoneTransform()
{
    // Physics blending reconstructs component-space transforms, including
    // hidden accessories. Collapse them again before publishing the pose to
    // rendering; a visibility flag alone does not survive that reconstruction.
    if (const auto* Mesh = GetSkeletalMeshAsset())
    {
        auto& Transforms = GetEditableComponentSpaceTransforms();
        const auto& Visibility = GetEditableBoneVisibilityStates();
        const auto& Ref = Mesh->GetRefSkeleton();
        for (int32 Bone = 0; Bone < Transforms.Num() && Bone < Visibility.Num(); ++Bone)
        {
            const int32 Parent = Ref.GetParentIndex(Bone);
            if (Visibility[Bone] != BVS_Visible)
            {
                if (Transforms.IsValidIndex(Parent)) Transforms[Bone] = Transforms[Parent];
                Transforms[Bone].SetScale3D(FVector::ZeroVector);
            }
        }
    }
    Super::FinalizeBoneTransform();
    if (bFreezeAfterNextPose)
    {
        bFreezeAfterNextPose = false;
        SetComponentTickEnabled(false);
    }
}

AERCorpse::AERCorpse()
{
    PrimaryActorTick.bCanEverTick = true; PrimaryActorTick.TickGroup = TG_PostPhysics;
    bReplicates = true; bAlwaysRelevant = true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
    Body = CreateDefaultSubobject<UERCorpseMeshComponent>(TEXT("Body"));
    Body->SetupAttachment(GetRootComponent());
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    InitialLifeSpan = 90;
}
const FERTeamAppearance& AERCorpse::GetTeamAppearance() const
{
    const bool Alpha = Team == EERTeam::Alpha;
    return ERSkins::Select(Alpha ? AlphaSkins : OmegaSkins, Alpha ? AlphaAppearance : OmegaAppearance, SkinIndex);
}
void AERCorpse::BeginPlay()
{
    Super::BeginPlay();
    const auto& Appearance = GetTeamAppearance();
    if (Appearance.Mesh) Appearance.Apply(Body, true);
    else
    {
        const auto& Materials = Team == EERTeam::Alpha ? AlphaMaterials : OmegaMaterials;
        for (int32 I = 0; I < Materials.Num(); ++I) if (Materials[I]) Body->SetMaterial(I, Materials[I]);
    }
    // Evaluate the complete pose at LOD0 before creating the physics bodies.
    // Keep evaluation active until settling so non-physical fingers, clothing
    // and hidden-bone descendants follow the simulated anatomical bones.
    Body->SetForcedLOD(1);
    Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    if (auto* Instance = Body->GetSingleNodeInstance()) Instance->SetPlaying(false);
    Body->TickAnimation(0.f, false);
    Body->RefreshBoneTransforms();
    // Cosmetic bodies only contact the world: never obstruct spawns, players
    // or bullets. Each peer simulates the fall independently.
    Body->SetCollisionObjectType(ECC_PhysicsBody);
    Body->SetCollisionResponseToAllChannels(ECR_Ignore);
    Body->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
    Body->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
    Body->SetSimulatePhysics(true);
    Body->WakeAllRigidBodies();
    Body->AddImpulseToAllBodiesBelow(FVector(ImpactDirection).GetSafeNormal() * 120.f + FVector(0, 0, -40), NAME_None, true, true);
    GetWorldTimerManager().SetTimer(SettleTimer, this, &AERCorpse::Settle, 5.f, false);
    if (HasAuthority())
    {
        TArray<AERCorpse*> Older;
        for (TActorIterator<AERCorpse> It(GetWorld()); It; ++It) if (*It != this) Older.Add(*It);
        Older.Sort([](const AERCorpse& A, const AERCorpse& B) { return A.GetGameTimeSinceCreation() > B.GetGameTimeSinceCreation(); });
        while (Older.Num() >= 12) { Older[0]->Destroy(); Older.RemoveAt(0); }
    }
    if (Appearance.bCopyPhysicsPoseForRendering)
    {
        PoseBody=NewObject<UPoseableMeshComponent>(this);
        AddInstanceComponent(PoseBody);
        PoseBody->SetSkinnedAssetAndUpdate(Appearance.Mesh);
        PoseBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        PoseBody->RegisterComponent();
        PoseBody->AttachToComponent(Body, FAttachmentTransformRules::KeepWorldTransform);
        PoseBody->bUseAttachParentBound = true;
        PoseBody->SetWorldTransform(Body->GetComponentTransform());
        PoseBody->SetForcedLOD(1);
        for(int32 I=0;I<Appearance.Materials.Num();++I)PoseBody->SetMaterial(I,Appearance.Materials[I]);
        for(int32 LOD=0;LOD<Appearance.Mesh->GetLODNum();++LOD)for(int32 Slot=0;Slot<Appearance.Materials.Num();++Slot)
            PoseBody->ShowMaterialSection(Slot,INDEX_NONE,!Appearance.HiddenMaterialSlots.Contains(Slot),LOD);
        for(FName Bone:Appearance.HiddenBones)PoseBody->HideBoneByName(Bone,EPhysBodyOp::PBO_None);
        Body->SetVisibility(false,false);
    }
    else SetActorTickEnabled(false);
    UE_LOG(LogTemp, Display, TEXT("ER_CORPSE_CREATED team=%d physics=%d mesh=%s"), static_cast<int32>(Team), Body->IsSimulatingPhysics(), *GetPathNameSafe(Body->GetSkeletalMeshAsset()));
}
void AERCorpse::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if(!PoseBody)return;
    PoseBody->SetWorldTransform(Body->GetComponentTransform());
    const auto& Ref=Body->GetSkeletalMeshAsset()->GetRefSkeleton();
    const auto& Pose=Body->GetComponentSpaceTransforms();
    // Copy component-space physics results, not the original animation's local
    // transforms. Updating once in a batch avoids per-bone pose allocations.
    PoseBody->BoneSpaceTransforms.SetNumUninitialized(Pose.Num());
    for (int32 B=0; B<Pose.Num(); ++B)
    {
        const int32 Parent=Ref.GetParentIndex(B);
        PoseBody->BoneSpaceTransforms[B]=Parent==INDEX_NONE ? Pose[B] : Pose[B].GetRelativeTransform(Pose[Parent]);
    }
    PoseBody->MarkRefreshTransformDirty();
    PoseBody->RefreshBoneTransforms();
    if (bFinishPosePending)
    {
        PoseBody->SetComponentTickEnabled(false);
        SetActorTickEnabled(false);
    }
}
void AERCorpse::Settle()
{
    Body->SetAllPhysicsLinearVelocity(FVector::ZeroVector);
    Body->SetAllPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
    Body->PutAllRigidBodiesToSleep();
    // Capture one final post-physics visual pose before freezing both components.
    if (auto* CorpseMesh = Cast<UERCorpseMeshComponent>(Body)) CorpseMesh->FreezeAfterPhysicsPose();
    bFinishPosePending = true;
    UE_LOG(LogTemp, Display, TEXT("ER_CORPSE_SETTLED pelvis=%s"), *Body->GetSocketLocation(TEXT("pelvis")).ToString());
}
bool AERCorpse::HasValidRagdollPose() const
{
    const auto& Appearance = GetTeamAppearance();
    const auto* Mesh = Body->GetSkeletalMeshAsset();
    const auto* Physics = Body->GetPhysicsAsset();
    if (!Mesh || !Physics || Physics->SkeletalBodySetups.Num() != 16 || Physics->ConstraintSetup.Num() != 15 ||
        !Appearance.Matches(Body, true)) return false;
    for (const auto& Joint : Physics->ConstraintSetup)
    {
        const auto& Profile = Joint->GetConstraintProfilePropertiesOrDefault(NAME_None);
        if (Profile.LinearLimit.XMotion != LCM_Locked || Profile.LinearLimit.YMotion != LCM_Locked ||
            Profile.LinearLimit.ZMotion != LCM_Locked || !Profile.bEnableProjection) return false;
    }
    for (const FName Bone : Appearance.HiddenBones)
        if (!Body->GetBoneTransform(Body->GetBoneIndex(Bone)).GetScale3D().IsNearlyZero(.001f)) return false;
    const auto& Ref = Mesh->GetRefSkeleton();
    auto Reference = [&Ref](FName Name)
    {
        int32 Bone = Ref.FindBoneIndex(Name);
        FTransform Result = Ref.GetRefBonePose()[Bone];
        while ((Bone = Ref.GetParentIndex(Bone)) != INDEX_NONE) Result *= Ref.GetRefBonePose()[Bone];
        return Result;
    };
    for (const TCHAR* Side : {TEXT("l"), TEXT("r")})
        for (const auto& Names : {TPair<FString,FString>(TEXT("upperarm_"),TEXT("lowerarm_")),
            TPair<FString,FString>(TEXT("lowerarm_"),TEXT("hand_")),
            TPair<FString,FString>(TEXT("thigh_"),TEXT("calf_")), TPair<FString,FString>(TEXT("calf_"),TEXT("foot_"))})
        {
            const FName Start(*(Names.Key + Side)), End(*(Names.Value + Side));
            // Retargeted animation can scale individual bones. Compare against
            // the rendered parent scale, not just the overall character scale.
            const FVector LocalSegment = Reference(Start).InverseTransformPosition(Reference(End).GetLocation());
            const float Expected = Body->GetBoneTransform(Body->GetBoneIndex(Start)).TransformVector(LocalSegment).Size();
            const float Actual = FVector::Distance(Body->GetBoneTransform(Body->GetBoneIndex(Start)).GetLocation(), Body->GetBoneTransform(Body->GetBoneIndex(End)).GetLocation());
            if (FMath::Abs(Actual - Expected) > 5.f)
            {
                const auto* StartBody = Body->GetBodyInstance(Start);
                const auto* EndBody = Body->GetBodyInstance(End);
                const float Physical = StartBody && EndBody ? FVector::Distance(StartBody->GetUnrealWorldTransform().GetLocation(), EndBody->GetUnrealWorldTransform().GetLocation()) : -1;
                UE_LOG(LogTemp, Warning, TEXT("ER_CORPSE_STRETCH team=%d bone=%s expected=%.2f actual=%.2f physical=%.2f"), static_cast<int32>(Team), *Start.ToString(), Expected, Actual, Physical);
                return false;
            }
        }
    if (Appearance.bCopyPhysicsPoseForRendering)
    {
        if (!PoseBody || PoseBody->GetSkinnedAsset()!=Appearance.Mesh || PoseBody->GetCollisionEnabled()!=ECollisionEnabled::NoCollision) return false;
        for (FName Bone : {FName("pelvis"),FName("head"),FName("hand_l"),FName("hand_r"),FName("foot_l"),FName("foot_r")})
            if (!PoseBody->GetBoneTransform(PoseBody->GetBoneIndex(Bone)).Equals(Body->GetBoneTransform(Body->GetBoneIndex(Bone)),.1f)) return false;
    }
    return true;
}
void AERCorpse::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(AERCorpse, Team, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(AERCorpse, SkinIndex, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(AERCorpse, ImpactDirection, COND_InitialOnly);
}
