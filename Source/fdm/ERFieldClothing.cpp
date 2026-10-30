#include "ERCharacter.h"
#include "Engine/SkeletalMesh.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/SkeletalBodySetup.h"
#if WITH_EDITOR
#include "ClothingAsset.h"
#include "ClothingAssetFactory.h"
#include "ClothingSimulationFactory.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "SkeletalMeshClothingSystemUtilities.h"
#include "Rendering/SkeletalMeshModel.h"
#include "ChaosCloth/ChaosClothConfig.h"
#endif

#if WITH_EDITOR
namespace
{
UPhysicsAsset* MakeClothCollision(USkeletalMesh* Mesh,bool Skirt,UPhysicsAsset* Source)
{
    if(!Source)return nullptr;
    // A separate embedded asset cannot affect hit traces, ragdoll contacts or mass.
    auto* Physics=FindObject<UPhysicsAsset>(Mesh,TEXT("ER_FieldClothCollision"));
    if(!Physics)Physics=NewObject<UPhysicsAsset>(Mesh,TEXT("ER_FieldClothCollision"),RF_Transactional);
    Physics->Modify();Physics->SkeletalBodySetups.Reset();Physics->ConstraintSetup.Reset();Physics->CollisionDisableTable.Reset();
    for(auto Original:Source->SkeletalBodySetups)
    {
        const FString Name=Original->BoneName.ToString();
        if(!(Name==TEXT("pelvis") || Name.StartsWith(TEXT("spine_")) || Name.StartsWith(TEXT("thigh_")) || Name.StartsWith(TEXT("calf_")) || (!Skirt && Name.StartsWith(TEXT("upperarm_")))))continue;
        if(Mesh->GetRefSkeleton().FindBoneIndex(Original->BoneName)==INDEX_NONE)continue;
        auto* Body=DuplicateObject<USkeletalBodySetup>(Original,Physics);
        if(Skirt)
        {
            // Chaos cloth uses rounded primitives; cover the hips with paired
            // capsules rather than the ragdoll's sharp, inset torso boxes.
            for(const auto& Box:Body->AggGeom.BoxElems)
            {
                const FVector Half(Box.X*.5,Box.Y*.5,Box.Z*.5);
                const float Radius=FMath::Min(Half.Y,Half.Z)*.95f;
                for(float Side:{-1.f,1.f})
                {
                    FKSphylElem Shape;Shape.Radius=Radius;Shape.Length=FMath::Max(.5f,Box.Z-2*Radius);Shape.Rotation=Box.Rotation;
                    Shape.Center=Box.Center+Box.Rotation.RotateVector(FVector(Side*FMath::Max(0.f,float(Half.X)-Radius),0,0));Body->AggGeom.SphylElems.Add(Shape);
                }
            }
            Body->AggGeom.BoxElems.Reset();
            if(Name.StartsWith(TEXT("thigh_")))for(auto& Shape:Body->AggGeom.SphylElems)Shape.Radius+=1.f;
        }
        Body->InvalidatePhysicsData();Physics->SkeletalBodySetups.Add(Body);
    }
    Physics->UpdateBodySetupIndexMap();Physics->MarkPackageDirty();return Physics;
}
}
#endif

