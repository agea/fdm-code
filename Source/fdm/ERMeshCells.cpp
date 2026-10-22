#include "ERMeshCells.h"
#include "ERCellMesher.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"

namespace
{
using FCellPoly=TArray<FERMeshCorner,TInlineAllocator<12>>;
FERMeshCorner Interpolate(const FERMeshCorner& A,const FERMeshCorner& B,double T)
{return {FMath::Lerp(A.Position,B.Position,T),FMath::Lerp(A.Normal,B.Normal,T),FMath::Lerp(A.UV,B.UV,T)};}
FCellPoly Clip(const FCellPoly& In,int32 Axis,double At,bool Less)
{
    FCellPoly Out;if(In.IsEmpty())return Out;
    auto Distance=[&](const FERMeshCorner& V){return (V.Position[Axis]-At)*(Less ? 1.:-1.);};
    auto A=In.Last();double DA=Distance(A);
    for(const auto& B:In)
    {
        const double DB=Distance(B);
        if((DA<=0)!=(DB<=0))Out.Add(Interpolate(A,B,DA/(DA-DB)));
        if(DB<=0)Out.Add(B);A=B;DA=DB;
    }
    return Out;
}
void Append(const FCellPoly& P,UE::Geometry::FDynamicMesh3& Mesh,int32 Material=0)
{
    if(P.Num()<3)return;
    auto* N=Mesh.Attributes()->PrimaryNormals();auto* UV=Mesh.Attributes()->PrimaryUV();
    for(int32 I=1;I+1<P.Num();++I)
    {
        const FERMeshCorner C[3]={P[0],P[I],P[I+1]};
        if(FVector::CrossProduct(C[1].Position-C[0].Position,C[2].Position-C[0].Position).SizeSquared()<1.e-10)continue;
        int32 V[3],Ns[3],Us[3];
        for(int32 J=0;J<3;++J){V[J]=Mesh.AppendVertex(C[J].Position);Ns[J]=N->AppendElement(FVector3f(C[J].Normal.GetSafeNormal()));Us[J]=UV->AppendElement(FVector2f(C[J].UV));}
        const auto T=Mesh.AppendTriangle(V[0],V[1],V[2]);
        if(T>=0){N->SetTriangle(T,{Ns[0],Ns[1],Ns[2]});UV->SetTriangle(T,{Us[0],Us[1],Us[2]});Mesh.Attributes()->GetMaterialID()->SetValue(T,Material);}
    }
}
bool Bit(const TArray<uint64>& B,int32 I){return (B[I>>6]>>(I&63))&1;}
// Merge removed cells only. Source voids never create damage boxes or fracture
// faces. A completely lost chunk is one cut, including its empty room volume.
void Cuts(const FERCellWorld& W,int32 CI,TArray<FBox>& Out)
{
    const auto& C=W.GetChunks()[CI];const auto& P=W.GetPanels()[C.Panel];
    if(C.Revision==0 || C.Alive==C.InitialAlive)return;
    if(!C.AliveCount){Out.Add(FBox(FVector(C.X0,0,C.Z0)*P.Cell,FVector(C.X0+C.SX,P.Dims.Y,C.Z0+C.SZ)*P.Cell));return;}
    TBitArray<> Used(false,C.SX*C.SZ*P.Dims.Y);
    auto Free=[&](int32 X,int32 Y,int32 Z){const int32 I=FERCellWorld::CellIndex(C,X,Y,Z);return !Used[I] && Bit(C.InitialAlive,I) && !Bit(C.Alive,I);};
    for(int32 Y=0;Y<P.Dims.Y;++Y)for(int32 Z=0;Z<C.SZ;++Z)for(int32 X=0;X<C.SX;++X)if(Free(X,Y,Z))
    {
        int32 X1=X+1,Z1=Z+1,Y1=Y+1;while(X1<C.SX && Free(X1,Y,Z))++X1;
        for(bool Grow=true;Grow && Z1<C.SZ;){for(int32 K=X;K<X1 && Grow;++K)Grow=Free(K,Y,Z1);if(Grow)++Z1;}
        for(bool Grow=true;Grow && Y1<P.Dims.Y;){for(int32 KZ=Z;KZ<Z1 && Grow;++KZ)for(int32 KX=X;KX<X1 && Grow;++KX)Grow=Free(KX,Y1,KZ);if(Grow)++Y1;}
        for(int32 KY=Y;KY<Y1;++KY)for(int32 KZ=Z;KZ<Z1;++KZ)for(int32 KX=X;KX<X1;++KX)Used[FERCellWorld::CellIndex(C,KX,KY,KZ)]=true;
        Out.Add(FBox(FVector(C.X0+X,Y,C.Z0+Z)*P.Cell,FVector(C.X0+X1,Y1,C.Z0+Z1)*P.Cell));
    }
}
}

