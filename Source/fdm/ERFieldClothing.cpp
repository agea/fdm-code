#include "ERCharacter.h"
#include "Engine/SkeletalMesh.h"
#include "PhysicsEngine/PhysicsAsset.h"
#if WITH_EDITOR
#include "ClothingAsset.h"
#include "ClothingAssetFactory.h"
#include "Rendering/SkeletalMeshModel.h"
#include "ChaosCloth/ChaosClothConfig.h"
#endif

bool AERCharacter::ConfigureFieldClothing(USkeletalMesh* TargetMesh,bool Skirt)
{
#if WITH_EDITOR
    if(!TargetMesh || !TargetMesh->GetPathName().StartsWith(TEXT("/Game/EclipseRecoil/Characters/Teams/")))return false;
    TargetMesh->Modify();
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
            if(!Name.Contains(TEXT("skirt")) || S.HasClothingData())continue;
            FSkeletalMeshClothBuildParams Params;Params.AssetName=FString::Printf(TEXT("ER_FieldSkirt_%d"),Section);
            Params.LodIndex=0;Params.SourceSection=Section;Params.bRemoveFromMesh=false;Params.PhysicsAsset=TargetMesh->GetPhysicsAsset();
            auto* Cloth=Cast<UClothingAssetCommon>(Factory->CreateFromSkeletalMesh(TargetMesh,Params));if(!Cloth)return false;
            TargetMesh->AddClothingAsset(Cloth);
            auto& LOD=Cloth->LodData[0];auto& Map=LOD.PointWeightMaps[0];
            Map.bEnabled=true;Map.CurrentTarget=uint8(EWeightMapTargetCommon::MaxDistance);
            int32 Free=0;
            for(int32 I=0;I<LOD.PhysicalMeshData.Vertices.Num();++I)
            {
                const auto& V=LOD.PhysicalMeshData.Vertices[I];
                Map.Values[I]=V.Z>60 && V.Z<96 ? FMath::Clamp((96.f-V.Z)*.8f,0.f,22.f):0.f;
                Free+=Map.Values[I]>0;
            }
            UE_LOG(LogTemp,Display,TEXT("ER_FIELD_CLOTHING_SECTION mesh=%s section=%s vertices=%d free=%d"),*TargetMesh->GetName(),*Name,LOD.PhysicalMeshData.Vertices.Num(),Free);
            if(!Free) {TargetMesh->GetMeshClothingAssets().Remove(Cloth);continue;}
            Cloth->ApplyParameterMasks(true);if(!Cloth->BindToSkeletalMesh(TargetMesh,0,Section,0))return false;
        }
    }
    int32 Tuned=0;
    for(auto Asset:TargetMesh->GetMeshClothingAssets())if(auto* Cloth=Cast<UClothingAssetCommon>(Asset))
    {
        Cloth->PhysicsAsset=TargetMesh->GetPhysicsAsset();
        if(!Skirt)for(auto& LOD:Cloth->LodData)for(auto& Map:LOD.PointWeightMaps)
            if(Map.CurrentTarget==uint8(EWeightMapTargetCommon::MaxDistance))
                for(float& Value:Map.Values)Value=FMath::Clamp(Value,0.f,6.f);
        for(auto& Pair:Cloth->ClothConfigs)if(auto* Config=Cast<UChaosClothConfig>(Pair.Value))
        {
            Config->CollisionThickness=.6f;Config->FrictionCoefficient=.3f;
            Config->DampingCoefficient=.6f;Config->LocalDampingCoefficient=.2f;
            Config->AnimDriveStiffness.Low=Skirt?.15f:.7f;Config->AnimDriveStiffness.High=Skirt?.15f:.7f;
            Config->AnimDriveDamping.Low=.5f;Config->AnimDriveDamping.High=.5f;
            Config->bUseCCD=true;
        }
        Cloth->ApplyParameterMasks(true);Cloth->InvalidateAllCachedData();++Tuned;
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
    if(!TargetMesh || TargetMesh->GetMeshClothingAssets().IsEmpty())return false;
    for(auto Asset:TargetMesh->GetMeshClothingAssets())
    {
        const auto* Cloth=Cast<UClothingAssetCommon>(Asset);if(!Cloth || Cloth->PhysicsAsset!=TargetMesh->GetPhysicsAsset())return false;
        bool Found=false;
        for(const auto& LOD:Cloth->LodData)for(const auto& Map:LOD.PointWeightMaps)
            if(Map.CurrentTarget==uint8(EWeightMapTargetCommon::MaxDistance))
            {
                bool Pinned=false,Free=false;Found=true;
                for(float V:Map.Values) { if(V<0 || V>(Skirt?22.001f:6.001f))return false;Pinned|=V==0;Free|=V>0; }
                if(!Pinned || (Skirt && !Free))return false;
            }
        if(!Found)return false;
    }
    return true;
#else
    return false;
#endif
}