FString AERCharacter::InspectFieldClothing(USkeletalMesh* Mesh)
{
    FString Result;
#if WITH_EDITOR
    if(!Mesh)return Result;
    TArray<UClothingAssetBase*> InUse;Mesh->GetClothingAssetsInUse(InUse);
    Result+=FString::Printf(TEXT("active=%d in_use=%d\n"),Mesh->HasActiveClothingAssets(),InUse.Num());
    for(const auto& LOD:Mesh->GetResourceForRendering()->LODRenderData)for(const auto& S:LOD.RenderSections)
        if(S.HasClothingData())Result+=FString::Printf(TEXT("render cloth=%d guid=%s\n"),S.CorrespondClothAssetIndex,*S.ClothingData.AssetGuid.ToString());
    for(auto Asset:Mesh->GetMeshClothingAssets())if(auto* Cloth=Cast<UClothingAssetCommon>(Asset))
    {
        Result+=FString::Printf(TEXT("valid=%d factory=%s used_bones=%d\n"),Cloth->IsValid(),*GetPathNameSafe(UClothingSimulationFactory::GetClothingSimulationFactory(Cloth)),Cloth->UsedBoneNames.Num());
        Result+=FString::Printf(TEXT("cloth=%s physics=%s\n"),*Cloth->GetName(),*GetPathNameSafe(Cloth->PhysicsAsset));
        for(const auto& LOD:Cloth->LodData)for(const auto& Map:LOD.PointWeightMaps)
        {
            float Max=0;int32 Free=0;for(float V:Map.Values){Max=FMath::Max(Max,V);Free+=V>0;}
            Result+=FString::Printf(TEXT("map=%d n=%d free=%d max=%.3f\n"),Map.CurrentTarget,Map.Values.Num(),Free,Max);
        }
        if(Cloth->PhysicsAsset)for(auto B:Cloth->PhysicsAsset->SkeletalBodySetups)
        {
            Result+=FString::Printf(TEXT("bone=%s boxes=%d spheres=%d capsules=%d\n"),*B->BoneName.ToString(),B->AggGeom.BoxElems.Num(),B->AggGeom.SphereElems.Num(),B->AggGeom.SphylElems.Num());
            for(const auto& S:B->AggGeom.SphylElems)Result+=FString::Printf(TEXT(" radius=%.2f length=%.2f center=%s\n"),S.Radius,S.Length,*S.Center.ToString());
        }
    }
#endif
    return Result;
}