void ERMeshCells::Rasterize(const FERCellPanel& P,TFunctionRef<void(FIntVector)> Put)
{
    for(int32 I=0;I<P.Skin.Num();I+=3)
    {
        const FCellPoly Triangle{P.Skin[I],P.Skin[I+1],P.Skin[I+2]};
        const auto Normal=FVector::CrossProduct(Triangle[1].Position-Triangle[0].Position,Triangle[2].Position-Triangle[0].Position).GetAbs();
        const int32 Axis=Normal.X>Normal.Y && Normal.X>Normal.Z ? 0:Normal.Y>Normal.Z ? 1:2,U=(Axis+1)%3,V=(Axis+2)%3;
        FBox Bounds(ForceInit);for(const auto& C:Triangle)Bounds+=C.Position;
        const int32 U0=FMath::Clamp(FMath::FloorToInt(Bounds.Min[U]/P.Cell[U]-.0001),0,P.Dims[U]-1),U1=FMath::Clamp(FMath::FloorToInt(Bounds.Max[U]/P.Cell[U]+.0001),0,P.Dims[U]-1);
        const int32 V0=FMath::Clamp(FMath::FloorToInt(Bounds.Min[V]/P.Cell[V]-.0001),0,P.Dims[V]-1),V1=FMath::Clamp(FMath::FloorToInt(Bounds.Max[V]/P.Cell[V]+.0001),0,P.Dims[V]-1);
        for(int32 B=V0;B<=V1;++B)for(int32 A=U0;A<=U1;++A)
        {
            auto Poly=Clip(Clip(Clip(Clip(Triangle,U,A*P.Cell[U]-.001,false),U,(A+1)*P.Cell[U]+.001,true),V,B*P.Cell[V]-.001,false),V,(B+1)*P.Cell[V]+.001,true);
            if(Poly.Num()<3)continue;
            double Lo=DBL_MAX,Hi=-DBL_MAX;for(const auto& C:Poly){Lo=FMath::Min(Lo,C.Position[Axis]);Hi=FMath::Max(Hi,C.Position[Axis]);}
            const int32 D0=FMath::Clamp(FMath::FloorToInt((Lo-.001)/P.Cell[Axis]),0,P.Dims[Axis]-1),D1=FMath::Clamp(FMath::FloorToInt((Hi+.001)/P.Cell[Axis]),0,P.Dims[Axis]-1);
            for(int32 D=D0;D<=D1;++D){FIntVector C;C[Axis]=D;C[U]=A;C[V]=B;Put(C);}
        }
    }
}

