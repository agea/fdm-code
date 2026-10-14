#include "ERTeamAppearance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimInstance.h"
#include "Components/StaticMeshComponent.h"

void FERTeamAppearance::Apply(USkeletalMeshComponent* Component, bool bCorpse) const
{
    if (!Component || !Mesh) return;
    TArray<USceneComponent*> Children;
    Component->GetChildrenComponents(false, Children);
    for (auto* Child : Children) if (Child->ComponentHasTag(TEXT("ERSkinGear"))) Child->DestroyComponent();
    Component->EmptyOverrideMaterials();
    Component->SetSkeletalMeshAsset(Mesh);
    Component->SetRelativeLocation(FVector(0, 0, FeetOffset));
    Component->SetRelativeRotation(FRotator(0, -90, 0));
    Component->SetRelativeScale3D(FVector(Scale));
    Component->SetPhysicsAsset(Mesh->GetPhysicsAsset(), true);
    if (bCorpse) Component->PlayAnimation(CorpsePose, true);
    else
    {
        Component->SetAnimationMode(EAnimationMode::AnimationBlueprint);
        Component->SetAnimInstanceClass(AnimationClass);
    }
    for (int32 I = 0; I < Materials.Num(); ++I)
        if (Materials[I]) Component->SetMaterial(I, Materials[I]);
    for (const FName Bone : HiddenBones) Component->HideBoneByName(Bone, EPhysBodyOp::PBO_None);
    for (int32 LOD = 0; LOD < Mesh->GetLODNum(); ++LOD)
        for (int32 Slot = 0; Slot < Mesh->GetMaterials().Num(); ++Slot)
            Component->ShowMaterialSection(Slot, INDEX_NONE, !HiddenMaterialSlots.Contains(Slot), LOD);
    for (const auto& Part : Attachments)
    {
        if (!Part.Mesh || !Component->DoesSocketExist(Part.Bone)) continue;
        auto* Gear = NewObject<UStaticMeshComponent>(Component->GetOwner());
        Component->GetOwner()->AddInstanceComponent(Gear);
        Gear->ComponentTags.Add(TEXT("ERSkinGear"));
        Gear->SetStaticMesh(Part.Mesh);
        if (Part.Material) Gear->SetMaterial(0, Part.Material);
        Gear->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Gear->SetOwnerNoSee(Component->bOwnerNoSee);
        Gear->SetVisibility(Component->IsVisible());
        Gear->SetCastShadow(true);
        Gear->RegisterComponent();
        Gear->AttachToComponent(Component, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Part.Bone);
        Gear->SetRelativeTransform(Part.Transform);
    }
}

bool FERTeamAppearance::Matches(USkeletalMeshComponent* Component, bool bCorpse) const
{
    if (!Component || !Mesh || Component->GetSkeletalMeshAsset() != Mesh || !Component->DoesSocketExist(TEXT("HandGrip_R")))
    { UE_LOG(LogTemp, Warning, TEXT("ER_TEAM_MISMATCH mesh_or_grip component=%s"), *GetNameSafe(Component)); return false; }
    if (!bCorpse && (!Component->GetAnimInstance() || !Component->GetAnimInstance()->IsA(AnimationClass)))
    { UE_LOG(LogTemp, Warning, TEXT("ER_TEAM_MISMATCH animation component=%s actual=%s expected=%s"), *Component->GetName(), *GetNameSafe(Component->GetAnimInstance()), *GetNameSafe(AnimationClass.Get())); return false; }
    for (int32 LOD = 0; LOD < Mesh->GetLODNum(); ++LOD)
        for (const int32 Slot : HiddenMaterialSlots)
            if (Component->IsMaterialSectionShown(Slot, LOD)) { UE_LOG(LogTemp, Warning, TEXT("ER_TEAM_MISMATCH section slot=%d lod=%d"), Slot, LOD); return false; }
    for (const FName Bone : HiddenBones) if (!Component->IsBoneHiddenByName(Bone)) { UE_LOG(LogTemp, Warning, TEXT("ER_TEAM_MISMATCH bone=%s"), *Bone.ToString()); return false; }
    for (int32 I = 0; I < Materials.Num(); ++I)
        if (Materials[I] && Component->GetMaterial(I) != Materials[I]) { UE_LOG(LogTemp, Warning, TEXT("ER_TEAM_MISMATCH material=%d"), I); return false; }
    int32 GearCount = 0;
    TArray<USceneComponent*> Children;
    Component->GetChildrenComponents(false, Children);
    for (auto* Child : Children) if (Child->ComponentHasTag(TEXT("ERSkinGear")))
    {
        const auto* Gear = Cast<UStaticMeshComponent>(Child);
        if (!Gear || !Attachments.IsValidIndex(GearCount)) return false;
        const auto& Part = Attachments[GearCount++];
        if (Gear->GetStaticMesh() != Part.Mesh || Gear->GetAttachSocketName() != Part.Bone ||
            !Gear->GetRelativeTransform().Equals(Part.Transform) || Gear->GetCollisionEnabled() != ECollisionEnabled::NoCollision) return false;
    }
    if (GearCount != Attachments.Num()) return false;
    return Component->GetRelativeScale3D().Equals(FVector(Scale), .001f);
}
