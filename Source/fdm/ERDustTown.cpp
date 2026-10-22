#include "ERMapTemplate.h"
#include "ERProceduralMap.h"
#include "Engine/StaticMesh.h"
#include "Algo/Reverse.h"

namespace
{
struct FRect
{
    FVector2D Min,Max;
    bool Contains(FVector2D P) const {return P.X>Min.X && P.X<Max.X && P.Y>Min.Y && P.Y<Max.Y;}
};
// Rectangulate a union/difference exactly on its compressed coordinate grid.
// No random retries, per-frame meshes, or unbounded voxel allocation.
TArray<FRect> Partition(const TArray<FRect>& Include,const TArray<FRect>& Exclude)
{
    TArray<float> X,Y;
    for(const auto* List:{&Include,&Exclude})for(const auto& R:*List)
    {X.AddUnique(R.Min.X);X.AddUnique(R.Max.X);Y.AddUnique(R.Min.Y);Y.AddUnique(R.Max.Y);}
    X.Sort();Y.Sort();TArray<FRect> Out;
    if(X.Num()>180 || Y.Num()>180 || X.Num()<2 || Y.Num()<2)return Out;
    const int32 NX=X.Num()-1,NY=Y.Num()-1;TArray<uint8> Cells;Cells.Init(0,NX*NY);
    for(int32 J=0;J<NY;++J)for(int32 I=0;I<NX;++I)
    {
        const FVector2D P((X[I]+X[I+1])*.5,(Y[J]+Y[J+1])*.5);
        bool In=false;for(const auto& R:Include)In|=R.Contains(P);
        if(In)for(const auto& R:Exclude)if(R.Contains(P)){In=false;break;}
        Cells[J*NX+I]=In;
    }
    for(int32 J=0;J<NY;++J)for(int32 I=0;I<NX;++I)if(Cells[J*NX+I])
    {
        int32 End=I+1;while(End<NX && Cells[J*NX+End])++End;
        int32 Top=J+1;
        for(;Top<NY;++Top){bool Full=true;for(int32 K=I;K<End;++K)Full&=Cells[Top*NX+K]!=0;if(!Full)break;}
        for(int32 V=J;V<Top;++V)for(int32 U=I;U<End;++U)Cells[V*NX+U]=0;
        // Sub-2cm fragments from nearly coincident parcel lines cannot form a
        // player passage and are below the minimum authored-piece thickness.
        if(X[End]-X[I]>=2 && Y[Top]-Y[J]>=2)Out.Add({FVector2D(X[I],Y[J]),FVector2D(X[End],Y[Top])});
    }
    return Out;
}
FRect Road(FVector A,FVector B,float Width,bool Caps=true)
{
    FVector2D Lo(FMath::Min(A.X,B.X),FMath::Min(A.Y,B.Y)),Hi(FMath::Max(A.X,B.X),FMath::Max(A.Y,B.Y));
    if(FMath::IsNearlyEqual(A.X,B.X)){Lo.X-=Width*.5;Hi.X+=Width*.5;if(Caps){Lo.Y-=Width*.5;Hi.Y+=Width*.5;}}
    else{Lo.Y-=Width*.5;Hi.Y+=Width*.5;if(Caps){Lo.X-=Width*.5;Hi.X+=Width*.5;}}
    return {Lo,Hi};
}
}