bool AERCharacter::ConfigureFieldClothing(USkeletalMesh* TargetMesh,bool Skirt,USkeletalMesh* SourceMesh,UPhysicsAsset* SourceCollision)
{
#if WITH_EDITOR
    if(!TargetMesh || !TargetMesh->GetPathName().StartsWith(TEXT("/Game/EclipseRecoil/Characters/Teams/")))return false;
    TargetMesh->Modify();
    auto* Collision=MakeClothCollision(TargetMesh,Skirt,SourceCollision?SourceCollision:TargetMesh->GetPhysicsAsset());
    if(!Collision || (!Skirt && !SourceMesh))return false;
    if(Skirt)
    {
        auto* Model=TargetMesh->GetImportedModel();if(!Model || Model->LODModels.IsEmpty())return false;
        auto* Factory=NewObject<UClothingAssetFactory>();
        // Mixed skirt/shoe sections keep shoes and waist fixed; only the skirt moves.
        const int32 Count=Model->LODModels[0].Sections.Num();
        for(int32 Section=0;Section<Count;++Section)
        {
            const auto& S=Model->LODModels[0].Sections[Section];
            if(!TargetMesh->GetMaterials().IsValidIndex(S.MaterialIndex))continue;
            const FString Name=TargetMesh->GetMaterials()[S.MaterialIndex].MaterialSlotName.ToString().ToLower();
            if(!Name.Contains(TEXT("leather_skirt")))continue;
            FSkeletalMeshClothBuildParams Params;Params.AssetName=FString::Printf(TEXT("ER_FieldSkirt_%d"),Section);
            Params.LodIndex=0;Params.SourceSection=Section;Params.bRemoveFromMesh=false;Params.PhysicsAsset=TargetMesh->GetPhysicsAsset();
            auto* Cloth=TargetMesh->GetMeshClothingAssets().Num()?Cast<UClothingAssetCommon>(TargetMesh->GetMeshClothingAssets()[0]):nullptr;
            if(!Cloth)
            {
                Cloth=Cast<UClothingAssetCommon>(Factory->CreateFromSkeletalMesh(TargetMesh,Params));if(!Cloth)return false;
                TargetMesh->AddClothingAsset(Cloth);
            }
            auto& LOD=Cloth->LodData[0];auto& Map=LOD.PointWeightMaps[0];
            Map.bEnabled=true;Map.CurrentTarget=uint8(EWeightMapTargetCommon::MaxDistance);
            int32 Free=0;
            for(int32 I=0;I<LOD.PhysicalMeshData.Vertices.Num();++I)
            {
                const auto& V=LOD.PhysicalMeshData.Vertices[I];
                Map.Values[I]=V.Z>60 && V.Z<96 ? FMath::Clamp((96.f-V.Z)*1.6f,0.f,24.f):0.f;
                Free+=Map.Values[I]>0;
            }
            UE_LOG(LogTemp,Display,TEXT("ER_FIELD_CLOTHING_SECTION mesh=%s section=%s vertices=%d free=%d"),*TargetMesh->GetName(),*Name,LOD.PhysicalMeshData.Vertices.Num(),Free);
            if(!Free) {TargetMesh->GetMeshClothingAssets().Remove(Cloth);continue;}
            Cloth->ApplyParameterMasks(true);
            // Bind after painting, including the lower render LODs below.
        }
    }
    int32 Tuned=0;
    for(auto Asset:TargetMesh->GetMeshClothingAssets())if(auto* Cloth=Cast<UClothingAssetCommon>(Asset))
    {
        Cloth->PhysicsAsset=Collision;
        auto* Model=TargetMesh->GetImportedModel();
        TSet<int32> Materials;
        for(const auto& Section:Model->LODModels[0].Sections)
        {
            const FString Name=TargetMesh->GetMaterials()[Section.MaterialIndex].MaterialSlotName.ToString().ToLower();
            if((Skirt && Name.Contains(TEXT("leather_skirt"))) || Section.ClothingData.AssetGuid==Cloth->GetAssetGuid())Materials.Add(Section.MaterialIndex);
        }
        for(const auto& Section:TargetMesh->GetResourceForRendering()->LODRenderData[0].RenderSections)
            if(Section.ClothingData.AssetGuid==Cloth->GetAssetGuid())Materials.Add(Section.MaterialIndex);
        if(Materials.Num()!=1){UE_LOG(LogTemp,Error,TEXT("ER_CLOTH_BIND_MATERIALS mesh=%s count=%d"),*TargetMesh->GetName(),Materials.Num());return false;}
        Cloth->UnbindFromSkeletalMesh(TargetMesh,INDEX_NONE,INDEX_NONE);
        // Each render LOD needs its own simulation LOD mapping. Only one is
        // active at runtime; retaining the small simulation mesh avoids a
        // collision discontinuity when rendering switches to a simpler mesh.
        while(Cloth->LodData.Num()<Model->LODModels.Num())
        {const FClothLODDataCommon Last=Cloth->LodData.Last();Cloth->LodData.Add(Last);}
        const auto* SourceCloth=SourceMesh && SourceMesh->GetMeshClothingAssets().Num()?Cast<UClothingAssetCommon>(SourceMesh->GetMeshClothingAssets()[0]):nullptr;
        for(int32 L=0;L<Cloth->LodData.Num();++L)
        {
            auto& LOD=Cloth->LodData[L];
            for(int32 M=0;M<LOD.PointWeightMaps.Num();++M)
            {
                auto& Map=LOD.PointWeightMaps[M];
                if(Map.CurrentTarget!=uint8(EWeightMapTargetCommon::MaxDistance))continue;
                if(Skirt)
                {
                    for(int32 I=0;I<Map.Values.Num();++I)
                    {const auto& V=LOD.PhysicalMeshData.Vertices[I];Map.Values[I]=V.Z>60 && V.Z<96?FMath::Clamp((96.f-V.Z)*1.6f,0.f,24.f):0.f;}
                }
                else
                {
                    if(!SourceCloth || SourceCloth->LodData.IsEmpty())return false;
                    const auto& SourceLOD=SourceCloth->LodData[FMath::Min(L,SourceCloth->LodData.Num()-1)];
                    if(!SourceLOD.PointWeightMaps.IsValidIndex(M))return false;
                    const auto& Original=SourceLOD.PointWeightMaps[M];if(Original.Values.Num()!=Map.Values.Num())return false;
                    for(int32 I=0;I<Map.Values.Num();++I)Map.Values[I]=FMath::Clamp(Original.Values[I],0.f,24.f);
                }
            }
            const auto* Distances=LOD.PointWeightMaps.FindByPredicate([](const FPointWeightMap& Map){return Map.CurrentTarget==uint8(EWeightMapTargetCommon::MaxDistance);});
            if(!Distances)return false;const TArray<float> MaxDistances=Distances->Values;
            for(auto Target:{EWeightMapTargetCommon::BackstopRadius,EWeightMapTargetCommon::BackstopDistance})
            {
                auto* Map=LOD.PointWeightMaps.FindByPredicate([Target](const FPointWeightMap& P){return P.CurrentTarget==uint8(Target);});
                if(!Map)Map=&LOD.PointWeightMaps.AddDefaulted_GetRef();
                Map->bEnabled=true;Map->CurrentTarget=uint8(Target);Map->Values.SetNum(MaxDistances.Num());
                for(int32 I=0;I<Map->Values.Num();++I)Map->Values[I]=Target==EWeightMapTargetCommon::BackstopRadius && MaxDistances[I]>0?3.f:0.f;
            }
        }
        for(auto& Pair:Cloth->ClothConfigs)if(auto* Config=Cast<UChaosClothConfig>(Pair.Value))
        {
            Config->CollisionThickness=1.2f;Config->FrictionCoefficient=.4f;
            Config->DampingCoefficient=.05f;Config->LocalDampingCoefficient=.3f;
            Config->AnimDriveStiffness.Low=.15f;Config->AnimDriveStiffness.High=.15f;
            Config->AnimDriveDamping.Low=.5f;Config->AnimDriveDamping.High=.5f;
            Config->bUseCCD=true;Config->bUseLegacyBackstop=false;
        }
        for(auto& Pair:Cloth->ClothConfigs)if(auto* Shared=Cast<UChaosClothSharedSimConfig>(Pair.Value))
        {Shared->IterationCount=4;Shared->MaxIterationCount=8;Shared->SubdivisionCount=2;}
        Cloth->ApplyParameterMasks(true);
        // UE5.8 also needs persistent source-section user data. Calling
        // BindToSkeletalMesh alone loses this binding on the next rebuild.
        for(int32 L=0;L<Model->LODModels.Num();++L)
        {
            int32 Bound=0;
            for(int32 S=0;S<Model->LODModels[L].Sections.Num();++S)
                {
                    const auto* Info=TargetMesh->GetLODInfo(L);
                    const int32 Material=Info && Info->LODMaterialMap.IsValidIndex(S) && Info->LODMaterialMap[S]>=0?Info->LODMaterialMap[S]:Model->LODModels[L].Sections[S].MaterialIndex;
                    if(!Materials.Contains(Material))continue;
                    FString Error;
                    if(!FSkeletalMeshClothingSystemUtilities::AssignClothingToSection(TargetMesh,Cloth,L,S,L,&Error))
                    {UE_LOG(LogTemp,Error,TEXT("ER_CLOTH_BIND_FAILED mesh=%s lod=%d section=%d reason=%s"),*TargetMesh->GetName(),L,S,*Error);return false;}++Bound;
                }
            if(Bound!=1){UE_LOG(LogTemp,Error,TEXT("ER_CLOTH_BIND_SECTIONS mesh=%s lod=%d count=%d sections=%d"),*TargetMesh->GetName(),L,Bound,Model->LODModels[L].Sections.Num());return false;}
        }
        Cloth->BuildLodTransitionData();Cloth->InvalidateAllCachedData();++Tuned;
    }
    TargetMesh->PostEditChange();TargetMesh->MarkPackageDirty();
    UE_LOG(LogTemp,Display,TEXT("ER_FIELD_CLOTHING mesh=%s skirt=%d assets=%d"),*TargetMesh->GetName(),Skirt,Tuned);
    return Tuned>0;
#else
    return false;
#endif
}


