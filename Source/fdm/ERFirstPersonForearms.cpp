#include "ERCharacter.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalMeshSocket.h"

#if WITH_EDITOR
#include "MeshDescription.h"
#include "SkeletalMeshAttributes.h"
#include "Animation/Skeleton.h"
#include "Rendering/SkeletalMeshModel.h"
#include "Rendering/SkeletalMeshLODModel.h"
#include "Engine/Blueprint.h"
#include "Serialization/ArchiveReplaceObjectRef.h"
#include "UObject/UObjectHash.h"
#include "UObject/Package.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

namespace
{
TArray<FTransform> ReferenceFrames(const FReferenceSkeleton& Skeleton)
{
    TArray<FTransform> Frames = Skeleton.GetRefBonePose();
    for (int32 Bone = 0; Bone < Frames.Num(); ++Bone)
        if (const int32 Parent = Skeleton.GetParentIndex(Bone); Parent != INDEX_NONE)
            Frames[Bone] *= Frames[Parent];
    return Frames;
}

bool IsForearmBone(const FReferenceSkeleton& Skeleton, int32 Bone, const TArray<FName>& Excluded)
{
    bool bForearm = false;
    for (; Bone != INDEX_NONE; Bone = Skeleton.GetParentIndex(Bone))
    {
        const FName Name = Skeleton.GetBoneName(Bone);
        if (Excluded.Contains(Name)) return false;
        if (Name.ToString().StartsWith(TEXT("lowerarm_"))) bForearm = true;
    }
    return bForearm;
}
}
#endif

bool AERCharacter::DeleteUnloadedParagonPackages(const TArray<FString>& Packages)
{
#if WITH_EDITOR
    auto& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
    const bool bRuntimePrune = FParse::Param(FCommandLine::Get(), TEXT("ERDeleteUnusedCharacters"));
    if (!bRuntimePrune && !IFileManager::Get().FileExists(*(FPaths::ProjectSavedDir() / TEXT("AssetAudit/paragon-backup-manifest.json")))) return false;
    TSet<FName> Batch;
    TMap<FName, FString> Files;
    for (const FString& Name : Packages)
    {
        if (!FPackageName::IsValidLongPackageName(Name) ||
            !(Name.StartsWith(TEXT("/Game/ParagonTwinblast/")) || Name.StartsWith(TEXT("/Game/ParagonDrongo/")) ||
              Name.StartsWith(TEXT("/Game/ParagonPhase/")) || Name.StartsWith(TEXT("/Game/ParagonMurdock/")) || Name.StartsWith(TEXT("/Game/ParagonLtBelica/")) ||
              Name.StartsWith(TEXT("/Game/ParagonGadget/")) || Name.StartsWith(TEXT("/Game/ParagonWraith/")) ||
              Name.StartsWith(TEXT("/Game/EclipseRecoil/Characters/")) || Name.StartsWith(TEXT("/Game/Characters/")))) return false;
        if (FindPackage(nullptr, *Name))
        { UE_LOG(LogTemp, Error, TEXT("ER_PACKAGE_PRUNE loaded_package=%s"), *Name); return false; }
        FString Filename;
        if (!FPackageName::DoesPackageExist(Name, &Filename)) return false;
        Batch.Add(FName(*Name)); Files.Add(FName(*Name), Filename);
    }
    // Validate the entire batch before changing any file. Only packages in this
    // independently audited set may refer to another member of the set.
    for (FName Name : Batch)
    {
        TArray<FName> References;
        Registry.GetReferencers(Name, References, UE::AssetRegistry::EDependencyCategory::Package | UE::AssetRegistry::EDependencyCategory::Manage);
        for (FName Reference : References)
            if (!Batch.Contains(Reference))
            { UE_LOG(LogTemp, Error, TEXT("ER_PACKAGE_PRUNE outside_reference=%s target=%s"), *Reference.ToString(), *Name.ToString()); return false; }
    }
    int32 Removed = 0;
    for (const auto& Pair : Files)
    {
        // Editor package deletion without loading incompatible legacy demo
        // Worlds. PackageDeleted removes their entries from Unreal's registry.
        if (!IFileManager::Get().Delete(*Pair.Value, false, true)) return false;
        Registry.PackageDeleted(CreatePackage(*Pair.Key.ToString()));
        if (++Removed % 100 == 0) UE_LOG(LogTemp, Display, TEXT("ER_PACKAGE_PRUNE_PROGRESS removed=%d total=%d"), Removed, Files.Num());
    }
    return true;
#else
    return false;
#endif
}

