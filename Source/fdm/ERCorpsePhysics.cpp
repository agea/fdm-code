#include "ERCorpse.h"
#include "Engine/SkeletalMesh.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/SkeletalBodySetup.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"

bool AERCorpse::BuildTeamRagdollPhysics(UPhysicsAsset* Asset, USkeletalMesh* Mesh, float LimbSwing, float LegSwing)
{
#if WITH_EDITOR
    if (!Asset || !Mesh || !Asset->GetPathName().StartsWith(TEXT("/Game/EclipseRecoil/Characters/Teams/")) ||
        !Mesh->GetPathName().StartsWith(TEXT("/Game/EclipseRecoil/Characters/Teams/"))) return false;
    const auto& Ref = Mesh->GetRefSkeleton();
    struct FPart { const TCHAR* Bone; const TCHAR* Parent; const TCHAR* End; float Radius; float Mass; };
    const FPart Parts[] = {
        {TEXT("pelvis"),nullptr,nullptr,0,9}, {TEXT("spine_01"),TEXT("pelvis"),nullptr,0,10},
        {TEXT("spine_03"),TEXT("spine_01"),nullptr,0,15}, {TEXT("head"),TEXT("spine_03"),nullptr,8,4},
        {TEXT("upperarm_l"),TEXT("spine_03"),TEXT("lowerarm_l"),4,2}, {TEXT("lowerarm_l"),TEXT("upperarm_l"),TEXT("hand_l"),4,2},
        {TEXT("hand_l"),TEXT("lowerarm_l"),nullptr,4,1},
        {TEXT("upperarm_r"),TEXT("spine_03"),TEXT("lowerarm_r"),4,2}, {TEXT("lowerarm_r"),TEXT("upperarm_r"),TEXT("hand_r"),4,2},
        {TEXT("hand_r"),TEXT("lowerarm_r"),nullptr,4,1},
        {TEXT("thigh_l"),TEXT("pelvis"),TEXT("calf_l"),7,5}, {TEXT("calf_l"),TEXT("thigh_l"),TEXT("foot_l"),5,4},
        {TEXT("foot_l"),TEXT("calf_l"),nullptr,0,1},
        {TEXT("thigh_r"),TEXT("pelvis"),TEXT("calf_r"),7,5}, {TEXT("calf_r"),TEXT("thigh_r"),TEXT("foot_r"),5,4},
        {TEXT("foot_r"),TEXT("calf_r"),nullptr,0,1}
    };
    auto Reference = [&Ref](FName Name)
    {
        int32 Bone = Ref.FindBoneIndex(Name);
        FTransform Result = Ref.GetRefBonePose()[Bone];
        while ((Bone = Ref.GetParentIndex(Bone)) != INDEX_NONE) Result *= Ref.GetRefBonePose()[Bone];
        return Result;
    };
    for (const auto& Part : Parts)
        if (Ref.FindBoneIndex(Part.Bone) == INDEX_NONE || (Part.End && Ref.FindBoneIndex(Part.End) == INDEX_NONE)) return false;
    Asset->Modify();
    Asset->SkeletalBodySetups.Reset(); Asset->ConstraintSetup.Reset(); Asset->CollisionDisableTable.Reset();
    for (const auto& Part : Parts)
    {
        auto* Setup = NewObject<USkeletalBodySetup>(Asset, NAME_None, RF_Transactional);
        Setup->BoneName = Part.Bone;
        Setup->PhysicsType = EPhysicsType::PhysType_Simulated;
        Setup->bConsiderForBounds = true;
        Setup->DefaultInstance.SetMassOverride(Part.Mass);
        Setup->DefaultInstance.LinearDamping = .6f;
        Setup->DefaultInstance.AngularDamping = 2.f;
        Setup->DefaultInstance.SetPositionSolverIterationCount(12);
        const FTransform Bone = Reference(Part.Bone);
        if (Part.End)
        {
            const FVector End = Bone.InverseTransformPosition(Reference(Part.End).GetLocation());
            FKSphylElem Shape;
            Shape.Radius = Part.Radius;
            Shape.Length = FMath::Max(End.Size() - 2.f * Part.Radius, 2.f);
            Shape.Center = End * .5f;
            Shape.Rotation = FQuat::FindBetweenNormals(FVector::UpVector, End.GetSafeNormal()).Rotator();
            Setup->AggGeom.SphylElems.Add(Shape);
        }
        else if (Part.Radius > 0)
        {
            FKSphereElem Shape;
            Shape.Radius = Part.Radius;
            Shape.Center = Bone.InverseTransformVectorNoScale(FVector(0, 0, FCString::Strcmp(Part.Bone, TEXT("head")) == 0 ? 4 : 0));
            Setup->AggGeom.SphereElems.Add(Shape);
        }
        else
        {
            const FString Name(Part.Bone);
            const FVector Extent = Name.StartsWith(TEXT("foot")) ? FVector(5,11,4) : Name == TEXT("pelvis") ? FVector(13,10,9) : FVector(13,9,Name == TEXT("spine_03") ? 14 : 10);
            const FVector Offset = Name.StartsWith(TEXT("foot")) ? FVector(0,7,-3) : FVector(0,0,Name == TEXT("spine_03") ? -5 : 0);
            FKBoxElem Shape;
            Shape.X = Extent.X * 2; Shape.Y = Extent.Y * 2; Shape.Z = Extent.Z * 2;
            Shape.Center = Bone.InverseTransformVectorNoScale(Offset);
            Shape.Rotation = Bone.GetRotation().Inverse().Rotator();
            Setup->AggGeom.BoxElems.Add(Shape);
        }
        Setup->InvalidatePhysicsData(); Setup->CreatePhysicsMeshes();
        Asset->SkeletalBodySetups.Add(Setup);
        if (Part.Parent)
        {
            auto* Joint = NewObject<UPhysicsConstraintTemplate>(Asset, NAME_None, RF_Transactional);
            auto& Constraint = Joint->DefaultInstance;
            Constraint.JointName = Part.Bone; Constraint.ConstraintBone1 = Part.Bone; Constraint.ConstraintBone2 = Part.Parent;
            Constraint.SetRefFrame(EConstraintFrame::Frame1, FTransform::Identity);
            Constraint.SetRefFrame(EConstraintFrame::Frame2, Bone.GetRelativeTransform(Reference(Part.Parent)));
            Constraint.SetLinearXMotion(LCM_Locked); Constraint.SetLinearYMotion(LCM_Locked); Constraint.SetLinearZMotion(LCM_Locked);
            const FString Name(Part.Bone);
            const bool bFieldFit=LegSwing<65.f;
            const bool bLeg=Name.StartsWith(TEXT("thigh")) || Name.StartsWith(TEXT("calf")) || Name.StartsWith(TEXT("foot"));
            float Swing = Name.StartsWith(TEXT("spine")) ? 25.f : Name == TEXT("head") ? 40.f : bLeg ? LegSwing : LimbSwing;
            if (bFieldFit && Name.StartsWith(TEXT("calf"))) Swing=FMath::Min(Swing,25.f);
            if (bFieldFit && Name.StartsWith(TEXT("foot"))) Swing=FMath::Min(Swing,15.f);
            if (bFieldFit && Name.StartsWith(TEXT("hand"))) Swing=FMath::Min(Swing,20.f);
            Constraint.SetAngularSwing1Limit(ACM_Limited, Swing); Constraint.SetAngularSwing2Limit(ACM_Limited, Swing);
            Constraint.SetAngularTwistLimit(ACM_Limited, bFieldFit && bLeg ? 15.f : 35.f);
            Constraint.SetProjectionParams(true, .5f, .2f, 1.f, 15.f);
            Constraint.SetDisableCollision(true);
            // Editor serialization writes DefaultProfile back into the
            // instance. Commit the authored locks/limits there before saving.
            Joint->SetDefaultProfile(Constraint);
            Asset->ConstraintSetup.Add(Joint);
        }
    }
    // Cosmetic ragdolls contact only the map, not other parts of themselves.
    for (int32 A = 0; A < Asset->SkeletalBodySetups.Num(); ++A)
        for (int32 B = A + 1; B < Asset->SkeletalBodySetups.Num(); ++B) Asset->DisableCollision(A, B);
    Asset->UpdateBodySetupIndexMap(); Asset->UpdateBoundsBodiesArray(); Asset->SetPreviewMesh(Mesh);
    Asset->PostEditChange(); Asset->MarkPackageDirty();
    Mesh->Modify(); Mesh->SetPhysicsAsset(Asset); Mesh->PostEditChange(); Mesh->MarkPackageDirty();
    return true;
#else
    return false;
#endif
}
