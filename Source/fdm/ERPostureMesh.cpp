#include "ERPostureMesh.h"
#include "ERCharacter.h"
#include "ERMovement.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalMeshSocket.h"
#include "ERWeapon.h"

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
    const auto* Data=C && C->Weapon ? C->Weapon->Data.Get() : nullptr;
    if (Mesh && Data && Data->bFitSupportHand)
    {
        const auto& Ref=Mesh->GetRefSkeleton();auto& Pose=GetEditableComponentSpaceTransforms();
        const auto* Right=Mesh->FindSocket(TEXT("HandGrip_R"));
        const auto* Surface=Data->SupportSurfaceContacts.IsEmpty() ? nullptr : Mesh->FindSocket(TEXT("ER_Palm_L"));
        const auto* Left=Surface ? Surface : Mesh->FindSocket(TEXT("HandGrip_L"));
        auto FitArm=[&](const TCHAR* UpperName,const TCHAR* LowerName,const TCHAR* HandName,FVector Target,FQuat Rotation)
        {
            const int32 Upper=Ref.FindBoneIndex(UpperName),Lower=Ref.FindBoneIndex(LowerName),Hand=Ref.FindBoneIndex(HandName);
            if (!Pose.IsValidIndex(Upper) || !Pose.IsValidIndex(Lower) || !Pose.IsValidIndex(Hand)) return;
            const TArray<FTransform> Original=Pose;
            const FVector Shoulder=Pose[Upper].GetLocation(),Elbow=Pose[Lower].GetLocation(),Wrist=Pose[Hand].GetLocation();
            const float A=FVector::Distance(Shoulder,Elbow),B=FVector::Distance(Elbow,Wrist);
            const FVector Direction=(Target-Shoulder).GetSafeNormal();
            const float D=FMath::Clamp(FVector::Distance(Shoulder,Target),FMath::Abs(A-B)+.1f,A+B-.1f);
            FVector Bend=FVector::VectorPlaneProject(Elbow-Shoulder,Direction).GetSafeNormal();
            if (Bend.IsNearlyZero()) Bend=FVector::VectorPlaneProject(-FVector::ZAxisVector,Direction).GetSafeNormal();
            const float Along=(A*A-B*B+D*D)/(2.f*D);
            const FVector NewElbow=Shoulder+Direction*Along+Bend*FMath::Sqrt(FMath::Max(0.f,A*A-Along*Along));
            const FVector NewWrist=Shoulder+Direction*D;
            FTransform NewUpper=Pose[Upper],NewLower=Pose[Lower],NewHand=Pose[Hand];
            NewUpper.SetRotation(FQuat::FindBetweenNormals((Elbow-Shoulder).GetSafeNormal(),(NewElbow-Shoulder).GetSafeNormal())*NewUpper.GetRotation());
            NewLower.SetRotation(FQuat::FindBetweenNormals((Wrist-Elbow).GetSafeNormal(),(NewWrist-NewElbow).GetSafeNormal())*NewLower.GetRotation());
            NewLower.SetLocation(NewElbow);NewHand.SetLocation(NewWrist);NewHand.SetRotation(Rotation);
            for (int32 Bone=Upper;Bone<Pose.Num();++Bone)
            {
                int32 Ancestor=Bone;
                while (Ancestor!=INDEX_NONE && Ancestor!=Upper && Ancestor!=Lower && Ancestor!=Hand) Ancestor=Ref.GetParentIndex(Ancestor);
                const FTransform* Replacement=Ancestor==Hand ? &NewHand : Ancestor==Lower ? &NewLower : Ancestor==Upper ? &NewUpper : nullptr;
                if (Replacement) Pose[Bone]=Original[Bone].GetRelativeTransform(Original[Ancestor]) * *Replacement;
            }
        };
        const int32 RightBone=Right ? Ref.FindBoneIndex(Right->BoneName) : INDEX_NONE;
        const int32 Upper=Ref.FindBoneIndex(TEXT("upperarm_l")),Lower=Ref.FindBoneIndex(TEXT("lowerarm_l")),Hand=Ref.FindBoneIndex(TEXT("hand_l"));
        if (Right && Left && Pose.IsValidIndex(RightBone) && Pose.IsValidIndex(Upper) && Pose.IsValidIndex(Lower) && Pose.IsValidIndex(Hand))
        {
            FTransform Weapon=Data->GripTransform * Right->GetSocketLocalTransform() * Pose[RightBone];
            if (Surface)
            {
                // Seat the stock near the shoulder rather than holding the gun at full reach.
                FitArm(TEXT("upperarm_r"),TEXT("lowerarm_r"),TEXT("hand_r"),Pose[RightBone].GetLocation()-Weapon.GetUnitAxis(EAxis::X)*12.f,Pose[RightBone].GetRotation());
                Weapon=Data->GripTransform * Right->GetSocketLocalTransform() * Pose[RightBone];
            }
            const FQuat Rotation=Surface ? Weapon.GetRotation()*Data->SupportPalmRotation.Quaternion()*Left->GetSocketLocalTransform().GetRotation().Inverse() : Pose[Hand].GetRotation();
            const FVector Offset=Rotation.RotateVector(Left->RelativeLocation*Pose[Hand].GetScale3D());
            const FVector Shoulder=Pose[Upper].GetLocation();
            const float Reach=FVector::Distance(Shoulder,Pose[Lower].GetLocation())+FVector::Distance(Pose[Lower].GetLocation(),Pose[Hand].GetLocation())-.1f;
            FVector Target=Weapon.TransformPosition(Data->SupportHandOffset)-Offset;
            if (Surface)
            {
                for (const FVector& Point : Data->SupportSurfaceContacts)
                {
                    Target=Weapon.TransformPosition(Point)-Offset;
                    if (FVector::Distance(Shoulder,Target)<=Reach) break;
                }
            }
            else for (float Slide=.25f;Slide<=10.f && FVector::Distance(Shoulder,Target)>Reach;Slide+=.25f)
                Target=Weapon.TransformPosition(Data->SupportHandOffset-FVector(Slide,0,0))-Offset;
            FitArm(TEXT("upperarm_l"),TEXT("lowerarm_l"),TEXT("hand_l"),Target,Rotation);
        }
    }
    Super::FinalizeBoneTransform();
}
