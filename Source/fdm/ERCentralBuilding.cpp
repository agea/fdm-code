#include "ERMapTemplate.h"
#include "ERProceduralMap.h"
#include "Engine/StaticMesh.h"
#include "Paths/PCGExPath.h"
#include "Data/PCGPointData.h"
#include "PCGDataAsset.h"
#include "Metadata/PCGMetadata.h"
#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "UObject/Package.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#endif

namespace ERProcedural
{
bool BuildCentralBuilding(const UERMapTemplate& T,int32 Seed,TArray<FERMapTemplatePiece>& Pieces,FERProceduralLayout& L,TArray<FVector>& Patrol,int32 CrossingOverride)
{
    const auto& V=T.CentralBuilding;
    auto Range=[](FVector2D R,float Min,float Max)
    {return !R.ContainsNaN() && R.X>=Min && R.Y<=Max && R.X<=R.Y && FMath::Fmod(R.X,50)==0 && FMath::Fmod(R.Y,50)==0;};
    if(T.Symmetry!=EERTemplateSymmetry::HalfTurn ||
        !Range({V.ArenaMin.X,V.ArenaMax.X},6400,8000) || !Range({V.ArenaMin.Y,V.ArenaMax.Y},8400,10000) ||
        !Range(V.BuildingWidth,1800,2400) || !Range(V.BuildingDepth,2200,3000) ||
        V.MinFloors<1 || V.MaxFloors>3 || V.MinFloors>V.MaxFloors || T.RoofMesh || T.ShellWallMesh)return false;
    for(auto* Mesh:{V.FenceMesh.Get(),V.ContainerMesh.Get(),V.TankMesh.Get(),V.BarrelMesh.Get()})
        if(Mesh && (Mesh->GetBoundingBox().GetSize().ContainsNaN() || Mesh->GetBoundingBox().GetSize().GetMin()<.1))return false;
    if(V.ContainerSurfaces.Num()>8 || V.ContainerSurfaces.Contains(nullptr))return false;
    FRandomStream R(Seed);
    auto Pick=[&](float A,float B){return float(R.RandRange(int32(A/50),int32(B/50))*50);};
    L.CrossingRule=CrossingOverride ? EERCrossingRule(CrossingOverride):EERCrossingRule(R.RandRange(1,3));
    L.BuildingFloors=R.RandRange(V.MinFloors,V.MaxFloors);
    const float W=Pick(V.BuildingWidth.X,V.BuildingWidth.Y),D=Pick(V.BuildingDepth.X,V.BuildingDepth.Y);
    const float Stride=320,Roof=L.BuildingFloors*Stride;
    L.ArenaSize=FVector(Pick(V.ArenaMin.X,V.ArenaMax.X),Pick(V.ArenaMin.Y,V.ArenaMax.Y),1600);
    L.CentralBuildingSize=FVector(W,D,Roof);
    const FVector Half=L.ArenaSize*.5;
    Pieces.Reset();L.AlphaSpawns.Reset();L.OmegaSpawns.Reset();
    auto Box=[&](FVector P,FVector Size,EERBlockMaterial M,bool Fixed=false,UStaticMesh* Mesh=nullptr,bool Pair=true,bool Deck=false)->FERMapTemplatePiece&
    {
        auto& B=Pieces.AddDefaulted_GetRef();B.Center=P;B.Size=Size;B.Material=M;B.Destructible=!Fixed;
        B.Mirrored=Pair;B.FixedMesh=Mesh;B.WalkableSurface=Deck;
        const float Bottom=P.Z-Size.Z*.5;
        // Only a real upper slab seeds support. Door/window headers must still
        // depend on their jambs, rather than becoming floating permanent anchors.
        B.Anchored=Bottom>0 && FMath::IsNearlyZero(FMath::Fmod(Bottom,Stride),.01f);
        if(Mesh && Mesh==V.ContainerMesh)B.MeshRotation=FRotator(0,90,0);
        if(M==EERBlockMaterial::Metal && !Mesh)
        {
            if(Fixed)B.FixedSurface=Deck && V.DeckSurface ? V.DeckSurface:V.SteelSurface;
            else B.CellSurface=V.SteelSurface;
        }
        return B;
    };
    auto Slab=[&](float X0,float X1,float Y0,float Y1,float Z,bool Pair=true)
    {Box({(X0+X1)*.5,(Y0+Y1)*.5,Z-10},{X1-X0,Y1-Y0,20},EERBlockMaterial::Metal,true,nullptr,Pair,true);};
    auto Place=[&](const FTransform& Frame,FVector P,FVector Size,EERBlockMaterial Material,bool Deck=false)->FERMapTemplatePiece&
    {
        auto& B=Box(Frame.TransformPosition(P),Size,Material,false,nullptr,true,Deck);
        B.RotationYaw=Frame.Rotator().Yaw;B.Anchored=false;return B;
    };
    auto TimberPlate=[&](const FTransform& Frame,float X0,float X1,float Y0,float Y1,float Top)
    {if(X1>X0 && Y1>Y0)Place(Frame,{(X0+X1)*.5,(Y0+Y1)*.5,Top-1},{X1-X0,Y1-Y0,2},EERBlockMaterial::Wood,true);};
    auto TimberFlight=[&](const FTransform& Frame,float X,float StartY,int32 Floor,bool WallAccess)
    {
        const float Base=Floor*Stride;
        for(int32 I=0;I<16;++I)
        {
            const float Top=Base+(I+1)*20,Y=StartY+(I+.5)*30;
            Place(Frame,{X,Y,Top-1},{180,30,2},EERBlockMaterial::Wood,true);
            // Higher flights rest on the previous flight's treads through the
            // open stairwell. No invisible fixed base or upper anchor remains.
            const float Bottom=Floor ? Base-Stride+(I+1)*20:0,Height=Top-2-Bottom;
            for(float S:{-1.f,1.f})Place(Frame,{X+S*78,Y,Bottom+Height*.5},{12,2,Height},EERBlockMaterial::Wood);
        }
        for(int32 Copy=0;Copy<2;++Copy)
        {
            const FTransform Pair=Frame*FTransform(FRotator(0,Copy ? 180:0,0));
            TArray<FVector> Path{Pair.TransformPosition({X,StartY-100,Base+100})};
            for(int32 I=0;I<16;++I)Path.Add(Pair.TransformPosition({X,StartY+(I+.5)*30,Base+(I+1)*20+120}));
            Path.Add(Pair.TransformPosition({X,WallAccess ? 0:StartY+580,Base+420}));
            if(WallAccess)L.WallAccessFlights.Add(L.StairRoutes.Num());
            L.StairRoutes.Add(MoveTemp(Path));
        }
    };
    auto SteelLadder=[&](const FTransform& Frame,FVector Origin,float DeckHeight)
    {
        const float Height=DeckHeight+110;
        // Calibrated 2mm folded steel; paired geometry and movement metadata.
        for(float Y:{-42.f,42.f})
        {
            for(float X:{-3.9f,3.9f})Place(Frame,Origin+FVector(X,Y,Height*.5),{.2f,8,Height},EERBlockMaterial::Metal);
            for(float Edge:{-3.9f,3.9f})Place(Frame,Origin+FVector(0,Y+Edge,Height*.5),{7.6f,.2f,Height},EERBlockMaterial::Metal);
        }
        for(float Z=25;Z<DeckHeight;Z+=25)Place(Frame,Origin+FVector(0,0,Z),{.2f,76,4},EERBlockMaterial::Metal);
        const FTransform LadderFrame(Frame.GetRotation(),Frame.TransformPosition(Origin));
        L.Ladders.Add({LadderFrame,Height,DeckHeight});
        L.Ladders.Add({LadderFrame*FTransform(FRotator(0,180,0)),Height,DeckHeight});
    };
    auto Container=[&](FVector Center,FVector Size,float Yaw)
    {
        auto& B=Box(Center,Size,EERBlockMaterial::Metal);B.RotationYaw=Yaw;B.Strength=8;B.Anchored=false;
        // Visual rolls are independent for the two copies and never consume geometry RNG.
        if(!V.ContainerSurfaces.IsEmpty())
        {
            const uint32 PaletteSeed=HashCombineFast(uint32(Seed),0x46414354u);
            B.CellSurface=V.ContainerSurfaces[HashCombineFast(PaletteSeed,uint32(L.Containers.Num()))%V.ContainerSurfaces.Num()];
            B.MirroredCellSurface=V.ContainerSurfaces[HashCombineFast(PaletteSeed,uint32(L.Containers.Num()+1))%V.ContainerSurfaces.Num()];
        }
        for(int32 Copy=0;Copy<2;++Copy)L.Containers.Add({FTransform(FRotator(0,Yaw+(Copy ? 180:0),0),Copy ? FVector(-Center.X,-Center.Y,Center.Z):Center),Size});
    };
    const auto MainMaterial=R.RandRange(0,1) ? EERBlockMaterial::Concrete:EERBlockMaterial::Brick;
    // Half-turn pairs include the central building itself, both stairwells and
    // all floors. Stairs repeat every storey with a genuinely open floor well.
    const float SX=-W*.5+180,Start=-D*.5+220,End=Start+480;
    for(int32 Floor=0;Floor<L.BuildingFloors;++Floor)
    {
        const float Z=Floor*Stride;
        // South facade: offset door and two windows. The north door is rotated.
        const float Door=-300,DoorHalf=140;
        auto Facade=[&](float A,float B,float Lo,float Hi)
        {if(B>A && Hi>Lo)Box({(A+B)*.5,-D*.5+10,Z+(Lo+Hi)*.5},{B-A,20,Hi-Lo},MainMaterial);};
        if(Floor==0)
        {
            Facade(-W*.5,Door-DoorHalf,0,300);Facade(Door+DoorHalf,W*.5,0,300);
            Facade(Door-DoorHalf,Door+DoorHalf,240,300);
        }
        else
        {
            for(float X:{-W*.25,W*.25}){Facade(X-100,X+100,0,100);Facade(X-100,X+100,210,300);}
            Facade(-W*.5,-W*.25-100,0,300);Facade(-W*.25+100,W*.25-100,0,300);Facade(W*.25+100,W*.5,0,300);
        }
        Box({-W*.5+10,0,Z+150},{20,D-40,300},MainMaterial);
        const float DeckZ=Z+Stride;
        Slab(-W*.5+290,W*.5-290,-D*.5,D*.5,DeckZ,false);
        Slab(-W*.5,-W*.5+70,-D*.5,D*.5,DeckZ);
        Slab(-W*.5+70,-W*.5+290,-D*.5,Start,DeckZ);
        Slab(-W*.5+70,-W*.5+290,End,D*.5,DeckZ);
        // 20cm risers and 30cm treads; the landing is part of the solid deck.
        for(int32 I=0;I<16;++I)Box({SX,Start+(I+.5)*30,Z+(I+1)*20-10},{180,30,20},EERBlockMaterial::Metal,true,nullptr,true,true);
        for(int32 Copy=0;Copy<2;++Copy)
        {
            const FTransform Frame(FRotator(0,Copy ? 180:0,0),FVector::ZeroVector);
            TArray<FVector> Path{Frame.TransformPosition({SX,Start-100,Z+100})};
            // The standing capsule reaches the next riser before its centre;
            // reserve that step-up clearance in the route sample as well.
            for(int32 I=0;I<16;++I)Path.Add(Frame.TransformPosition({SX,Start+(I+.5)*30,Z+(I+1)*20+120}));
            Path.Add(Frame.TransformPosition({SX,End+100,DeckZ+100}));L.StairRoutes.Add(Path);
        }
        if(L.CrossingRule==EERCrossingRule::FencedCrossings)
        {
            const int32 N=FMath::CeilToInt((W*.5-20)/300);const float Width=(W*.5-20)/N;
            for(int32 I=0;I<N;++I)for(int32 Row=0;Row<2;++Row)
            {Box({-W*.5+20+(I+.5)*Width,0,Z+(Row+.5)*150},{Width,12,150},EERBlockMaterial::Metal,true,V.FenceMesh);Pieces.Last().PawnBarrier=true;}
        }
    }
    // Roof parapets leave the two stairs and the crossing over their landings open.
    // Paired exterior access supplements, rather than replaces, the main stairs.
    const float MainLadderY=-D*.25;
    SteelLadder(FTransform::Identity,{W*.5+35,MainLadderY,0},Roof);
    for(const auto& Interval:{FVector2D(-D*.5,-MainLadderY-80),FVector2D(-MainLadderY+80,D*.5)})
        Box({-W*.5+10,(Interval.X+Interval.Y)*.5,Roof+50},{20,Interval.Y-Interval.X,100},EERBlockMaterial::Concrete);
    Box({0,-D*.5+10,Roof+50},{W-40,20,100},EERBlockMaterial::Concrete);
    // Guard the stairwell's inward side, but leave its top mouth open.
    Box({SX+120,(Start+End)*.5,Roof+50},{20,End-Start,100},EERBlockMaterial::Metal,true);
    const float SideX=Half.X-650;
    const bool SideBuildings=L.CrossingRule!=EERCrossingRule::OpenYard || R.RandRange(0,1)==1;
    const float SideW=Pick(700,800),SideD=Pick(1000,1200);
    float HighestRoof=Roof;
    if(SideBuildings)
    {
        const int32 Floors=R.RandRange(1,3);
        const bool Tower=L.CrossingRule!=EERCrossingRule::FencedCrossings && R.RandRange(0,1)==1;
        const bool Yard=L.CrossingRule==EERCrossingRule::ClimbableWall;
        const float SW=Yard ? 700:SideW,SD=Yard ? 1000:SideD;
        const float Yaw=L.CrossingRule==EERCrossingRule::FencedCrossings ? 0:R.RandRange(-1,1)*20;
        const FTransform Frame(FRotator(0,Yaw,0),FVector(-SideX,Yard ? -D*.5-950:0,0));
        const auto Material=R.RandRange(0,1) ? EERBlockMaterial::Brick:EERBlockMaterial::Concrete;
        // Towers always use ladders; reserve full stairwells for the broadest buildings.
        const bool LadderAccess=Tower || SW<800 || SD<1150;
        const float StairX=-SW*.5+180,StairStart=-SD*.5+220,StairEnd=StairStart+480;
        HighestRoof=FMath::Max(HighestRoof,Floors*Stride);
        for(int32 Copy=0;Copy<2;++Copy)L.SecondaryStructures.Add({Frame*FTransform(FRotator(0,Copy ? 180:0,0)),{SW,SD,Floors*Stride},Floors,Material,Tower,LadderAccess});
        for(int32 Floor=0;Floor<Floors;++Floor)
        {
            const float Base=Floor*Stride,Top=Base+Stride,H=Stride-2;
            if(Tower)
            {
                for(float X:{-SW*.5+35,SW*.5-35})for(float Y:{-SD*.5+35,SD*.5-35})
                    Place(Frame,{X,Y,Base+H*.5},{12,2,H},EERBlockMaterial::Wood);
            }
            else
            {
                const auto M=Floor%2 ? (Material==EERBlockMaterial::Brick ? EERBlockMaterial::Concrete:EERBlockMaterial::Brick):Material;
                for(float X:{-SW*.5+10,SW*.5-10})Place(Frame,{X,0,Base+H*.5},{20,SD-40,H},M);
                for(float Y:{-SD*.5+10,SD*.5-10})
                {
                    const float Gap=Floor ? 220:280;
                    for(float Sign:{-1.f,1.f})Place(Frame,{Sign*(SW+Gap)*.25,Y,Base+H*.5},{(SW-Gap)*.5,20,H},M);
                    if(Floor)Place(Frame,{0,Y,Base+50},{Gap,20,100},M);
                    const float Lintel=Floor ? 210:240;
                    Place(Frame,{0,Y,Base+(Lintel+H)*.5},{Gap,20,H-Lintel},M);
                }
            }
            if(LadderAccess)
            {
                // Alternate the hatch between storeys. Each ladder rests on a
                // solid lower deck and reaches every floor, not just the roof.
                const float HatchX=-SW*.5+160,HatchY=(Floor%2 ? 1.f:-1.f)*SD*.25;
                TimberPlate(Frame,-SW*.5,HatchX-80,-SD*.5,SD*.5,Top);
                TimberPlate(Frame,HatchX+80,SW*.5,-SD*.5,SD*.5,Top);
                TimberPlate(Frame,HatchX-80,HatchX+80,-SD*.5,HatchY-80,Top);
                TimberPlate(Frame,HatchX-80,HatchX+80,HatchY+80,SD*.5,Top);
                // Face the left-hand hatch toward the room, keeping the
                // Bank crossing through the two doors unobstructed below.
                const FTransform Inner(FRotator(0,180,0),FVector(HatchX+35,HatchY,Base));
                SteelLadder(Inner*Frame,FVector::ZeroVector,Stride);
            }
            else
            {
                TimberPlate(Frame,-SW*.5,StairX-110,-SD*.5,SD*.5,Top);
                TimberPlate(Frame,StairX+110,SW*.5,-SD*.5,SD*.5,Top);
                TimberPlate(Frame,StairX-110,StairX+110,-SD*.5,StairStart,Top);
                TimberPlate(Frame,StairX-110,StairX+110,StairEnd,SD*.5,Top);
                TimberFlight(Frame,StairX,StairStart,Floor,false);
            }
        }
        // Exterior steel ladder remains an alternative route on either type.
        const float LadderY=-SD*.25;
        SteelLadder(Frame,{SW*.5+35,LadderY,0},Floors*Stride);
        // The handles extend above the roof; rungs stop below its lip, leaving
        // a clear entry between the handles for descending from the roof.
        Place(Frame,{-SW*.5+1,0,Floors*Stride+50},{2,SD-4,100},EERBlockMaterial::Wood);
        for(const auto& Interval:{FVector2D(-SD*.5+2,LadderY-80),FVector2D(LadderY+80,SD*.5-2)})
            Place(Frame,{SW*.5-1,(Interval.X+Interval.Y)*.5,Floors*Stride+50},{2,Interval.Y-Interval.X,100},EERBlockMaterial::Wood);
        for(float Y:{-SD*.5+1,SD*.5-1})Place(Frame,{0,Y,Floors*Stride+50},{SW,2,100},EERBlockMaterial::Wood);
    }
    auto Fence=[&](float A,float B,float Y,float Height)
    {
        if(B<=A)return;
        const int32 N=FMath::CeilToInt((B-A)/400);const float Width=(B-A)/N;
        const int32 Rows=FMath::CeilToInt(Height/200);const float H=Height/Rows;
        for(int32 I=0;I<N;++I)for(int32 Row=0;Row<Rows;++Row)
        {Box({A+(I+.5)*Width,Y,(Row+.5)*H},{Width,12,H},EERBlockMaterial::Metal,true,V.FenceMesh);Pieces.Last().PawnBarrier=true;}
    };
    if(L.CrossingRule==EERCrossingRule::FencedCrossings)
    {
        // No ground bypass and no destroyable shortcut: only the central roof
        // or the two side buildings cross this permanent separation.
        Fence(-Half.X+12,-SideX-SideW*.5,0,HighestRoof+250);
        Fence(-SideX+SideW*.5,-W*.5,0,HighestRoof+250);
        for(float X:{-SideX,SideX})L.CrossingRoutes.Add({{X,-SideD*.5-150,100},{X,SideD*.5+150,100}});
        L.CrossingRoutes.Add({{SX,End+110,Roof+100},{-SX,-End-110,Roof+100}});
    }
    else
    {
        if(L.CrossingRule==EERCrossingRule::ClimbableWall)
        {
            // Brick can be breached; timber access can be cut down/collapsed.
            Box({-(Half.X-12+W*.5)*.5,0,150},{Half.X-12-W*.5,30,300},EERBlockMaterial::Brick);
            const float X=-W*.5-400;
            const FTransform Bridge(FVector(X,0,0));
            TimberPlate(Bridge,-100,100,-120,120,320);
            for(float PX:{-80.f,80.f})for(float PY:{-90.f,90.f})Place(Bridge,{PX,PY,159},{12,2,318},EERBlockMaterial::Wood);
            L.WallBridges={{X,0,320},{-X,0,320}};
            for(float Sign:{-1.f,1.f})
            {
                TimberFlight(FTransform(FRotator(0,Sign>0 ? 180:0,0),FVector(X,0,0)),0,-600,0,true);
                for(int32 Copy=0;Copy<2;++Copy)L.ClimbPoints.Add({Copy ? -X:X,Sign*700,100});
            }
            for(float WingX:{X,-X})L.CrossingRoutes.Add({{WingX,-70,420},{WingX,70,420}});
        }
        else
            for(float X:{-(W*.5+400),W*.5+400})L.CrossingRoutes.Add({{X,-200,100},{X,200,100}});
        // Ground route doglegs through the opposing offset doors.
        L.CrossingRoutes.Add({{-300,-D*.5-150,100},{-300,-200,100},{300,200,100},{300,D*.5+150,100}});
    }
    // Perimeter fencing is fixed, high enough to exceed all traversal limits.
    // It is independent of the crossing rule and never becomes cell geometry.
    const float PerimeterHeight=FMath::Max(600.f,HighestRoof+350),FenceRowHeight=PerimeterHeight/FMath::CeilToInt(PerimeterHeight/200);
    Fence(-Half.X,Half.X,-Half.Y+6,PerimeterHeight);
    const int32 Count=FMath::CeilToInt((L.ArenaSize.Y-24)/400);
    for(int32 I=0;I<Count;++I)for(int32 Row=0;Row<FMath::CeilToInt(PerimeterHeight/200);++Row)
    {
        const float Length=(L.ArenaSize.Y-24)/Count;
        Box({-Half.X+6,-Half.Y+12+(I+.5)*Length,(Row+.5)*FenceRowHeight},{12,Length,FenceRowHeight},EERBlockMaterial::Metal,true,V.FenceMesh);
        Pieces.Last().PawnBarrier=true;
        Pieces.Last().MeshRotation=FRotator(0,90,0);
    }
    // Cover is chosen in reserved cells; the two teams never roll independently.
    // Keep a clear rear spawn strip, side-building lanes and central approach.
    Container({-1100,-Half.Y+1000,140},{Pick(1000,1200),260,280},R.RandRange(-1,1)*15);
    const float MinY=-Half.Y+1800,MaxY=-D*.5-450;
    const int32 Rows=R.RandRange(1,FMath::Clamp(FMath::FloorToInt((MaxY-MinY)/400),1,3));
    for(int32 Row=0;Row<Rows;++Row)for(float Sign:{-1.f,1.f})
    {
        const float Y=MinY+(Row+.5)*(MaxY-MinY)/Rows;
        const float X=Sign*(W*.5+500);
        const int32 Kind=R.RandRange(0,7);const float Yaw=R.RandRange(-3,3)*15;
        L.CoverAngles.Add(Yaw);L.CoverAngles.Add(Yaw+180);
        L.ModuleKinds.Add(EERTemplateModule(Kind==0 || Kind==7 ? uint8(EERTemplateModule::Cargo):Kind==1 ? uint8(EERTemplateModule::Tanks):Kind>=4 && Kind<=5 ? uint8(EERTemplateModule::LCover):uint8(EERTemplateModule::Crates)));
        if(Kind==0 || Kind==7)Container({X,Y,Kind==0 ? 130.f:70.f},{350,220,Kind==0 ? 260.f:140.f},Yaw);
        else if(Kind==1){Box({X,Y,130},{280,280,260},EERBlockMaterial::Metal,true,V.TankMesh).RotationYaw=Yaw;}
        else if(Kind==2 || Kind==6)
        {
            Box({X,Y,50},{240,240,100},EERBlockMaterial::Wood).RotationYaw=Yaw;
            if(Kind==6)Box({X,Y,150},{200,200,100},EERBlockMaterial::Wood).RotationYaw=Yaw;
        }
        else if(Kind>=4)Box({X,Y,60},{300,40,120},Kind==4 ? EERBlockMaterial::Brick:EERBlockMaterial::Concrete).RotationYaw=Yaw;
        else for(float Offset:{-70.f,70.f})
        {const FVector P=FTransform(FRotator(0,Yaw,0),FVector(X,Y,55)).TransformPosition({Offset,0,0});Box(P,{100,100,110},EERBlockMaterial::Metal,true,V.BarrelMesh).RotationYaw=Yaw;}
    }
    for(int32 I=0;I<10;++I)L.AlphaSpawns.Add(FTransform(FRotator(0,90,0),FVector(-850+(I%5-2)*180,-Half.Y+220+(I/5)*200,100)));
    const float EndY=Half.Y-600;
    if(L.CrossingRule==EERCrossingRule::FencedCrossings)
        Patrol={{-SideX,-EndY,100},{-SideX,EndY,100},{SideX,EndY,100},{SideX,-EndY,100}};
    else
        Patrol={{-300,-EndY,100},{-300,-D*.5-150,100},{-300,-200,100},{300,200,100},{300,D*.5+150,100},{300,EndY,100},
                {200,EndY,100},{200,D*.5+150,100},{300,250,100},{-300,-250,100},{-200,-D*.5-150,100},{-200,-EndY,100}};
    // PCGEx's path representation is shared with the authoring preview. Reject
    // degenerate paths before they can become patrol or crossing instructions.
    for(const auto& Route:L.CrossingRoutes)
    {
        TArray<FTransform> Transforms;for(const auto& P:Route)Transforms.Add(FTransform(P));
        const PCGExPaths::FPath Path(TConstPCGValueRange<FTransform>(MakeConstStridedView(Transforms)),false);
        if(Path.TotalLength<100 || !FMath::IsFinite(Path.TotalLength))return false;
        for(const auto& Edge:Path.Edges)if(!Path.IsEdgeValid(Edge))return false;
    }
    return Pieces.Num()<=1536;
}
}

