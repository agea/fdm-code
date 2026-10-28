#include "ERProceduralMap.h"

namespace ERProcedural
{
bool BuildAlleyLowCover(const UERMapTemplate& T,int32 Seed,TArray<FERMapTemplatePiece>& Pieces,FERProceduralLayout& L)
{
    if(!T.Town.LowCoverAndUnderpass)return true;
    const auto& V=T.Town;const FVector Half=L.ArenaSize*.5;
    FRandomStream R(int32(HashCombine(uint32(Seed),0x434f5645u)));
    auto Mate=[](FVector P){return FVector(-P.X,-P.Y,P.Z);};
    TArray<FBox> Occupied;
    for(const auto& P:Pieces)if(!P.VisualOnly && P.Center.Z+P.Size.Z*.5f>1)
        Occupied.Add(FBox(-P.Size*.5,P.Size*.5).TransformBy(FTransform(FRotator(0,P.RotationYaw,0),P.Center)).ExpandBy(FVector(70,70,0)));
    if(V.ProceduralHouses)for(const auto& H:L.Houses)
    {
        const auto S=H.Frame.GetScale3D();
        Occupied.Add(FBox(-FVector(S.X,S.Y,0)*.5,FVector(S.X,S.Y,0)*.5+FVector(0,0,S.Z)).TransformBy(FTransform(H.Frame.GetRotation(),H.Frame.GetLocation())).ExpandBy(FVector(70,70,0)));
    }
    for(const auto& Ramp:L.UnderpassRamps)Occupied.Add(FBox(FVector(Ramp.Bounds.Min.X-120,Ramp.Bounds.Min.Y-120,-1000),FVector(Ramp.Bounds.Max.X+120,Ramp.Bounds.Max.Y+120,2000)));
    for(const auto& H:L.UnderpassBreaches)Occupied.Add(FBox(FVector(H.Min.X-120,H.Min.Y-120,-1000),FVector(H.Max.X+120,H.Max.Y+120,2000)));
    for(const auto& G:L.TownGates)
    {const auto P=G.Frame.GetLocation();Occupied.Add(FBox(P-FVector(G.Gap*.5f+70,350,0),P+FVector(G.Gap*.5f+70,350,300)));}
    auto Clear=[&](FBox B)
    {
        for(const auto& Other:Occupied)if(B.Intersect(Other))return false;
        for(const auto& House:L.Houses)for(const auto& Entry:House.Entrances)for(int32 I=1;I<Entry.Num();++I)
            if(FMath::LineBoxIntersection(B.ExpandBy(FVector(60,60,100)),Entry[I-1],Entry[I],Entry[I]-Entry[I-1]))return false;
        for(const auto* Starts:{&L.AlphaSpawns,&L.OmegaSpawns})for(const auto& S:*Starts)if(B.ExpandBy(FVector(450,450,200)).IsInsideOrOn(S.GetLocation()))return false;
        for(const auto* Paths:{&L.RoomAccessRoutes,&L.RooftopRoutes,&L.StairRoutes})for(const auto& Path:*Paths)for(int32 I=1;I<Path.Num();++I)
        {
            const auto A=Path[I-1],C=Path[I];if(FMath::Min(A.Z,C.Z)<-50)continue;
            if(FMath::LineBoxIntersection(B.ExpandBy(FVector(45,45,100)),A,C,C-A))return false;
        }
        for(const auto& A:L.LowerRoute)if(B.ExpandBy(FVector(90,90,100)).IsInsideOrOn(A))return false;
        for(const auto& Tower:L.DistrictTowers)if(B.ExpandBy(FVector(180,180,1000)).IsInsideOrOn(Tower.GetLocation()))return false;
        return true;
    };
    for(int32 Region=0;Region<2;++Region)
    {
        TArray<FVector> Candidates;
        const float Core=L.DistrictWallCount==1 ? 1600.f:FMath::Max(1100.f,L.DistrictOpenHalfDepth-100);
        const float Grid=V.ProceduralHouses ? 100:200;
        for(float Y=-Half.Y+800;Y<=-250;Y+=Grid)for(float X=-Half.X+300;X<Half.X-250;X+=Grid)
        {
            if((FMath::Abs(Y)<Core)!=(Region==0))continue;
            Candidates.Add({X,Y,-1});
        }
        // Curb-side sites also fit narrow alleys: a 50cm-deep wall leaves a
        // standing capsule on the route and at least70cm beside house shells.
        for(const auto& Path:L.DistrictRoutes)for(int32 I=3;I<Path.Num();I+=5)
        {
            const auto P=Path[I];if(P.Y>=-250 || P.Y<-Half.Y+800 || ((FMath::Abs(P.Y)<Core)!=(Region==0)))continue;
            const auto D=(P-Path[I-1]).GetSafeNormal2D();const FVector Side(-D.Y,D.X,0);
            for(float Offset:{75.f,100.f,140.f})for(float Sign:{-1.f,1.f})Candidates.Add(FVector(P.X,P.Y,0)+Side*Offset*Sign+FVector(0,0,FMath::Fmod(D.Rotation().Yaw+360.f,180.f)));
        }
        for(int32 I=Candidates.Num()-1;I>0;--I)Candidates.Swap(I,R.RandRange(0,I));
        const int32 Target=Region ? 9:7;int32 Pairs=0;
        for(const auto Candidate:Candidates)
        {
            const FVector2D C(Candidate);const int32 Kind=Candidate.Z>=0 ? 1:R.RandRange(0,2);const float Yaw=Candidate.Z>=0 ? Candidate.Z:R.RandRange(0,1)*90.f;
            const FVector Size=Kind==0 ? FVector(240,180,110):Kind==1 ? FVector(300,50,90):FVector(200,75,65);
            const FVector Actual=FMath::Abs(FMath::Sin(FMath::DegreesToRadians(Yaw)))>.5 ? FVector(Size.Y,Size.X,Size.Z):Size;
            const float Ground=AlleyGroundHeight(L,C);float Low=Ground,High=Ground;
            for(float X:{-1.f,1.f})for(float Y:{-1.f,1.f})
            {const float Z=AlleyGroundHeight(L,C+FVector2D(Actual.X*X,Actual.Y*Y)*.5f);Low=FMath::Min(Low,Z);High=FMath::Max(High,Z);}
            if(High-Low>14 || Low<-.1f)continue;
            const FVector P(C,Low+Size.Z*.5f-2);const FBox Bounds(P-Actual*.5,P+Actual*.5);
            const FVector MP=Mate(P);const FBox Other(MP-Actual*.5,MP+Actual*.5);
            if(!Clear(Bounds) || !Clear(Other))continue;
            for(int32 Copy=0;Copy<2;++Copy)
            {
                auto& B=Pieces.AddDefaulted_GetRef();B.Center=Copy ? MP:P;B.Size=Size;B.RotationYaw=Yaw+Copy*180;
                B.Destructible=Kind==0;B.Anchored=Kind==0;B.Material=Kind==0 ? EERBlockMaterial::Wood:EERBlockMaterial::Stone;
                if(B.Destructible)B.CellSurface=V.WoodSurface;else B.FixedSurface=V.WallSurface;
                const auto Box=Copy ? Other:Bounds;L.LowCovers.Add(Box);Occupied.Add(Box.ExpandBy(FVector(100,100,0)));
            }
            if(++Pairs>=Target)break;
        }
        if(!Region)L.CentralLowCovers=Pairs*2;
        if(Pairs<(Region ? 3:4)){UE_LOG(LogTemp,Warning,TEXT("ER_ALLEY_COVER_REJECT seed=%d region=%d pairs=%d"),Seed,Region,Pairs);return false;}
    }
    return true;
}
}
