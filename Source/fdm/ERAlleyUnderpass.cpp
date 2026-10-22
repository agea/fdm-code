#include "ERProceduralMap.h"

namespace
{
FVector Mate(FVector P){return {-P.X,-P.Y,P.Z};}
FBox Mate(FBox B){return {FVector(-B.Max.X,-B.Max.Y,B.Min.Z),FVector(-B.Min.X,-B.Min.Y,B.Max.Z)};}
bool Inside(const FBox& B,FVector2D P){return P.X>B.Min.X+.01 && P.X<B.Max.X-.01 && P.Y>B.Min.Y+.01 && P.Y<B.Max.Y-.01;}
bool Filled(const TArray<FBox>& A,const TArray<FBox>& Holes,FVector2D P)
{for(const auto& H:Holes)if(Inside(H,P))return false;for(const auto& B:A)if(Inside(B,P))return true;return false;}
// Exact rectangle union/difference. Floors, roof frames and wall corners never
// overlap, including at the bends of the S-shaped passage.
TArray<FBox> Partition(const TArray<FBox>& A,const TArray<FBox>& Holes,TArray<FBox>* Boundary=nullptr)
{
    TArray<float> X,Y;for(const auto* Set:{&A,&Holes})for(const auto& B:*Set)
    {X.AddUnique(B.Min.X);X.AddUnique(B.Max.X);Y.AddUnique(B.Min.Y);Y.AddUnique(B.Max.Y);}
    X.Sort();Y.Sort();TArray<FBox> Out;if(X.Num()<2 || Y.Num()<2 || X.Num()>200 || Y.Num()>200)return Out;
    const int32 NX=X.Num()-1,NY=Y.Num()-1;TArray<uint8> Grid;Grid.Init(0,NX*NY);
    for(int32 J=0;J<NY;++J)for(int32 I=0;I<NX;++I)
    {
        if(!Filled(A,Holes,{(X[I]+X[I+1])*.5,(Y[J]+Y[J+1])*.5}))continue;Grid[J*NX+I]=1;
        if(Boundary)for(int32 Axis=0;Axis<2;++Axis)for(int32 Sign:{-1,1})
        {
            FVector2D P((X[I]+X[I+1])*.5,(Y[J]+Y[J+1])*.5);
            P[Axis]=Axis ? (Sign<0 ? Y[J]-.1:Y[J+1]+.1):(Sign<0 ? X[I]-.1:X[I+1]+.1);
            if(Filled(A,{},P))continue;bool Portal=false;for(const auto& H:Holes)Portal|=Inside(H,P);if(Portal)continue;
            FVector Lo(X[I],Y[J],0),Hi(X[I+1],Y[J+1],0);
            if(Sign<0)Hi[Axis]=Lo[Axis]+20;else Lo[Axis]=Hi[Axis]-20;
            Boundary->Add(FBox(Lo,Hi));
        }
    }
    for(int32 J=0;J<NY;++J)for(int32 I=0;I<NX;++I)if(Grid[J*NX+I])
    {
        int32 End=I+1;while(End<NX && Grid[J*NX+End])++End;
        int32 Top=J+1;for(;Top<NY;++Top){bool Full=true;for(int32 K=I;K<End;++K)Full&=Grid[Top*NX+K]!=0;if(!Full)break;}
        for(int32 V=J;V<Top;++V)for(int32 U=I;U<End;++U)Grid[V*NX+U]=0;
        Out.Add(FBox(FVector(X[I],Y[J],0),FVector(X[End],Y[Top],0)));
    }
    return Out;
}
}
namespace ERProcedural
{
float AlleyRampHeight(const FERAlleyRamp& R,FVector2D P)
{
    const FVector2D A(R.Entry),D=FVector2D(R.Exit)-A;
    return FMath::Lerp(float(R.Entry.Z),float(R.Exit.Z),FMath::Clamp(FVector2D::DotProduct(P-A,D)/D.SizeSquared(),0.,1.));
}
bool PlanAlleyUnderpass(const UERMapTemplate& T,int32 Seed,FVector Base,TArray<FERMapTemplatePiece>& Pieces,FERProceduralLayout& L)
{
    if(!T.Town.LowCoverAndUnderpass)return true;
    const auto& V=T.Town;const FVector Half=L.ArenaSize*.5;FRandomStream R(int32(HashCombine(uint32(Seed),0x54554e4cu)));
    L.UnderpassAxis=1+((uint32(Seed)>>2)&1);
    auto Snap=[&](float Value,int32 Axis)
    {const float W=L.ArenaSize[Axis]+40;const int32 N=FMath::CeilToInt(W/75);return (FMath::RoundToInt((Value/W+.5f)*N)-N*.5f)*(W/N);};
    auto Rect=[&](FVector2D A,FVector2D B,float Width)
    {
        return FBox(FVector(Snap(FMath::Min(A.X,B.X)-Width*.5f,0),Snap(FMath::Min(A.Y,B.Y)-Width*.5f,1),0),
                    FVector(Snap(FMath::Max(A.X,B.X)+Width*.5f,0),Snap(FMath::Max(A.Y,B.Y)+Width*.5f,1),0));
    };
    FERAlleyRamp Ramp;const float Bend=R.RandRange(10,14)*75.f;
    TArray<FVector> Path;
    if(L.UnderpassAxis==1)
    {
        const float X=-Half.X+450,Start=Base.Y+400;
        Ramp.Bounds=FBox(FVector(Snap(X-225,0),Snap(Start,1),0),FVector(Snap(X+225,0),Snap(Start+1100,1),0));
        const float CX=Ramp.Bounds.GetCenter().X;Ramp.Entry={CX,Ramp.Bounds.Min.Y,180};Ramp.Exit={CX,Ramp.Bounds.Max.Y,-360};
        Path={Ramp.Exit,{CX,-1500,-360},{Bend,-1500,-360},{Bend,0,-360}};
        L.UnderpassApproach={Base+FVector(0,0,100),{CX,Base.Y,100},Ramp.Entry+FVector(0,0,100)};
    }
    else
    {
        const float Y=-900;
        Ramp.Bounds=FBox(FVector(Snap(-Half.X+100,0),Snap(Y-225,1),0),FVector(Snap(-Half.X+800,0),Snap(Y+225,1),0));
        const float CY=Ramp.Bounds.GetCenter().Y;Ramp.Entry={Ramp.Bounds.Min.X,CY,0};Ramp.Exit={Ramp.Bounds.Max.X,CY,-360};
        Path={Ramp.Exit,{-Bend,CY,-360},{-Bend,0,-360}};
    }
    L.UnderpassRamps.Add(Ramp);L.UnderpassRamps.Add({Mate(Ramp.Bounds),Mate(Ramp.Entry),Mate(Ramp.Exit)});
    const int32 North=Path.Num();for(int32 I=North-1;I>=0;--I)Path.Add(Mate(Path[I]));
    TArray<FBox> Footprints,Ramps{Ramp.Bounds,Mate(Ramp.Bounds)};
    for(int32 I=1;I<Path.Num();++I)
    {const auto B=Rect(FVector2D(Path[I-1]),FVector2D(Path[I]),450);Footprints.Add(B);Footprints.Add(Mate(B));}
    const FVector2D Breach=L.UnderpassAxis==1 ? FVector2D(Bend,-850):FVector2D(-1700,Ramp.Exit.Y);
    const FBox Hole=Rect(Breach,Breach,300);L.UnderpassBreaches={Hole,Mate(Hole)};
    // A continuous lower floor, enclosing walls and visible ceiling, with no
    // permanent collider behind the two cell-based road sections.
    auto Solid=[&](const FBox& B,float Bottom,float Top,bool Breakable=false)
    {
        auto& P=Pieces.AddDefaulted_GetRef();P.Center=FVector(B.GetCenter().X,B.GetCenter().Y,(Bottom+Top)*.5f);
        P.Size=FVector(B.GetSize().X,B.GetSize().Y,Top-Bottom);P.Material=Breakable ? EERBlockMaterial::Brick:EERBlockMaterial::Concrete;
        P.Destructible=Breakable;P.WalkableSurface=Breakable;P.Anchored=Breakable;
        if(!Breakable)P.FixedSurface=V.WallSurface;
    };
    TArray<FBox> Edges;const auto Floors=Partition(Footprints,Ramps,&Edges);if(Floors.IsEmpty())return false;
    for(const auto& B:Floors)Solid(B,-380,-360);
    for(const auto& B:Partition(Edges,{}))Solid(B,-360,-30);
    TArray<FBox> RoofHoles=Ramps;RoofHoles.Append(L.UnderpassBreaches);
    for(const auto& B:Partition(Footprints,RoofHoles))Solid(B,-30,-5);
    for(const auto& B:L.UnderpassBreaches)
    {
        if(!Filled(Footprints,Ramps,FVector2D(B.GetCenter())))return false;
        Solid(B,-10,0,true); // 10cm brick paving, using unchanged cell HP.
    }
    TArray<FVector> Route{Ramp.Entry+FVector(0,0,100)};Route.Add(Ramp.Exit+FVector(0,0,100));
    for(int32 I=1;I<Path.Num();++I)Route.Add(Path[I]+FVector(0,0,100));Route.Add(Mate(Ramp.Entry)+FVector(0,0,100));
    for(int32 I=1;I<Route.Num();++I)
    {
        const int32 N=FMath::Max(1,FMath::CeilToInt(FVector::Dist(Route[I-1],Route[I])/90));
        if(I==1)L.UnderpassRoute.Add(Route[0]);
        for(int32 J=1;J<=N;++J)L.UnderpassRoute.Add(FMath::Lerp(Route[I-1],Route[I],float(J)/N));
    }
    return true;
}
}