UPCGDataAsset* UERMapTemplate::CreatePCGPreview(int32 Seed) const
{
    FERProceduralConfig C;C.TemplateAsset=const_cast<UERMapTemplate*>(this);C.Seed=Seed;
    FERProceduralLayout L;if(!ERProcedural::Build(C,FERBlockRoomConfig(),L))return nullptr;
    auto* Asset=NewObject<UPCGDataAsset>();
    auto Add=[&](FName Pin,const TArray<FTransform>& Transforms,int32 Route)
    {
        auto* Data=NewObject<UPCGPointData>(Asset);
        auto* Id=Data->Metadata->CreateAttribute<int32>(TEXT("RouteId"),Route,false,true);
        auto& Points=Data->GetMutablePoints();
        for(int32 I=0;I<Transforms.Num();++I)
        {
            auto& P=Points.AddDefaulted_GetRef();P.Transform=Transforms[I];P.Seed=int32(HashCombine(uint32(Seed),uint32(I)));
            P.MetadataEntry=Data->Metadata->AddEntry();Id->SetValue(P.MetadataEntry,Route);
        }
        auto& Tagged=Asset->Data.TaggedData.AddDefaulted_GetRef();Tagged.Data=Data;Tagged.Pin=Pin;Tagged.Tags.Add(Pin.ToString());
    };
    Add(TEXT("AlphaSpawns"),L.AlphaSpawns,-1);Add(TEXT("OmegaSpawns"),L.OmegaSpawns,-1);
    for(int32 I=0;I<L.CrossingRoutes.Num();++I)
    {TArray<FTransform> Path;for(const auto& P:L.CrossingRoutes[I])Path.Add(FTransform(P));Add(TEXT("Crossings"),Path,I);}
    TArray<FTransform> Fixed;for(const auto& P:L.FixedPieces)if(!P.VisualOnly)Fixed.Add(P.Transform);
    Add(TEXT("FixedVolumes"),Fixed,-1);
    TArray<FTransform> Cells;
    for(const auto& P:L.Panels)Cells.Add(FTransform(P.Transform.GetRotation(),P.Transform.TransformPosition(P.Size*.5),P.Size/100));
    Add(TEXT("CellVolumes"),Cells,-1);
    return Asset;
}

