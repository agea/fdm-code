#include "ERWounds.h"
#include "Components/DecalComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "Misc/App.h"

UERWounds::UERWounds()
{
    SetIsReplicatedByDefault(true);
    for (int32 I=1; I<=4; ++I)
        Materials.Add(TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(FString::Printf(
            TEXT("/Game/EclipseRecoil/Effects/Wounds/MI_ER_Wound_%d.MI_ER_Wound_%d"), I, I))));
    for (int32 I=1; I<=4; ++I)
        Materials.Add(TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(FString::Printf(
            TEXT("/Game/EclipseRecoil/Effects/Wounds/MI_ER_ArmorImpact_%d.MI_ER_ArmorImpact_%d"), I, I))));
}
void UERWounds::BeginPlay()
{
    Super::BeginPlay();
    if (!Body) if (auto* Character=Cast<ACharacter>(GetOwner())) Body=Character->GetMesh();
    OnRep_Marks();
}
void UERWounds::SetBody(USkinnedMeshComponent* Mesh)
{
    Body=Mesh;
    OnRep_Marks();
}
void UERWounds::Record(USkeletalMeshComponent* Mesh, const FHitResult& Hit, bool bArmorOnly)
{
    if (!GetOwner()->HasAuthority() || !Mesh || !Mesh->GetSkeletalMeshAsset()) return;
    FName BoneName=Hit.BoneName;
    // Accepted body hits without an anatomical bone still need a visual mark.
    if(BoneName.IsNone() || Mesh->GetBoneIndex(BoneName)==INDEX_NONE)BoneName=Mesh->FindClosestBone(Hit.ImpactPoint);
    if(BoneName.IsNone())return;
    const FTransform Bone=Mesh->GetSocketTransform(BoneName);
    FERWound Mark;
    Mark.Bone=BoneName;
    // Hits are on anatomical physics bodies, not on the rendered clothing.
    // Follow the actual bullet ray into the mesh instead of the hull normal,
    // which points away from the visible surface at grazing/limb contacts.
    FVector Outward=(Hit.TraceStart-Hit.TraceEnd).GetSafeNormal();
    if(Outward.IsNearlyZero())Outward=Hit.ImpactNormal.GetSafeNormal();
    if(Outward.IsNearlyZero())Outward=Mesh->GetForwardVector();
    Mark.Position=Bone.InverseTransformPosition(Hit.ImpactPoint);
    Mark.Rotation=Bone.InverseTransformVectorNoScale(Outward).Rotation();
    Mark.Variant=FMath::RandHelper(4)+(bArmorOnly ? 4 : 0);
    // wound2 is the last user attachment and was authored upside down.
    Mark.Rotation.Roll=RollForVariant(Mark.Variant);
    Mark.Size=FMath::FRandRange(3.f,4.5f);
    if (Marks.Num()>=MaxMarks) Marks.RemoveAt(0);
    Marks.Add(Mark);
    OnRep_Marks();
    GetOwner()->ForceNetUpdate();
}
void UERWounds::CopyFrom(const UERWounds* Source)
{
    if (!GetOwner()->HasAuthority() || !Source) return;
    Marks=Source->Marks;
    OnRep_Marks();
}
void UERWounds::ClearDecals()
{
    for (UDecalComponent* Decal:Decals) if (IsValid(Decal)) Decal->DestroyComponent();
    Decals.Reset();
}
void UERWounds::OnRep_Marks()
{
    ClearDecals();
    if (!Body || !FApp::CanEverRender() || GetNetMode()==NM_DedicatedServer) return;
    Body->SetReceivesDecals(true);
    for (const auto& Mark:Marks)
    {
        if (Body->GetBoneIndex(Mark.Bone)==INDEX_NONE || !Materials.IsValidIndex(Mark.Variant)) continue;
        auto* Material=Materials[Mark.Variant].LoadSynchronous();
        if (!Material) continue;
        auto* Decal=NewObject<UDecalComponent>(GetOwner());
        Decal->SetDecalMaterial(Material);
        // Tiny wound marks must not disappear at short combat distances.
        Decal->SetFadeScreenSize(0.f);
        // Bridge the physics hull / skin gap without changing visible size.
        const float Aspect=Mark.Variant==3 ? 2537.f/2048.f : Mark.Variant==0 ? 1765.f/1804.f : 1.f;
        const float VisualSize=Mark.Size*.4f;
        Decal->DecalSize=FVector(40.f,VisualSize,VisualSize*Aspect);
        Decal->SetupAttachment(Body,Mark.Bone);
        Decal->SetRelativeLocationAndRotation(Mark.Position,Mark.Rotation);
        Decal->SetRelativeScale3D(Body->GetSocketTransform(Mark.Bone).GetScale3D().Reciprocal());
        Decal->RegisterComponent();
        Decals.Add(Decal);
    }
}
void UERWounds::EndPlay(const EEndPlayReason::Type Reason)
{
    ClearDecals();
    Super::EndPlay(Reason);
}
void UERWounds::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UERWounds,Marks);
}