void ERMeshCells::Build(const FERCellWorld& World,int32 SkinPanelIndex,UE::Geometry::FDynamicMesh3& Mesh)
{
    const auto& P=World.GetPanels()[SkinPanelIndex];Mesh.EnableAttributes();Mesh.Attributes()->EnableMaterialID();
    TArray<FBox> Removed;TArray<int32> Changed;
    for(int32 C=P.FirstChunk;C<P.FirstChunk+P.ChunksX*P.ChunksZ;++C)if(World.GetChunks()[C].Revision){Cuts(World,C,Removed);Changed.Add(C);}
    for(int32 I=0;I<P.Skin.Num();I+=3)
    {
        TArray<FCellPoly> Polys;Polys.Add({P.Skin[I],P.Skin[I+1],P.Skin[I+2]});
        for(const auto& Cut:Removed)
        {
            const auto Box=Cut.ExpandBy(.001);
            TArray<FCellPoly> Next;
            for(const auto& Poly:Polys)
            {
                FBox Bounds(ForceInit);for(const auto& V:Poly)Bounds+=V.Position;
                if(!Bounds.Intersect(Box)){Next.Add(Poly);continue;}
                auto Inside=Poly;
                for(int32 Axis=0;Axis<3 && Inside.Num()>=3;++Axis)
                {
                    auto Outside=Clip(Inside,Axis,Box.Min[Axis],true);if(Outside.Num()>=3)Next.Add(MoveTemp(Outside));
                    Inside=Clip(Inside,Axis,Box.Min[Axis],false);
                    Outside=Clip(Inside,Axis,Box.Max[Axis],false);if(Outside.Num()>=3)Next.Add(MoveTemp(Outside));
                    Inside=Clip(Inside,Axis,Box.Max[Axis],true);
                }
            }
            Polys=MoveTemp(Next);if(Polys.IsEmpty())break;
        }
        for(const auto& Poly:Polys)Append(Poly,Mesh);
    }
    TSet<int32> Edges(Changed);
    for(int32 C:Changed)
    {
        const int32 Local=C-P.FirstChunk,X=Local%P.ChunksX,Z=Local/P.ChunksX;
        if(X)Edges.Add(C-1);if(X+1<P.ChunksX)Edges.Add(C+1);if(Z)Edges.Add(C-P.ChunksX);if(Z+1<P.ChunksZ)Edges.Add(C+P.ChunksX);
    }
    auto EdgeOrder=Edges.Array();EdgeOrder.Sort();TArray<ERCellMesh::FQuad> Faces;
    for(int32 C:EdgeOrder)
    {
        ERCellMesh::BuildFaces(World,C,Faces,true);
        for(const auto& Q:Faces)
        {
            const int32 Axis=FMath::Abs(Q.Normal.X)>.5 ? 0:FMath::Abs(Q.Normal.Y)>.5 ? 1:2,U=(Axis+1)%3,V=(Axis+2)%3;
            const float Mirror=Q.Normal[Axis]<0 ? -1.f:1.f;
            FCellPoly Poly;for(int32 Corner=0;Corner<4;++Corner)Poly.Add({FVector(Q.Corner[Corner]),FVector(Q.Normal),FVector2D(Mirror*Q.Corner[Corner][U],Q.Corner[Corner][V])/100});
            Append(Poly,Mesh,1);
        }
    }
}

void ERMeshCells::BoxSkin(FERCellPanelDesc& P)
{
    for(int32 Axis=0;Axis<3;++Axis)for(int32 Sign:{-1,1})
    {
        const int32 U=(Axis+1)%3,V=(Axis+2)%3;FERMeshCorner Q[4];
        for(int32 I=0;I<4;++I){Q[I].Position[Axis]=Sign>0 ? P.Size[Axis]:0;Q[I].Position[U]=(I==1 || I==2) ? P.Size[U]:0;Q[I].Position[V]=I>=2 ? P.Size[V]:0;Q[I].Normal=FVector::ZeroVector;Q[I].Normal[Axis]=Sign;Q[I].UV={(Sign<0 ? -1.:1.)*Q[I].Position[U]/100,Q[I].Position[V]/100};}
        if(Sign>0)Swap(Q[1],Q[3]);
        for(int32 I:{0,1,2,0,2,3})P.Skin.Add(Q[I]);
    }
    P.SkinCRC=1;P.SkinName=TEXT("AlleyMasonry");P.bSolidSkin=true;
}
