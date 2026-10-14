#include "ERCorpse.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#if WITH_EDITOR
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#endif

FERAppearanceAttachment AERCorpse::MakeFieldAttachment(UStaticMesh* Gear, UMaterialInterface* Material, USkeletalMesh* Character, FName Bone, FVector Offset, FVector Scale)
{
    FERAppearanceAttachment Part;
    if (!Character) return Part;
    const auto& Ref = Character->GetRefSkeleton();
    int32 Index = Ref.FindBoneIndex(Bone);
    if (Index == INDEX_NONE) return Part;
    FTransform Reference = Ref.GetRefBonePose()[Index];
    while ((Index = Ref.GetParentIndex(Index)) != INDEX_NONE) Reference *= Ref.GetRefBonePose()[Index];
    const FTransform Desired(FQuat::Identity, Reference.GetLocation() + Offset, Scale);
    Part.Mesh = Gear; Part.Material = Material; Part.Bone = Bone;
    Part.Transform = Desired.GetRelativeTransform(Reference);
    return Part;
}

bool AERCorpse::BuildFieldGear(UStaticMesh* Mesh, bool bHelmet)
{
#if WITH_EDITOR
    if (!Mesh || !Mesh->GetPathName().StartsWith(TEXT("/Game/EclipseRecoil/Characters/Teams/"))) return false;
    FMeshDescription Description;
    FStaticMeshAttributes Attributes(Description); Attributes.Register();
    Attributes.GetVertexInstanceUVs().SetNumChannels(1);
    const auto Group = Description.CreatePolygonGroup();
    Attributes.GetPolygonGroupMaterialSlotNames()[Group] = TEXT("Fabric");
    auto Triangle = [&](FVector A, FVector B, FVector C)
    {
        TArray<FVertexInstanceID> Instances;
        const FVector Normal = FVector::CrossProduct(B-A, C-A).GetSafeNormal();
        const FVector Tangent = (B-A).GetSafeNormal();
        int32 UVIndex = 0;
        for (const FVector Point : {A,B,C})
        {
            auto Vertex = Description.CreateVertex();
            Attributes.GetVertexPositions()[Vertex] = FVector3f(Point);
            auto Instance = Description.CreateVertexInstance(Vertex);
            Attributes.GetVertexInstanceNormals()[Instance] = FVector3f(Normal);
            Attributes.GetVertexInstanceTangents()[Instance] = FVector3f(Tangent);
            Attributes.GetVertexInstanceBinormalSigns()[Instance] = 1;
            Attributes.GetVertexInstanceUVs().Set(Instance, 0, FVector2f(UVIndex == 1 ? 1 : 0, UVIndex == 2 ? 1 : 0));
            Attributes.GetVertexInstanceColors()[Instance] = FVector4f(1,1,1,1);
            Instances.Add(Instance); ++UVIndex;
        }
        Description.CreatePolygon(Group, Instances);
    };
    // Rounded textile pack and pockets, or a simple helmet shell with a rolled rim.
    auto Surface = [&](FVector Center, FVector Radius, bool Hemisphere)
    {
        constexpr int32 Sides = 32, Rings = 12;
        const float MinAngle = Hemisphere ? -.12f : -PI*.5f;
        auto Point = [&](int32 R, int32 S)
        {
            const float Elevation = FMath::Lerp(MinAngle, PI*.5f-.001f, float(R)/Rings);
            const float Angle = 2*PI*float(S)/Sides;
            // Superellipsoid makes bags box-shaped with softened corners.
            auto Shape = [&](float X) { return Hemisphere ? X : FMath::Sign(X)*FMath::Pow(FMath::Abs(X), .28f); };
            return Center + Radius*FVector(Shape(FMath::Cos(Elevation))*Shape(FMath::Cos(Angle)),
                Shape(FMath::Cos(Elevation))*Shape(FMath::Sin(Angle)), Shape(FMath::Sin(Elevation)));
        };
        for (int32 R=0; R<Rings; ++R) for (int32 S=0; S<Sides; ++S)
        {
            FVector A=Point(R,S), B=Point(R,S+1), C=Point(R+1,S+1), D=Point(R+1,S);
            Triangle(A,B,C); Triangle(A,C,D);
        }
        if (Hemisphere)
        {
            for (int32 S=0; S<Sides; ++S)
            {
                FVector A=Point(0,S), B=Point(0,S+1);
                FVector C=Center+(B-Center)*FVector(.94f,.94f,1), D=Center+(A-Center)*FVector(.94f,.94f,1);
                Triangle(A,D,C); Triangle(A,C,B);
            }
        }
    };
    if (bHelmet) Surface(FVector::ZeroVector, FVector(10.3f,12.3f,12), true);
    else
    {
        Surface(FVector::ZeroVector, FVector(14,7,20), false);
        Surface(FVector(0,-8,-5), FVector(10,3,9), false);
        for (int32 Side : {-1,1}) Surface(FVector(Side*14,0,-7), FVector(3,6,10), false);
        for (int32 Side : {-1,1}) Surface(FVector(Side*9,6,0), FVector(2,1,18), false);
    }
    Mesh->Modify();
    Mesh->GetStaticMaterials().Reset();
    Mesh->GetStaticMaterials().Add(FStaticMaterial(nullptr, TEXT("Fabric")));
    UStaticMesh::FBuildMeshDescriptionsParams Params;
    Params.bBuildSimpleCollision = false;
    Params.bFastBuild = false;
    if (!Mesh->BuildFromMeshDescriptions({&Description}, Params)) return false;
    Mesh->MarkPackageDirty();
    return true;
#else
    return false;
#endif
}