bool AERCharacter::PrepareTeamSkeleton(USkeleton* Skeleton, USkeletalMesh* PreviewMesh)
{
#if WITH_EDITOR
    if (!Skeleton || !PreviewMesh || PreviewMesh->GetSkeleton() != Skeleton ||
        !Skeleton->GetPathName().StartsWith(TEXT("/Game/EclipseRecoil/Characters/Teams/")) ||
        !PreviewMesh->GetPathName().StartsWith(TEXT("/Game/EclipseRecoil/Characters/Teams/"))) return false;
    Skeleton->Modify();
    const auto Compatible = Skeleton->GetCompatibleSkeletons();
    for (const auto& Other : Compatible)
        if (Other.ToSoftObjectPath().ToString().StartsWith(TEXT("/Game/Paragon")))
            Skeleton->RemoveCompatibleSkeleton(Other);
    // Preserve retarget pose data and translation modes. Only replace the
    // editor's source mesh link; runtime animation transforms stay unchanged.
    for (auto& Pair : Skeleton->AnimRetargetSources)
        if (Pair.Value.SourceReferenceMesh.ToSoftObjectPath().ToString().StartsWith(TEXT("/Game/Paragon")))
            Pair.Value.SourceReferenceMesh = PreviewMesh;
    Skeleton->SetPreviewMesh(PreviewMesh);
    Skeleton->MarkPackageDirty();
    return true;
#else
    return false;
#endif
}

bool AERCharacter::PrepareFirstPersonSkeleton(USkeleton* Skeleton, USkeletalMesh* PreviewMesh)
{
#if WITH_EDITOR
    if (!Skeleton || !PreviewMesh || !Skeleton->GetPathName().StartsWith(TEXT("/Game/EclipseRecoil/Characters/FirstPerson/"))) return false;
    Skeleton->Modify();
    const auto Compatible = Skeleton->GetCompatibleSkeletons();
    for (const auto& Other : Compatible)
        if (!Other.ToSoftObjectPath().ToString().StartsWith(TEXT("/Game/Characters/Mannequins/"))) Skeleton->RemoveCompatibleSkeleton(Other);
    Skeleton->AnimRetargetSources.Empty();
    Skeleton->SetPreviewMesh(PreviewMesh);
    Skeleton->MarkPackageDirty();
    return true;
#else
    return false;
#endif
}

bool AERCharacter::ConfigureFirstPersonTranslationTracks(USkeleton* Skeleton, const TArray<FName>& Bones)
{
#if WITH_EDITOR
    if (!Skeleton || Skeleton->GetPathName() != TEXT("/Game/EclipseRecoil/Characters/FirstPerson/SKEL_ER_FirstPerson.SKEL_ER_FirstPerson")) return false;
    for (FName Bone : Bones)
        if (Skeleton->GetReferenceSkeleton().FindBoneIndex(Bone) == INDEX_NONE) return false;
    Skeleton->Modify();
    for (FName Bone : Bones)
        Skeleton->SetBoneTranslationRetargetingMode(Skeleton->GetReferenceSkeleton().FindBoneIndex(Bone), EBoneTranslationRetargetingMode::Animation);
    Skeleton->MarkPackageDirty();
    return true;
#else
    return false;
#endif
}

bool AERCharacter::ReplaceOwnedAssetReferences(UObject* Asset, const TArray<UObject*>& Sources, const TArray<UObject*>& Replacements)
{
#if WITH_EDITOR
    if (!Asset || Sources.Num() != Replacements.Num() || !Asset->GetPathName().StartsWith(TEXT("/Game/EclipseRecoil/"))) return false;
    TMap<UObject*, UObject*> Map;
    for (int32 I = 0; I < Sources.Num(); ++I)
    {
        if (!Sources[I] || !Replacements[I]) return false;
        Map.Add(Sources[I], Replacements[I]);
        if (const auto* Before = Cast<UBlueprint>(Sources[I]))
        {
            const auto* After = Cast<UBlueprint>(Replacements[I]);
            if (!After || !Before->GeneratedClass || !After->GeneratedClass) return false;
            Map.Add(Before->GeneratedClass, After->GeneratedClass);
            Map.Add(Before->GeneratedClass->GetDefaultObject(), After->GeneratedClass->GetDefaultObject());
        }
    }
    Asset->Modify();
    // Include generated classes, CDOs, graph nodes and component templates in
    // this owned package. Never consolidate or modify the vendor originals.
    TArray<UObject*> Objects;
    GetObjectsWithOuter(Asset->GetOutermost(), Objects, true);
    // Class serialization can rehash UObjects: finish enumerating the package
    // before replacing anything, rather than serializing inside a hash iterator.
    for (UObject* Object : Objects)
    {
        FArchiveReplaceObjectRef<UObject> Replace(Object, Map, EArchiveReplaceObjectFlags::IgnoreOuterRef);
    }
    Asset->MarkPackageDirty();
    return true;
#else
    return false;
#endif
}

