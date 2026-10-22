#include "ERDestructibleMesh.h"
#include "Engine/StaticMesh.h"
#include "Misc/Crc.h"
#if WITH_EDITOR
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#endif

bool UERDestructibleMesh::Bake(UStaticMesh* Mesh)
{
#if WITH_EDITOR
    if(!Mesh || !Mesh->GetPathName().StartsWith(TEXT("/Game/EclipseRecoil/Art/")))return false;
    const auto* D=Mesh->GetMeshDescription(0);if(!D || D->Triangles().Num()<1 || Mesh->GetStaticMaterials().Num()!=1)return false;
    FStaticMeshConstAttributes A(*D);const auto Bounds=Mesh->GetBoundingBox();const auto Size=Bounds.GetSize();
    if(Size.GetMin()<1 || A.GetVertexInstanceUVs().GetNumChannels()<1)return false;
    TArray<FERMeshCorner> Next;uint32 CRC=1;
    for(const auto T:D->Triangles().GetElementIDs())for(const auto I:D->GetTriangleVertexInstances(T))
    {
        auto& C=Next.AddDefaulted_GetRef();
        C.Position=(FVector(A.GetVertexPositions()[D->GetVertexInstanceVertex(I)])-Bounds.Min)/Size;
        C.Normal=FVector(A.GetVertexInstanceNormals()[I]);C.UV=FVector2D(A.GetVertexInstanceUVs().Get(I,0));
        for(double V:{C.Position.X,C.Position.Y,C.Position.Z,C.Normal.X,C.Normal.Y,C.Normal.Z,C.UV.X,C.UV.Y})
            CRC=HashCombineFast(CRC,GetTypeHash(FMath::RoundToInt64(V*1000000)));
    }
    Modify();Source=Mesh;Corners=MoveTemp(Next);GeometryCRC=CRC;MarkPackageDirty();return true;
#else
    return false;
#endif
}
