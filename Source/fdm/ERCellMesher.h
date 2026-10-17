#pragma once
#include "CoreMinimal.h"
#include "ERCellWorld.h"

// Aggregated geometry for one chunk, in panel-local centimetres. Faces exist
// only between an alive cell and empty space (also across chunk borders);
// fully internal faces are never generated.
namespace ERCellMesh
{
    struct FQuad { FVector3f Corner[4]; FVector3f Normal; };
    void BuildFaces(const FERCellWorld& World, int32 Chunk, TArray<FQuad>& Quads);
    // Greedy maximal boxes over the chunk's alive cells, deterministic order.
    void BuildBoxes(const FERCellWorld& World, int32 Chunk, TArray<FBox>& Boxes);
}