bool AERCharacter::BuildFirstPersonForearms(USkeletalMesh* Destination, USkeletalMesh* SkinMesh,
    USkeletalMesh* FirstPersonRig, const TArray<int32>& ExcludedMaterials, const TArray<FName>& ExcludedBones)
{
#if WITH_EDITOR
    if (!Destination || !SkinMesh || !FirstPersonRig || Destination == SkinMesh ||
        !Destination->GetPathName().StartsWith(TEXT("/Game/EclipseRecoil/Characters/Teams/"))) return false;

    const FReferenceSkeleton& SourceRef = SkinMesh->GetRefSkeleton();
    const FReferenceSkeleton& RigRef = FirstPersonRig->GetRefSkeleton();
    const TArray<FTransform> SourceFrames = ReferenceFrames(SourceRef);
    const TArray<FTransform> RigFrames = ReferenceFrames(RigRef);
    TArray<int32> RigBones, SourceAnchors;
    TArray<float> SizeRatios;
    TArray<bool> ForearmBones;
    for (int32 Bone = 0; Bone < SourceRef.GetNum(); ++Bone)
    {
        int32 Anchor = Bone;
        int32 Match = INDEX_NONE;
        // Paragon forearm twist helpers do not share the FPS rig's authored twist
        // motion. Bind their geometry to the actual elbow-to-wrist segment.
        const FString SourceName = SourceRef.GetBoneName(Bone).ToString();
        if (SourceName.StartsWith(TEXT("lowerarm_")))
            Anchor = SourceRef.FindBoneIndex(FName(*(TEXT("lowerarm_") +
                FString(SourceName.EndsWith(TEXT("_l")) ? TEXT("l") : TEXT("r")))));
        while (Anchor != INDEX_NONE && (Match = RigRef.FindBoneIndex(SourceRef.GetBoneName(Anchor))) == INDEX_NONE)
            Anchor = SourceRef.GetParentIndex(Anchor);
        if (Match == INDEX_NONE) return false;
        SourceAnchors.Add(Anchor);
        RigBones.Add(Match);
        ForearmBones.Add(IsForearmBone(SourceRef, Bone, ExcludedBones));

        // Fit the skin's elbow-to-wrist length and palm size to the old FPS rig.
        const FString Name = SourceRef.GetBoneName(Anchor).ToString();
        const FString Side = Name.EndsWith(TEXT("_l")) ? TEXT("l") : TEXT("r");
        FName EndBone = NAME_None;
        if (Name.StartsWith(TEXT("lowerarm"))) EndBone = FName(*(TEXT("hand_") + Side));
        else if (Name.StartsWith(TEXT("hand_"))) EndBone = FName(*(TEXT("middle_01_") + Side));
        float Ratio = 1.f;
        const int32 SourceEnd = SourceRef.FindBoneIndex(EndBone);
        const int32 RigEnd = RigRef.FindBoneIndex(EndBone);
        if (SourceEnd != INDEX_NONE && RigEnd != INDEX_NONE)
            Ratio = FVector::Distance(RigFrames[Match].GetLocation(), RigFrames[RigEnd].GetLocation()) /
                FMath::Max(0.01, FVector::Distance(SourceFrames[Anchor].GetLocation(), SourceFrames[SourceEnd].GetLocation()));
        SizeRatios.Add(Ratio);
    }

    Destination->Modify();
    Destination->SetSkeleton(FirstPersonRig->GetSkeleton());
    Destination->SetRefSkeleton(RigRef);
    Destination->SetPhysicsAsset(nullptr);
    Destination->SetShadowPhysicsAsset(nullptr);
    Destination->SetPostProcessAnimBlueprint(FirstPersonRig->GetPostProcessAnimBlueprint());
    Destination->GetMeshClothingAssets().Empty();
    Destination->SetHasActiveClothingAssets(false);
    Destination->UnregisterAllMorphTarget();
    Destination->GetMeshOnlySocketList().Empty();
    for (const FName Name : {FName(TEXT("HandGrip_R")), FName(TEXT("HandGrip_L"))})
    {
        const USkeletalMeshSocket* Socket = FirstPersonRig->FindSocket(Name);
        if (!Socket) return false;
        Destination->AddSocket(DuplicateObject<USkeletalMeshSocket>(Socket, Destination));
    }
    Destination->RebuildSocketMap();
    Destination->SetMaterials(SkinMesh->GetMaterials());

    for (int32 LOD = 0; LOD < SkinMesh->GetLODNum(); ++LOD)
    {
        const FMeshDescription* Original = SkinMesh->GetMeshDescription(LOD);
        // Some Paragon skins have generated LODs without imported descriptions.
        // Rebuild those from the extracted base geometry using their reduction settings.
        if (!Original) Original = SkinMesh->GetMeshDescription(0);
        if (!Original)
        {
            UE_LOG(LogTemp, Error, TEXT("ER_FPS_FOREARM_BUILD missing mesh description: %s LOD=%d"), *SkinMesh->GetName(), LOD);
            return false;
        }
        FMeshDescription Description = *Original;
        FSkeletalMeshAttributes Attributes(Description);
        TArray<int32> DescriptionBones;
        DescriptionBones.Init(INDEX_NONE, Attributes.Bones().GetArraySize());
        int32 DifferentIndices = 0;
        for (const FBoneID ID : Attributes.Bones().GetElementIDs())
        {
            const int32 SourceIndex = SourceRef.FindBoneIndex(Attributes.GetBoneNames()[ID]);
            if (SourceIndex == INDEX_NONE) return false;
            DescriptionBones[ID.GetValue()] = SourceIndex;
            DifferentIndices += SourceIndex != ID.GetValue();
        }
        UE_LOG(LogTemp,Display,TEXT("ER_FPS_FOREARM_BONE_MAP mesh=%s LOD=%d reordered=%d source=%d rig=%d"),
            *SkinMesh->GetName(),LOD,DifferentIndices,SourceRef.GetRawBoneNum(),RigRef.GetRawBoneNum());
        auto Weights = Attributes.GetVertexSkinWeights();
        const auto SlotNames = Attributes.GetPolygonGroupMaterialSlotNames();
        TSet<FName> ExcludedSlots;
        for (const int32 Slot : ExcludedMaterials)
            if (SkinMesh->GetMaterials().IsValidIndex(Slot)) ExcludedSlots.Add(SkinMesh->GetMaterials()[Slot].MaterialSlotName);
        TArray<FTriangleID> Remove;
        const int32 OriginalTriangles = Description.Triangles().Num();
        for (const FTriangleID Triangle : Description.Triangles().GetElementIDs())
        {
            bool bKeep = !ExcludedSlots.Contains(SlotNames[Description.GetTrianglePolygonGroup(Triangle)]);
            for (const FVertexInstanceID Instance : Description.GetTriangleVertexInstances(Triangle))
            {
                float ForearmWeight = 0.f;
                for (const auto Weight : Weights.Get(Description.GetVertexInstanceVertex(Instance)))
                {
                    const int32 Bone = DescriptionBones[Weight.GetBoneIndex()];
                    if (ForearmBones[Bone]) ForearmWeight += Weight.GetWeight();
                }
                bKeep &= ForearmWeight >= .5f;
            }
            if (!bKeep) Remove.Add(Triangle);
        }
        // Imported skeletal descriptions own polygons as well as triangles.
        // Delete their polygons so the polygon-group indexes stay consistent.
        TSet<FPolygonID> RemovePolygons;
        for (const FTriangleID Triangle : Remove) RemovePolygons.Add(Description.GetTrianglePolygon(Triangle));
        Description.DeletePolygons(RemovePolygons.Array());
        if (Description.Triangles().Num() == 0) return false;
        const int32 RetainedTriangles = Description.Triangles().Num();
        FElementIDRemappings Remappings;
        Description.Compact(Remappings);
        Weights = Attributes.GetVertexSkinWeights();
        auto Positions = Attributes.GetVertexPositions();
        auto Normals = Attributes.GetVertexInstanceNormals();
        auto Tangents = Attributes.GetVertexInstanceTangents();

        // Rebind retained skin geometry to exactly the old forearm/hand frames.
        // UVs/materials stay with the selected skin; every weapon pose stays on the FPS rig.
        TMap<FVertexID, TArray<UE::AnimationCore::FBoneWeight>> OriginalWeights;
        for (const FVertexID Vertex : Description.Vertices().GetElementIDs())
        {
            FVector Position = FVector::ZeroVector;
            TMap<int32, float> MappedWeights;
            TArray<UE::AnimationCore::FBoneWeight>& Saved = OriginalWeights.Add(Vertex);
            for (const auto Weight : Weights.Get(Vertex))
            {
                const int32 Bone = DescriptionBones[Weight.GetBoneIndex()];
                Saved.Emplace(Bone,Weight.GetWeight());
                const int32 Anchor = SourceAnchors[Bone];
                const FVector Local = SourceFrames[Anchor].InverseTransformPosition(FVector(Positions[Vertex]));
                Position += RigFrames[RigBones[Bone]].TransformPosition(Local * SizeRatios[Bone]) * Weight.GetWeight();
                MappedWeights.FindOrAdd(RigBones[Bone]) += Weight.GetWeight();
            }
            Positions[Vertex] = FVector3f(Position);
            TArray<UE::AnimationCore::FBoneWeight> NewWeights;
            for (const auto& Pair : MappedWeights) NewWeights.Emplace(Pair.Key, Pair.Value);
            Weights.Set(Vertex, NewWeights);
        }
        for (const FVertexInstanceID Instance : Description.VertexInstances().GetElementIDs())
        {
            FVector Normal = FVector::ZeroVector, Tangent = FVector::ZeroVector;
            for (const auto Weight : OriginalWeights[Description.GetVertexInstanceVertex(Instance)])
            {
                const int32 Bone = Weight.GetBoneIndex();
                const FQuat Rotation = RigFrames[RigBones[Bone]].GetRotation() * SourceFrames[SourceAnchors[Bone]].GetRotation().Inverse();
                Normal += Rotation.RotateVector(FVector(Normals[Instance])) * Weight.GetWeight();
                Tangent += Rotation.RotateVector(FVector(Tangents[Instance])) * Weight.GetWeight();
            }
            Normals[Instance] = FVector3f(Normal.GetSafeNormal());
            Tangents[Instance] = FVector3f(Tangent.GetSafeNormal());
        }
        TArray<FBoneID> OldBones;
        for (const FBoneID Bone : Attributes.Bones().GetElementIDs()) OldBones.Add(Bone);
        for (const FBoneID Bone : OldBones) Attributes.DeleteBone(Bone);
        for (int32 Bone = 0; Bone < RigRef.GetNum(); ++Bone)
        {
            const FBoneID ID(Bone);
            Attributes.CreateBone(ID);
            Attributes.GetBoneNames()[ID] = RigRef.GetBoneName(Bone);
            Attributes.GetBoneParentIndices()[ID] = RigRef.GetParentIndex(Bone);
            Attributes.GetBonePoses()[ID] = RigRef.GetRefBonePose()[Bone];
        }
        for (const FName Profile : Attributes.GetSkinWeightProfileNames(true)) Attributes.UnregisterSkinWeightAttribute(Profile);
        for (const FName Morph : Attributes.GetMorphTargetNames()) Attributes.UnregisterMorphTargetAttribute(Morph);
        Destination->CreateMeshDescription(LOD, MoveTemp(Description));
        USkeletalMesh::FCommitMeshDescriptionParams Params;
        Params.bUpdateMorphTargets = false;
        Params.bForceUpdate = true;
        if (!Destination->CommitMeshDescription(LOD, Params)) return false;
        if (FSkeletalMeshLODInfo* Info = Destination->GetLODInfo(LOD))
        {
            Info->LODMaterialMap.Empty();
            Info->BuildSettings.bRecomputeNormals = false;
            Info->BuildSettings.bRecomputeTangents = false;
        }
        UE_LOG(LogTemp, Display, TEXT("ER_FPS_FOREARM_BUILD mesh=%s LOD=%d triangles=%d/%d rig=%s"),
            *Destination->GetName(), LOD, RetainedTriangles, OriginalTriangles, *FirstPersonRig->GetName());
    }
    Destination->GetRefBasesInvMatrix().Empty();
    Destination->CalculateInvRefMatrices();
    // The forearm-only reference bounds exclude the camera while the authored FPS
    // pose reaches into it. Keep the same culling volume as the old viewmodel.
    Destination->SetImportedBounds(FirstPersonRig->GetImportedBounds());
    Destination->InvalidateDeriveDataCacheGUID();
    Destination->PostEditChange();
    const auto* Model = Destination->GetImportedModel();
    UE_LOG(LogTemp,Display,TEXT("ER_FPS_FOREARM_RENDER mesh=%s triangles=%d bones=%d bind=%d"),
        *Destination->GetName(),Model->LODModels[0].IndexBuffer.Num()/3,Destination->GetRefSkeleton().GetRawBoneNum(),Destination->GetRefBasesInvMatrix().Num());
    Destination->MarkPackageDirty();
    return true;
#else
    return false;
#endif
}