bool AERCharacter::ValidateFieldClothing(USkeletalMesh* TargetMesh,bool Skirt)
{
#if WITH_EDITOR
    if(!TargetMesh || TargetMesh->GetMeshClothingAssets().IsEmpty() || !TargetMesh->HasActiveClothingAssets())return false;
    TArray<UClothingAssetBase*> InUse;TargetMesh->GetClothingAssetsInUse(InUse);
    if(InUse.Num()!=TargetMesh->GetMeshClothingAssets().Num())return false;
    for(int32 L=0;L<TargetMesh->GetLODNum();++L)if(!TargetMesh->HasActiveClothingAssetsForLOD(L))return false;
    for(auto Asset:TargetMesh->GetMeshClothingAssets())
    {
        const auto* Cloth=Cast<UClothingAssetCommon>(Asset);if(!Cloth || !Cloth->PhysicsAsset || Cloth->PhysicsAsset==TargetMesh->GetPhysicsAsset() || Cloth->PhysicsAsset->GetOuter()!=TargetMesh)return false;
        bool Found=false;
        for(const auto& LOD:Cloth->LodData)for(const auto& Map:LOD.PointWeightMaps)
            if(Map.CurrentTarget==uint8(EWeightMapTargetCommon::MaxDistance))
            {
                bool Pinned=false,Free=false;Found=true;
                for(float V:Map.Values) { if(V<0 || V>24.001f)return false;Pinned|=V==0;Free|=V>0; }
                if(!Pinned || (Skirt && !Free))return false;
            }
        if(!Found)return false;
    }
    return true;
#else
    return false;
#endif
}