UPCGDataAsset* UERMapTemplate::SavePCGPreview(const FString& PackageName,int32 Seed) const
{
#if WITH_EDITOR
    if(!PackageName.StartsWith(TEXT("/Game/EclipseRecoil/Art/MapTemplates/IndustrialPCG/")) || !FPackageName::IsValidLongPackageName(PackageName) || FPackageName::DoesPackageExist(PackageName))return nullptr;
    auto* Source=CreatePCGPreview(Seed);if(!Source)return nullptr;
    auto* Package=CreatePackage(*PackageName);
    auto* Saved=DuplicateObject<UPCGDataAsset>(Source,Package,*FPackageName::GetLongPackageAssetName(PackageName));
    Saved->SetFlags(RF_Public|RF_Standalone);FAssetRegistryModule::AssetCreated(Saved);Package->MarkPackageDirty();
    FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;Args.SaveFlags=SAVE_NoError;
    const FString Filename=FPackageName::LongPackageNameToFilename(PackageName,FPackageName::GetAssetPackageExtension());
    return UPackage::SavePackage(Package,Saved,*Filename,Args) ? Saved:nullptr;
#else
    return nullptr;
#endif
}

bool UERMapTemplate::DeleteArchivedProceduralLab()
{
#if WITH_EDITOR
    // UE's forced World deletion leaves this legacy package behind. Follow
    // the project's existing unloaded-package cleanup path for this exact map.
    const FString Name=TEXT("/Game/EclipseRecoil/Maps/L_ProceduralLab");
    if(!IFileManager::Get().FileExists(*(FPaths::ProjectSavedDir()/TEXT("MapRefactorBackup/2026-10-19/receipt.json"))) || FindPackage(nullptr,*Name))return false;
    auto& Registry=FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
    TArray<FName> References;Registry.GetReferencers(FName(*Name),References,UE::AssetRegistry::EDependencyCategory::Package|UE::AssetRegistry::EDependencyCategory::Manage);
    if(!References.IsEmpty())return false;
    FString Filename;if(!FPackageName::DoesPackageExist(Name,&Filename))return false;
    if(!IFileManager::Get().Delete(*Filename,false,true))return false;
    Registry.PackageDeleted(CreatePackage(*Name));return true;
#else
    return false;
#endif
}
