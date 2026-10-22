#pragma once
#include "ERCellWorld.h"
#include "DynamicMesh/DynamicMesh3.h"

namespace ERMeshCells
{
    void Rasterize(const FERCellPanel& P,TFunctionRef<void(FIntVector)> Put);
    void Build(const FERCellWorld& World,int32 Panel,UE::Geometry::FDynamicMesh3& Mesh);
    void BoxSkin(FERCellPanelDesc& Panel);
}
