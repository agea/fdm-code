#include "ERPostureMesh.h"
#include "ERCharacter.h"
#include "ERMovement.h"
#include "Engine/SkeletalMesh.h"

void UERPostureMesh::FinalizeBoneTransform()
{
    const auto* C = Cast<AERCharacter>(GetOwner());
    const auto* M = C ? Cast<UERMovement>(C->GetCharacterMovement()) : nullptr;
    const auto* Mesh = GetSkeletalMeshAsset();
    const float Blend = M ? M->Locomotion.ProneBlend : 0.f;
    if (Mesh && Blend > 0)
    {
        const auto& Ref = Mesh->GetRefSkeleton();
        auto& Pose = GetEditableComponentSpaceTransforms();
        const TArray<FTransform> Original = Pose;
        const int32 Left = Ref.FindBoneIndex(TEXT("upperarm_l")), Right = Ref.FindBoneIndex(TEXT("upperarm_r"));
        const int32 Neck = Ref.FindBoneIndex(TEXT("neck_01"));
        const int32 Pelvis = Ref.FindBoneIndex(TEXT("pelvis"));
        const int32 ThighLeft = Ref.FindBoneIndex(TEXT("thigh_l")), ThighRight = Ref.FindBoneIndex(TEXT("thigh_r"));
        auto InSubtree = [&Ref](int32 Bone, int32 Root)
        {
            if (Root == INDEX_NONE) return false;
            for (int32 I = Bone; I != INDEX_NONE; I = Ref.GetParentIndex(I)) if (I == Root) return true;
            return false;
        };
        const FVector Shoulder = Pose.IsValidIndex(Left) && Pose.IsValidIndex(Right) ?
            (Pose[Left].GetLocation() + Pose[Right].GetLocation()) * .5f : FVector::ZeroVector;
        // A common shoulder pivot keeps both rifle grips together while the body lies flat.
        const FQuat Arms(FVector::XAxisVector, Blend * PI * .5f);
        const FQuat Head(FVector::XAxisVector, Blend * FMath::DegreesToRadians(70.f));
        TArray<FTransform> StraightLegs;
        StraightLegs.SetNum(Pose.Num());
        for (int32 Bone = 0; Bone < Pose.Num(); ++Bone)
        {
            const int32 Parent = Ref.GetParentIndex(Bone);
            StraightLegs[Bone] = Parent == INDEX_NONE ? Ref.GetRefBonePose()[Bone] : Ref.GetRefBonePose()[Bone] * StraightLegs[Parent];
        }
        const FVector PelvisShift = Pose.IsValidIndex(Pelvis) ? Original[Pelvis].GetLocation() - StraightLegs[Pelvis].GetLocation() : FVector::ZeroVector;
        for (int32 Bone = 0; Bone < Pose.Num(); ++Bone)
        {
            if (InSubtree(Bone,ThighLeft) || InSubtree(Bone,ThighRight))
            {
                const int32 Parent = Ref.GetParentIndex(Bone);
                if (Pose.IsValidIndex(Parent) && Pelvis != INDEX_NONE)
                {
                    StraightLegs[Bone].AddToTranslation(PelvisShift);
                    Pose[Bone].Blend(Original[Bone], StraightLegs[Bone], Blend);
                }
            }
            else if (InSubtree(Bone,Left) || InSubtree(Bone,Right))
            {
                Pose[Bone].SetLocation(Shoulder + Arms.RotateVector(Original[Bone].GetLocation() - Shoulder));
                Pose[Bone].SetRotation(Arms * Original[Bone].GetRotation());
            }
            else if (InSubtree(Bone,Neck) && Pose.IsValidIndex(Neck))
            {
                const FVector Pivot = Original[Neck].GetLocation();
                Pose[Bone].SetLocation(Pivot + Head.RotateVector(Original[Bone].GetLocation() - Pivot));
                Pose[Bone].SetRotation(Head * Original[Bone].GetRotation());
            }
        }
    }
    Super::FinalizeBoneTransform();
}
