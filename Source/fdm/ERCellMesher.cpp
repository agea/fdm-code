#include "ERCellMesher.h"

void ERCellMesh::BuildFaces(const FERCellWorld& World, int32 ChunkIndex, TArray<FQuad>& Quads,bool FracturesOnly)
{
    Quads.Reset();
    const auto& C = World.GetChunks()[ChunkIndex];
    const auto& P = World.GetPanels()[C.Panel];
    if (C.AliveCount == 0) return;
    const FIntVector Ext(C.SX, P.Dims.Y, C.SZ);
    auto Alive = [&](const FIntVector& L) { return World.IsAlive(C.Panel, C.X0 + L.X, L.Y, C.Z0 + L.Z); };
    TArray<uint8> Mask;
    for (int32 Axis = 0; Axis < 3; ++Axis)
    {
        const int32 U = (Axis + 1) % 3, V = (Axis + 2) % 3;
        for (int32 Sign : {-1, 1})
            for (int32 D = 0; D < Ext[Axis]; ++D)
            {
                Mask.SetNumZeroed(Ext[U] * Ext[V]);
                bool bAny = false;
                for (int32 B = 0; B < Ext[V]; ++B)
                    for (int32 A = 0; A < Ext[U]; ++A)
                    {
                        FIntVector L; L[Axis] = D; L[U] = A; L[V] = B;
                        FIntVector N = L; N[Axis] += Sign;
                        const bool bFace = Alive(L) && !Alive(N) && (!FracturesOnly || World.WasInitiallyAlive({C.Panel,C.X0+N.X,N.Y,C.Z0+N.Z}));
                        Mask[A + Ext[U] * B] = bFace; bAny |= bFace;
                    }
                if (!bAny) continue;
                for (int32 B = 0; B < Ext[V]; ++B)
                    for (int32 A = 0; A < Ext[U]; ++A)
                    {
                        if (!Mask[A + Ext[U] * B]) continue;
                        int32 A1 = A + 1;
                        while (A1 < Ext[U] && Mask[A1 + Ext[U] * B]) ++A1;
                        int32 B1 = B + 1;
                        for (; B1 < Ext[V]; ++B1)
                        {
                            bool bRow = true;
                            for (int32 K = A; K < A1 && bRow; ++K) bRow = Mask[K + Ext[U] * B1] != 0;
                            if (!bRow) break;
                        }
                        for (int32 Y = B; Y < B1; ++Y) for (int32 X = A; X < A1; ++X) Mask[X + Ext[U] * Y] = 0;
                        auto Corner = [&](int32 CU, int32 CV)
                        {
                            FVector Cell; Cell[Axis] = D + (Sign > 0 ? 1 : 0); Cell[U] = CU; Cell[V] = CV;
                            Cell.X += C.X0; Cell.Z += C.Z0;
                            return FVector3f(Cell * P.Cell);
                        };
                        FQuad& Q = Quads.AddDefaulted_GetRef();
                        Q.Corner[0] = Corner(A, B); Q.Corner[1] = Corner(A1, B); Q.Corner[2] = Corner(A1, B1); Q.Corner[3] = Corner(A, B1);
                        Q.Normal = FVector3f::ZeroVector; Q.Normal[Axis] = Sign;
                        // Unreal front faces wind clockwise seen from the normal side:
                        // (c1-c0) x (c2-c0) points away from it, as in FRectangleMeshGenerator.
                        if (FVector3f::DotProduct((Q.Corner[1] - Q.Corner[0]) ^ (Q.Corner[2] - Q.Corner[0]), Q.Normal) > 0) Swap(Q.Corner[1], Q.Corner[3]);
                    }
            }
    }
}

void ERCellMesh::BuildBoxes(const FERCellWorld& World, int32 ChunkIndex, TArray<FBox>& Boxes)
{
    Boxes.Reset();
    const auto& C = World.GetChunks()[ChunkIndex];
    const auto& P = World.GetPanels()[C.Panel];
    if (C.AliveCount == 0) return;
    const int32 NY = P.Dims.Y;
    TBitArray<> Used(false, C.SX * C.SZ * NY);
    auto Free = [&](int32 X, int32 Y, int32 Z)
    {
        const int32 I = FERCellWorld::CellIndex(C, X, Y, Z);
        return !Used[I] && ((C.Alive[I >> 6] >> (I & 63)) & 1);
    };
    for (int32 Y = 0; Y < NY; ++Y)
        for (int32 Z = 0; Z < C.SZ; ++Z)
            for (int32 X = 0; X < C.SX; ++X)
            {
                if (!Free(X, Y, Z)) continue;
                int32 X1 = X + 1, Z1 = Z + 1, Y1 = Y + 1;
                while (X1 < C.SX && Free(X1, Y, Z)) ++X1;
                for (bool bGrow = true; bGrow && Z1 < C.SZ; )
                {
                    for (int32 K = X; K < X1 && bGrow; ++K) bGrow = Free(K, Y, Z1);
                    if (bGrow) ++Z1;
                }
                for (bool bGrow = true; bGrow && Y1 < NY; )
                {
                    for (int32 KZ = Z; KZ < Z1 && bGrow; ++KZ) for (int32 KX = X; KX < X1 && bGrow; ++KX) bGrow = Free(KX, Y1, KZ);
                    if (bGrow) ++Y1;
                }
                for (int32 KY = Y; KY < Y1; ++KY) for (int32 KZ = Z; KZ < Z1; ++KZ) for (int32 KX = X; KX < X1; ++KX) Used[FERCellWorld::CellIndex(C, KX, KY, KZ)] = true;
                Boxes.Add(FBox(FVector(C.X0 + X, Y, C.Z0 + Z) * P.Cell, FVector(C.X0 + X1, Y1, C.Z0 + Z1) * P.Cell));
            }
}
