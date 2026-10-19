#include "ERIndustrialFence.h"
#include "ERProceduralMap.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"

void ERIndustrialFence::Append(const FERLayoutFixedPiece& Panel,UE::Geometry::FDynamicMesh3& Mesh)
{
    using UE::Geometry::FIndex3i;
    const FVector Size=Panel.Transform.GetScale3D()*100;
    const bool AlongX=Size.X>=Size.Y;
    const float W=AlongX ? Size.X:Size.Y,H=Size.Z;
    const FQuat Rotation=Panel.Transform.GetRotation()*FRotator(0,AlongX ? 0:90,0).Quaternion();
    const FTransform Frame(Rotation,Panel.Transform.GetLocation());
    auto* Normals=Mesh.Attributes()->PrimaryNormals();auto* UVs=Mesh.Attributes()->PrimaryUV();
    auto Box=[&](FVector Center,FVector Extent,FQuat R=FQuat::Identity)
    {
        for(int32 Axis=0;Axis<3;++Axis)for(float Sign:{-1.f,1.f})
        {
            const int32 U=(Axis+1)%3,V=(Axis+2)%3;
            int32 Vert[4],N[4],UV[4];
            FVector Normal=FVector::ZeroVector;Normal[Axis]=Sign;
            Normal=Rotation.RotateVector(R.RotateVector(Normal));
            const FVector2f Corners[]={{-1,-1},{-1,1},{1,1},{1,-1}};
            for(int32 I=0;I<4;++I)
            {
                FVector P=FVector::ZeroVector;P[Axis]=Extent[Axis]*.5*Sign;
                P[U]=Corners[I].X*Extent[U]*.5;P[V]=Corners[I].Y*Extent[V]*.5*Sign;
                Vert[I]=Mesh.AppendVertex(Frame.TransformPosition(Center+R.RotateVector(P)));
                N[I]=Normals->AppendElement(FVector3f(Normal));
                UV[I]=UVs->AppendElement(FVector2f(P[U],P[V])/100);
            }
            for(const FIndex3i Id:{FIndex3i(0,1,2),FIndex3i(0,2,3)})
            {
                const int32 Tri=Mesh.AppendTriangle(Vert[Id.A],Vert[Id.B],Vert[Id.C]);
                Normals->SetTriangle(Tri,FIndex3i(N[Id.A],N[Id.B],N[Id.C]));
                UVs->SetTriangle(Tri,FIndex3i(UV[Id.A],UV[Id.B],UV[Id.C]));
            }
        }
    };
    for(float Sign:{-1.f,1.f})
    {
        Box({Sign*(W-FrameCm)*.5,0,0},{FrameCm,6,H});
        Box({0,0,Sign*(H-FrameCm)*.5},{W-2*FrameCm,6,FrameCm});
    }
    // Clip two diagonal wire families to the frame. Lines and collision have
    // identical physical thickness, so no hit probability or texture guess is used.
    const float X0=-W*.5+FrameCm,X1=W*.5-FrameCm,Z0=-H*.5+FrameCm,Z1=H*.5-FrameCm;
    for(float Slope:{-1.f,1.f})
    {
        const float C0=Z0-X1,C1=Z1-X0;
        for(int32 I=FMath::CeilToInt(C0/PitchCm);I<=FMath::FloorToInt(C1/PitchCm);++I)
        {
            const float C=I*PitchCm;
            const float A=FMath::Max(X0,Slope>0 ? Z0-C:C-Z1),B=FMath::Min(X1,Slope>0 ? Z1-C:C-Z0);
            if(B-A<WireCm)continue;
            const FVector Start(A,0,Slope*A+C),End(B,0,Slope*B+C),Delta=End-Start;
            Box((Start+End)*.5,{Delta.Size(),WireCm,WireCm},Delta.Rotation().Quaternion());
        }
    }
}
