#pragma once
#include "CoreMinimal.h"

struct FERLayoutFixedPiece;
namespace UE::Geometry { class FDynamicMesh3; }
namespace ERIndustrialFence
{
    // Part of the layout fingerprint: old clients must not accept other metal.
    constexpr uint32 GeometryVersion=3;
    constexpr float PlinthHeightCm=100,ModuleHeightCm=127;
    constexpr float FrameCm=4,PitchCm=16,WireCm=.4f;
    // One synchronous triangle mesh supplies BOTH the visible metal and queries.
    void Append(const FERLayoutFixedPiece& Panel,UE::Geometry::FDynamicMesh3& Mesh);
    void ModuleTransforms(const FERLayoutFixedPiece& Panel,const FBox& Bounds,TArray<FTransform>& Out);
}