namespace ERProcedural
{
bool BuildDustTown(const UERMapTemplate& T,int32 Seed,TArray<FERMapTemplatePiece>& Pieces,FERProceduralLayout& L,TArray<FVector>& Patrol)
{
    const auto& V=T.Town;
    if(V.SeededDistrict)return BuildAlleyDistrict(T,Seed,Pieces,L,Patrol);
    if(T.Symmetry!=EERTemplateSymmetry::None || V.Streets.Num()<6 || V.Streets.Num()>16 || V.Courts.Num()<5 || V.Courts.Num()>12 ||
        V.Gates.Num()<2 || V.Gates.Num()>6 || V.Patrol.Num()<4 || V.Patrol.Num()>64 ||
        V.ArenaMin.ContainsNaN() || V.ArenaMax.ContainsNaN() || V.ArenaMin.X<6000 || V.ArenaMin.Y<6500 ||
        V.ArenaMax.X>9500 || V.ArenaMax.Y>11000 || V.ArenaMin.X>V.ArenaMax.X || V.ArenaMin.Y>V.ArenaMax.Y ||
        !FMath::IsFinite(V.AxisWarp) || V.AxisWarp<0 || V.AxisWarp>3 ||
        V.BuildingHeight.ContainsNaN() || V.BuildingHeight.X<400 || V.BuildingHeight.Y>900 || V.BuildingHeight.X>V.BuildingHeight.Y ||
        V.DoorGap.ContainsNaN() || V.DoorGap.X<110 || V.DoorGap.Y>140 || V.DoorGap.X>V.DoorGap.Y ||
        !V.WallSurface || !V.RoofSurface || !V.RoadSurface || !V.WoodSurface || !V.IronSurface ||
        !V.ArchMesh || !V.LeftDoorMesh || !V.RightDoorMesh)return false;
    if(V.BuildingMeshes.Num()>16)return false;
    for(const auto& Mesh:V.BuildingMeshes)if(!Mesh || Mesh->GetBoundingBox().GetSize().ContainsNaN() || Mesh->GetBoundingBox().GetSize().GetMin()<1)return false;
    FRandomStream R(Seed),Art(int32(HashCombine(uint32(Seed),0x414c4c59u)));
    L.GroundSurface=V.GroundSurface;
    L.GradedTerrain=V.TraversableTown;
    auto Pick=[&](float Min,float Max,float Grid=50){return R.RandRange(FMath::CeilToInt(Min/Grid),FMath::FloorToInt(Max/Grid))*Grid;};
    L.ArenaSize=FVector(Pick(V.ArenaMin.X,V.ArenaMax.X),Pick(V.ArenaMin.Y,V.ArenaMax.Y),T.ArenaSize.Z);
    TArray<float> WX{0},WY{0};for(int32 I=1;I<4;++I){WX.Add(I*25+R.FRandRange(-V.AxisWarp,V.AxisWarp));WY.Add(I*25+R.FRandRange(-V.AxisWarp,V.AxisWarp));}WX.Add(100);WY.Add(100);
    auto Warp=[](float P,const TArray<float>& Axis){const int32 I=FMath::Clamp(FMath::FloorToInt(P/25),0,3);return FMath::Lerp(Axis[I],Axis[I+1],(P-I*25)/25);};
    auto Point=[&](FVector2D P){return FVector((Warp(P.X,WX)-50)*L.ArenaSize.X/100,(Warp(P.Y,WY)-50)*L.ArenaSize.Y/100,0);};
    auto ValidPoint=[](FVector2D P){return !P.ContainsNaN() && P.X>=3 && P.X<=97 && P.Y>=3 && P.Y<=97;};
    Pieces.Reset();Patrol.Reset();L.AlphaSpawns.Reset();L.OmegaSpawns.Reset();
    auto Box=[&](FVector P,FVector Size,UMaterialInterface* Surface,EERBlockMaterial Material=EERBlockMaterial::Stone,bool Visual=false,UStaticMesh* Mesh=nullptr,float Yaw=0,bool Walkable=false,bool Destructible=false)
    {
        auto& B=Pieces.AddDefaulted_GetRef();B.Center=P;B.Size=Size;B.Material=Material;B.Destructible=Destructible;
        B.FixedSurface=Destructible ? nullptr:Surface;B.VisualOnly=Visual;B.FixedMesh=Mesh;B.RotationYaw=Yaw;B.WalkableSurface=Walkable;
    };
    TArray<FRect> Roads,Upper;TArray<FVector> UpperPoints;TMap<FName,float> Widths;TMap<FName,TArray<FVector>> Paths;
    float UpperWidth=0,LowerWidth=0;
    for(const auto& Street:V.Streets)
    {
        if(Street.Name.IsNone() || Widths.Contains(Street.Name) || Street.Points.Num()<2 || Street.Points.Num()>16 ||
            Street.Width.ContainsNaN() || Street.Width.X<250 || Street.Width.Y>550 || Street.Width.X>Street.Width.Y)return false;
        const float Width=Pick(Street.Width.X,Street.Width.Y,10);Widths.Add(Street.Name,Width);
        TArray<FVector> Points;for(auto P:Street.Points){if(!ValidPoint(P))return false;Points.Add(Point(P));}
        for(int32 I=1;I<Points.Num();++I)
        {
            const FVector D=Points[I]-Points[I-1];if((FMath::Abs(D.X)>1 && FMath::Abs(D.Y)>1) || D.Size()<100)return false;
            Roads.Add(Road(Points[I-1],Points[I],Width));
            if(Street.Elevated)Upper.Add(Road(Points[I-1],Points[I],Width,false));
        }
        Paths.Add(Street.Name,Points);
        if(Street.Elevated)
        {
            if(!UpperPoints.IsEmpty())return false;
            UpperPoints=Points;UpperWidth=Width;
            for(int32 I=1;I<Points.Num()-1;++I)Upper.Add({FVector2D(Points[I].X-Width*.5,Points[I].Y-Width*.5),FVector2D(Points[I].X+Width*.5,Points[I].Y+Width*.5)});
        }
        else
        {
            for(auto& P:Points)P.Z=100;
            L.RoomAccessRoutes.Add(Points);L.TownStreetNames.Add(Street.Name);
        }
        if(Street.Name==TEXT("CT_A"))LowerWidth=Width;
    }
    if(UpperPoints.Num()<4 || UpperWidth<250 || LowerWidth<250 || !Paths.Contains(TEXT("CT_A")))return false;
    L.TownCrossing=Point(V.Crossing);
    L.LowerRoute={L.TownCrossing-FVector(700,0,-100),L.TownCrossing+FVector(700,0,100)};
    TMap<FName,FVector> Courts;
    for(const auto& Court:V.Courts)
    {
        if(!ValidPoint(Court.Center) || Court.Size.ContainsNaN() || Court.Size.X<8 || Court.Size.Y<8 || Court.Size.X>25 || Court.Size.Y>25 || Courts.Contains(Court.Name))return false;
        const FVector A=Point(Court.Center-Court.Size*.5),B=Point(Court.Center+Court.Size*.5);
        FRect Rect{FVector2D(FMath::Min(A.X,B.X),FMath::Min(A.Y,B.Y)),FVector2D(FMath::Max(A.X,B.X),FMath::Max(A.Y,B.Y))};Roads.Add(Rect);
        L.TownCourts.Add(FBox(FVector(Rect.Min,0),FVector(Rect.Max,1)));Courts.Add(Court.Name,Point(Court.Center));
    }
    if(!Courts.Contains(TEXT("T_Spawn")) || !Courts.Contains(TEXT("CT_Spawn")))return false;
    for(const auto Team:{0,1})for(int32 I=0;I<10;++I)
    {
        const FVector Center=Courts[Team==0 ? TEXT("CT_Spawn"):TEXT("T_Spawn")];
        const FVector P=Center+FVector((I%5-2)*140,(I/5 ? 1:-1)*90,100);
        (Team==0 ? L.AlphaSpawns:L.OmegaSpawns).Add(FTransform(FRotator(0,Team==0 ? 90:-90,0),P));
    }
    for(auto P:V.Patrol){if(!ValidPoint(P))return false;Patrol.Add(Point(P)+FVector(0,0,100));}
    // The complement of the lane/court union is solid city fabric, rather
    // than scattered arena bars. Seed-dependent widths and axis warp reshape
    // whole building parcels, court entrances and bends together.
    const FVector Half=L.ArenaSize*.5;
    const TArray<FRect> Extent{{FVector2D(-Half.X+30,-Half.Y+30),FVector2D(Half.X-30,Half.Y-30)}};
    auto Buildings=Partition(Extent,Roads);
    if(Buildings.Num()<12 || Buildings.Num()>100){UE_LOG(LogTemp,Warning,TEXT("ER_TOWN_REJECT seed=%d parcels=%d"),Seed,Buildings.Num());return false;}
    // Divide long parcels into house-sized frontages instead of stretching a
    // single atlas/building across an entire city block. Routes stay untouched.
    if(!V.BuildingMeshes.IsEmpty())
    {
        TArray<FRect> Lots;
        for(const auto& Foot:Buildings)
        {
            const auto S=Foot.Max-Foot.Min;
            const int32 NX=FMath::Max(1,FMath::CeilToInt(S.X/1000)),NY=FMath::Max(1,FMath::CeilToInt(S.Y/1000));
            for(int32 Y=0;Y<NY;++Y)for(int32 X=0;X<NX;++X)
                Lots.Add({Foot.Min+FVector2D(S.X*X/NX,S.Y*Y/NY),Foot.Min+FVector2D(S.X*(X+1)/NX,S.Y*(Y+1)/NY)});
        }
        Buildings=MoveTemp(Lots);
    }
    if(V.TraversableTown)for(const auto& Foot:Buildings)
    {
        const float Z=AlleyBaseHeight(L.ArenaSize,(Foot.Min+Foot.Max)*.5);
        L.TerrainPads.Add(FBox(FVector(Foot.Min,Z),FVector(Foot.Max,Z)));
    }
    if(V.TraversableTown)
    {
        for(auto* Starts:{&L.AlphaSpawns,&L.OmegaSpawns})for(auto& Start:*Starts){auto P=Start.GetLocation();P.Z+=AlleyGroundHeight(L,FVector2D(P));Start.SetLocation(P);}
        for(auto& P:Patrol)P.Z+=AlleyGroundHeight(L,FVector2D(P));
        for(auto& Route:L.RoomAccessRoutes)for(auto& P:Route)P.Z+=AlleyGroundHeight(L,FVector2D(P));
        for(auto& Court:L.TownCourts){const float Z=AlleyGroundHeight(L,FVector2D(Court.GetCenter()));Court.Min.Z=Z;Court.Max.Z=Z+1;}
        for(const auto& Plan:{TArray<FVector2D>{{45,88},{45,80},{45,74},{45,64}},TArray<FVector2D>{{92,34},{92,26},{92,21},{92,16}}})
        {TArray<FVector> Route;for(auto P:Plan){auto Q=Point(P);Q.Z=100+AlleyGroundHeight(L,FVector2D(Q));Route.Add(Q);}L.GroundRoutes.Add(Route);}
    }
    int32 Windows=0;TArray<FVector> RoofCenters;
    for(const auto& Foot:Buildings)
    {
        const FVector2D S=Foot.Max-Foot.Min,C=(Foot.Max+Foot.Min)*.5;float Height=Pick(V.BuildingHeight.X,V.BuildingHeight.Y);
        const float Base=V.TraversableTown ? AlleyBaseHeight(L.ArenaSize,C):0;
        const int32 First=Pieces.Num();
        L.TownBuildings.Add(FBox(FVector(Foot.Min,Base),FVector(Foot.Max,Base+Height)));
        const bool House=!V.BuildingMeshes.IsEmpty() && S.GetMin()>=240;
        if(House)
        {
            UStaticMesh* Mesh=V.BuildingMeshes[Art.RandRange(0,V.BuildingMeshes.Num()-1)];
            float Yaw=Art.RandRange(0,3)*90;FVector2D HouseSize=S,HouseCenter=C;bool Access=false;
            FTransform AccessFrame;float Width=0,Depth=0;
            bool Distant=true;for(auto P:RoofCenters)if(FVector::Dist2D(P,FVector(C,0))<1400)Distant=false;
            if(V.TraversableTown && RoofCenters.Num()<4 && Distant)for(int32 Side=0;Side<4;++Side)
            {
                const bool Swap=Side%2!=0;const float W=Swap ? S.Y:S.X,D=Swap ? S.X:S.Y;
                if(W<580 || D<650)continue;
                const FTransform Frame(FRotator(0,Side*90,0),FVector(C,Base));
                const FVector Entry=Frame.TransformPosition(FVector(-W*.5+70,-D*.5-60,0));
                bool OnRoad=false;for(const auto& Road:Roads)OnRoad|=Road.Contains(FVector2D(Entry));
                if(!OnRoad)continue;
                Access=true;AccessFrame=Frame;Width=W;Depth=D;Yaw=Side*90;
                Height=FMath::Min3(Height,600.f,float(FMath::FloorToInt((D-320)/30))*40);
                const FVector Center=Frame.TransformPosition(FVector(150,0,Height*.5));HouseCenter=FVector2D(Center);
                HouseSize=Swap ? FVector2D(D,W-300):FVector2D(W-300,D);
                break;
            }
            Box(FVector(HouseCenter,Base+Height*.5),FVector(HouseSize,Height),nullptr,EERBlockMaterial::Stone,false,Mesh);
            Pieces.Last().MeshRotation.Yaw=Yaw;Pieces.Last().MeshCollision=V.TraversableTown;
            const bool Swap=int32(Yaw/90)%2!=0;
            const FVector LocalSize(Swap ? HouseSize.Y:HouseSize.X,Swap ? HouseSize.X:HouseSize.Y,Height);
            L.Houses.Add({FTransform(FRotator(0,Yaw,0),FVector(HouseCenter,Base),LocalSize),Mesh,Access});
            if(Access)
            {
                RoofCenters.Add(FVector(C,0));
                const int32 N=FMath::CeilToInt(Height/40);const float Rise=Height/(2*N),Run=N*30.f;
                auto P=[&](FVector V){return AccessFrame.TransformPosition(V+FVector(-Width*.5,-Depth*.5,0));};
                auto Solid=[&](FVector Center,FVector Size)
                {
                    Box(P(Center),Size,V.RoofSurface,EERBlockMaterial::Stone,false,nullptr,0,true);
                    Pieces.Last().RotationYaw=Yaw;
                };
                TArray<FVector> Route{P(FVector(70,90,100)),P(FVector(70,180,100))};
                for(int32 I=0;I<N;++I)
                {
                    const float H=(I+1)*Rise;Solid(FVector(70,200+(I+.5)*30,H*.5),FVector(120,30,H));
                    Route.Add(P(FVector(70,200+(I+.5)*30,H+100)));
                }
                Solid(FVector(150,200+Run+60,Height*.25),FVector(280,120,Height*.5));
                Route.Add(P(FVector(70,200+Run+60,Height*.5+100)));Route.Add(P(FVector(230,200+Run+60,Height*.5+100)));
                for(int32 I=0;I<N;++I)
                {
                    const float H=(N+I+1)*Rise;Solid(FVector(230,200+Run-(I+.5)*30,H*.5),FVector(120,30,H));
                    Route.Add(P(FVector(230,200+Run-(I+.5)*30,H+100)));
                }
                const float Far=300+(Width-300)*.15f;
                Solid(FVector((170+Far)*.5,140,Height-10),FVector(Far-170,120,20));
                Route.Add(P(FVector(230,140,Height+100)));Route.Add(P(FVector(Far,140,Height+100)));
                Route.Add(P(FVector(Far+50,140,Height*.94f+100)));
                L.RooftopRoutes.Add(Route);
            }
            continue;
        }
        Box(FVector(C,Height*.5-10),FVector(S,Height-20),V.WallSurface);
        Box(FVector(C,Height-10),FVector(S,20),V.RoofSurface,EERBlockMaterial::Stone,false,nullptr,0,true);
        // Rooftop parapets/cornices read as actual buildings from the alleys.
        if(S.GetMin()>120)
        {
            for(float Sign:{-1.f,1.f})
            {
                Box(FVector(C.X+Sign*(S.X*.5-10),C.Y,Height+25),FVector(20,S.Y,50),V.WallSurface);
                Box(FVector(C.X,C.Y+Sign*(S.Y*.5-10),Height+25),FVector(S.X-40,20,50),V.WallSurface);
            }
        }
        // Closed windows/shutters are cosmetic on the solid facade. They
        // never become invisible player or projectile blockers in the lane.
        for(int32 Axis=0;Axis<2;++Axis)
        {
            const float Length=Axis ? S.Y:S.X,Depth=Axis ? S.X:S.Y;
            if(Length<350 || Depth<100)continue;
            const int32 Count=FMath::Clamp(FMath::FloorToInt(Length/400),1,4);
            for(float Sign:{-1.f,1.f})for(int32 I=0;I<Count && Windows<80;++I)
            {
                ++Windows;
                const float Along=(I-(Count-1)*.5)*Length/Count;
                const FVector P=Axis ? FVector(C.X+Sign*(S.X*.5+1),C.Y+Along,260):FVector(C.X+Along,C.Y+Sign*(S.Y*.5+1),260);
                const FVector2D Outside=Axis ? FVector2D(P.X+Sign*25,P.Y):FVector2D(P.X,P.Y+Sign*25);
                bool Exposed=false;for(const auto& Lane:Roads)Exposed|=Lane.Contains(Outside);
                if(!Exposed)continue;
                Box(P,Axis ? FVector(5,75,110):FVector(75,5,110),V.WoodSurface,EERBlockMaterial::Wood,true);
                Box(P+FVector(0,0,-65),Axis ? FVector(20,95,12):FVector(95,20,12),V.RoofSurface,EERBlockMaterial::Stone,true);
            }
        }
        for(int32 I=First;I<Pieces.Num();++I)Pieces[I].Center.Z+=Base;
    }
    // A roof turns the bent western approach into readable B tunnels. It
    // meets the arch heads at 4m, leaving full standing clearance beneath.
    if(V.CoveredTunnels)
    {
        const auto* Path=Paths.Find(TEXT("B_Tunnels"));
        if(!Path || Path->Num()<8)return false;
        TArray<FRect> Roofs;
        for(int32 I=3;I<Path->Num()-1;++I)Roofs.Add(Road((*Path)[I-1],(*Path)[I],Widths[TEXT("B_Tunnels")],true));
        for(const auto& Foot:Partition(Roofs,{}))
            Box(FVector((Foot.Min+Foot.Max)*.5,415),FVector(Foot.Max-Foot.Min,30),V.RoofSurface,EERBlockMaterial::Stone,false,nullptr,0,true);
    }
    // Raised Short A: solid embankments and slabs, with exactly one bridge
    // opening above the transverse CT road. Both ends have ordinary stairs.
    TArray<FRect> StairCuts;
    for(int32 End=0;End<2;++End)
    {
        const int32 A=End ? UpperPoints.Num()-1:0,B=End ? A-1:1;
        const FVector Start=UpperPoints[A],Dir=(UpperPoints[B]-Start).GetSafeNormal();
        if(FVector::Dist(Start,UpperPoints[B])<800)return false;
        StairCuts.Add(Road(Start-Dir*UpperWidth,Start+Dir*560,UpperWidth+2,false));
        const float Base=AlleyGroundHeight(L,FVector2D(Start));
        const int32 Count=FMath::CeilToInt((320-Base)/20);const float Rise=(320-Base)/Count,Tread=560.f/Count;
        TArray<FVector> Route{Start-Dir*70+FVector(0,0,Base+100)};
        for(int32 I=0;I<Count;++I)
        {
            const float H=Base+(I+1)*Rise;const FVector P=Start+Dir*((I+.5)*Tread);
            Box(P+FVector(0,0,H*.5),FMath::Abs(Dir.X)>.5 ? FVector(Tread,UpperWidth,H):FVector(UpperWidth,Tread,H),V.RoadSurface,EERBlockMaterial::Stone,false,nullptr,0,true);
            Route.Add(P+FVector(0,0,H+100));
        }
        Route.Add(Start+Dir*650+FVector(0,0,420));L.StairRoutes.Add(Route);
    }
    for(const auto& Foot:Partition(Upper,StairCuts))
        Box(FVector((Foot.Min+Foot.Max)*.5,305),FVector(Foot.Max-Foot.Min,30),V.RoadSurface,EERBlockMaterial::Stone,false,nullptr,0,true);
    TArray<FRect> BodyCuts=StairCuts;BodyCuts.Add({FVector2D(L.TownCrossing.X-UpperWidth,L.TownCrossing.Y-LowerWidth*.5-10),FVector2D(L.TownCrossing.X+UpperWidth,L.TownCrossing.Y+LowerWidth*.5+10)});
    for(const auto& Foot:Partition(Upper,BodyCuts))
        Box(FVector((Foot.Min+Foot.Max)*.5,145),FVector(Foot.Max-Foot.Min,290),V.WallSurface);
    for(float Sign:{-1.f,1.f})Box(L.TownCrossing+FVector(Sign*(UpperWidth*.5-10),0,360),FVector(20,LowerWidth+20,80),V.WallSurface);
    L.UpperRoute=L.StairRoutes[0];
    for(int32 I=1;I<UpperPoints.Num()-1;++I)L.UpperRoute.Add(UpperPoints[I]+FVector(0,0,420));
    for(int32 I=L.StairRoutes[1].Num()-1;I>=0;--I)L.UpperRoute.Add(L.StairRoutes[1][I]);
    // Opposite-swing leaves overlap in frontal projection. Their parallel
    // faces leave a diagonal standing passage rather than a straight sightline.
    for(const auto& Gate:V.Gates)
    {
        if(!ValidPoint(Gate.Position) || !Paths.Contains(Gate.Street))return false;
        FVector P=Point(Gate.Position);const auto& Path=Paths[Gate.Street];FVector Direction;bool Found=false;
        for(int32 I=1;I<Path.Num();++I)
        {
            const FVector D=Path[I]-Path[I-1];const float Alpha=FVector::DotProduct(P-Path[I-1],D)/D.SizeSquared();
            if(Alpha>0.05 && Alpha<.95 && FVector::Dist(P,Path[I-1]+D*Alpha)<1){Direction=D.GetSafeNormal();Found=true;break;}
        }
        if(!Found)return false;
        P.Z=AlleyGroundHeight(L,FVector2D(P));
        const float Yaw=FMath::Abs(Direction.Y)>.5 ? 0:90,Width=Widths[Gate.Street],Portal=Width-80,Gap=Pick(V.DoorGap.X,V.DoorGap.Y,10),Hinge=Portal*.5-5;
        const float S=(Gap+16)/(2*Hinge),Angle=FMath::RadiansToDegrees(FMath::Asin(S)),Co=FMath::Sqrt(1-S*S);
        const FTransform Frame(FRotator(0,Yaw,0),P);
        if(V.TraversableTown)L.TerrainPads.Add(FBox(FVector(-Width*.5,-350,0),FVector(Width*.5,350,0)).TransformBy(Frame));
        const float Length=(Hinge+12)/Co;
        if(Length<60)return false;
        const float Offset=Hinge-45;
        TArray<FVector> Passage;
        for(const FVector Local:{FVector(0,-300,100),FVector(Offset,-200,100),FVector(Offset,-Offset*S/Co,100),FVector(-Offset,Offset*S/Co,100),FVector(-Offset,200,100),FVector(0,300,100)})
            Passage.Add(Frame.TransformPosition(Local));
        L.TownGates.Add({Frame,Gap,Width,Angle,Passage});
        // Replace straight route segments with the same safe dogleg used by
        // physical probes. Reverse it when the road is travelled backwards.
        auto InsertPassage=[&](TArray<FVector>& Route,bool Closed)
        {
            for(int32 I=0;I<(Closed ? Route.Num():Route.Num()-1);++I)
            {
                const int32 J=(I+1)%Route.Num();const FVector A=Route[I];FVector D=Route[J]-A;D.Z=0;
                const float Alpha=FVector::DotProduct(P-A,D)/D.SizeSquared();
                if(Alpha<=0 || Alpha>=1 || FVector::Dist2D(P,A+D*Alpha)>1)continue;
                TArray<FVector> Detour=Passage;
                if(FVector::DotProduct(D,Frame.GetRotation().GetAxisY())<0)Algo::Reverse(Detour);
                Route.Insert(Detour,I+1);break;
            }
        };
        InsertPassage(Patrol,true);
        const int32 StreetIndex=L.TownStreetNames.IndexOfByKey(Gate.Street);
        if(StreetIndex==INDEX_NONE)return false;
        InsertPassage(L.RoomAccessRoutes[StreetIndex],false);
        for(float Sign:{-1.f,1.f})
        {
            Box(Frame.TransformPosition(FVector(Sign*(Width*.5-20),0,200)),FMath::Abs(Yaw)<1 ? FVector(40,80,400):FVector(80,40,400),V.WallSurface);
            const FVector Leaf=Frame.TransformPosition(FVector(Sign*(Hinge-Length*Co*.5),Sign*Length*S*.5,140));
            Box(Leaf,FVector(Length,12,280),V.WoodSurface,EERBlockMaterial::Wood,true,Sign<0 ? V.LeftDoorMesh:V.RightDoorMesh,Yaw-Angle);
            // Dark braces/hinges rotate with each leaf, like the reference.
            const FTransform LeafFrame(FRotator(0,Yaw-Angle,0),Leaf);
            for(int32 I=0;I<6;++I)
            {
                const float U=Sign<0 ? I/6.f:(5-I)/6.f;
                const float H=210+70*FMath::Sqrt(FMath::Max(0.f,1-FMath::Square(1-U)));
                Box(LeafFrame.TransformPosition(FVector((I+.5f)*Length/6-Length*.5,0,H*.5-140)),FVector(Length/6,10,H),V.WoodSurface,EERBlockMaterial::Wood,false,nullptr,Yaw-Angle);
                Pieces.Last().Anchored=V.TraversableTown;
            }
            for(float Z:{-75.f,0.f,75.f})Box(LeafFrame.TransformPosition(FVector(0,-9,Z)),FVector(Length-10,5,9),V.IronSurface,EERBlockMaterial::Metal,true,nullptr,Yaw-Angle);
        }
        Box(P+FVector(0,0,200),FVector(Width,80,400),V.WallSurface,EERBlockMaterial::Stone,true,V.ArchMesh,Yaw);
        // Curved-head collision is above standing clearance. Narrow segments
        // approximate the imported project-owned arch without sealing its gap.
        for(int32 I=0;I<24;++I)
        {
            const float X0=-Portal*.5+Portal*I/24,X1=-Portal*.5+Portal*(I+1)/24;
            const float Edge=FMath::Min(FMath::Abs(X0),FMath::Abs(X1))/(Portal*.5);
            const float Bottom=225+95*FMath::Sqrt(FMath::Max(0.f,1-Edge*Edge));
            Box(Frame.TransformPosition(FVector((X0+X1)*.5,0,(Bottom+400)*.5)),FMath::Abs(Yaw)<1 ? FVector(X1-X0,80,400-Bottom):FVector(80,X1-X0,400-Bottom),V.WallSurface);
        }
    }
    // Place project cover off the route centre-lines, after reserving streets.
    TArray<FBox> Used;
    for(const auto& Ref:{FVector2D(30,92),FVector2D(10,15),FVector2D(20,13),FVector2D(86,12),FVector2D(78,69),FVector2D(22,57)})
    {
        const FVector P=Point(Ref)+FVector(Pick(-50,50),Pick(-50,50),0);const FVector Size(Pick(100,200),120,Pick(75,150));
        const FBox Bounds(P-FVector(Size.X*.5,Size.Y*.5,0),P+FVector(Size.X*.5,Size.Y*.5,Size.Z));bool Clear=true;
        for(const auto& B:L.TownBuildings)if(B.ExpandBy(10).Intersect(Bounds))Clear=false;
        for(const auto& B:Used)if(B.Intersect(Bounds))Clear=false;
        for(const auto& Street:L.RoomAccessRoutes)for(int32 I=1;I<Street.Num();++I)
            if(FMath::LineBoxIntersection(Bounds.ExpandBy(FVector(90,90,100)),Street[I-1],Street[I],Street[I]-Street[I-1]))Clear=false;
        for(const auto* Starts:{&L.AlphaSpawns,&L.OmegaSpawns})for(const auto& Start:*Starts)if(Bounds.ExpandBy(FVector(100,100,100)).IsInside(Start.GetLocation()))Clear=false;
        if(Clear){Box(P+FVector(0,0,Size.Z*.5+AlleyGroundHeight(L,FVector2D(P))),Size,nullptr,EERBlockMaterial::Wood,false,nullptr,0,false,true);Pieces.Last().Anchored=V.TraversableTown;Used.Add(Bounds);}
    }
    if(Pieces.Num()>1024 || Used.IsEmpty()){UE_LOG(LogTemp,Warning,TEXT("ER_TOWN_REJECT seed=%d pieces=%d parcels=%d cover=%d"),Seed,Pieces.Num(),Buildings.Num(),Used.Num());return false;}
    return true;
}
}
