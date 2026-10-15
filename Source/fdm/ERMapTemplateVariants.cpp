#include "ERMapTemplate.h"
#include "ERProceduralMap.h"
#include "Engine/StaticMesh.h"

namespace ERProcedural
{
bool BuildTemplateVariants(const UERMapTemplate& T,int32 Seed,TArray<FERMapTemplatePiece>& Pieces,FERProceduralLayout& L,TArray<FVector>& Patrol)
{
    const auto& V=T.Variation;
    auto Range=[](FVector2D R,float Min,float Max)
    {
        return !R.ContainsNaN() && R.X>=Min && R.Y<=Max && R.X<=R.Y && FMath::Fmod(R.X,50)==0 && FMath::Fmod(R.Y,50)==0;
    };
    if(T.Symmetry!=EERTemplateSymmetry::HalfTurn || V.ArenaMin.ContainsNaN() || V.ArenaMax.ContainsNaN() ||
        !Range(FVector2D(V.ArenaMin.X,V.ArenaMax.X),3600,4800) || !Range(FVector2D(V.ArenaMin.Y,V.ArenaMax.Y),7000,9000) ||
        !Range(V.BaseWidth,1000,1400) || !Range(V.BaseDepth,850,1050) || V.MinRows<2 || V.MaxRows>3 || V.MinRows>V.MaxRows || V.Modules.IsEmpty() || V.Modules.Num()>16) return false;
    for(auto Kind:V.Modules)if(uint8(Kind)>uint8(EERTemplateModule::Crates))return false;
    for(auto* Mesh:{V.CargoMesh.Get(),V.TankMesh.Get(),V.BarrelMesh.Get(),V.ColumnMesh.Get(),V.BeamMesh.Get(),V.GuardMesh.Get()})
        if(Mesh && (Mesh->GetBoundingBox().GetSize().ContainsNaN() || Mesh->GetBoundingBox().GetSize().GetMin()<.1))return false;
    FRandomStream R(Seed);
    auto Pick=[&](float A,float B){return float(R.RandRange(FMath::CeilToInt(A/50),FMath::FloorToInt(B/50))*50);};
    const float ArenaX=Pick(V.ArenaMin.X,V.ArenaMax.X),ArenaY=Pick(V.ArenaMin.Y,V.ArenaMax.Y);
    L.ArenaSize=FVector(ArenaX,ArenaY,T.ArenaSize.Z);
    const FVector Half=L.ArenaSize*.5;
    Pieces.Reset();L.AlphaSpawns.Reset();L.OmegaSpawns.Reset();
    auto Box=[&](FVector P,FVector Size,EERBlockMaterial Material=EERBlockMaterial::Concrete,bool Fixed=false,UStaticMesh* Mesh=nullptr,float Yaw=0,bool Walkable=false,bool Anchored=false,bool Mirrored=true)
    {
        auto& Piece=Pieces.AddDefaulted_GetRef();Piece.Center=P;Piece.Size=Size;Piece.Material=Material;
        Piece.Destructible=!Fixed;Piece.FixedMesh=Mesh;Piece.MeshRotation=FRotator(0,Yaw,0);Piece.Mirrored=Mirrored;Piece.WalkableSurface=Walkable;Piece.Anchored=Anchored;
    };
    // A base is an assembly with invariant rear entry, two front windows, a
    // genuine stairwell and solid roof. Only its dimensions/frame are sampled.
    const float BW=Pick(V.BaseWidth.X,V.BaseWidth.Y),BD=Pick(V.BaseDepth.X,V.BaseDepth.Y),H=Pick(250,300),RoofZ=H+20;
    const float BaseX=Pick(-200,200),RearOffset=Pick(650,800);
    const FVector Origin(BaseX,-Half.Y+RearOffset+BD*.5,0);
    const float StairX=(R.RandRange(0,1) ? 1:-1)*BW*.25,Rear=-BD*.5,Front=BD*.5;
    const int32 Steps=FMath::CeilToInt(RoofZ/20);const float Rise=RoofZ/Steps,StairStart=Rear+90,StairEnd=StairStart+Steps*30;
    auto BaseBox=[&](FVector P,FVector Size,bool Fixed=false,bool Walkable=false,bool Anchor=false)
    {Box(Origin+P,Size,Fixed ? EERBlockMaterial::Metal:EERBlockMaterial::Concrete,Fixed,nullptr,0,Walkable,Anchor);};
    for(float Sign:{-1.f,1.f})BaseBox(FVector(Sign*(BW*.5-10),0,H*.5),FVector(20,BD,H));
    auto Strip=[&](float Lo,float Hi,float Y,float Bottom,float Top)
    {if(Hi>Lo && Top>Bottom)BaseBox(FVector((Lo+Hi)*.5,Y,(Bottom+Top)*.5),FVector(Hi-Lo,20,Top-Bottom));};
    Strip(-BW*.5+20,StairX-100,Rear+10,0,H);Strip(StairX+100,BW*.5-20,Rear+10,0,H);Strip(StairX-100,StairX+100,Rear+10,225,H);
    Strip(-BW*.5+20,-BW*.25-80,Front-10,0,H);Strip(-BW*.25+80,BW*.25-80,Front-10,0,H);Strip(BW*.25+80,BW*.5-20,Front-10,0,H);
    for(float X:{-BW*.25,BW*.25}){Strip(X-80,X+80,Front-10,0,100);Strip(X-80,X+80,Front-10,200,H);}
    auto Deck=[&](float X0,float X1,float Y0,float Y1)
    {BaseBox(FVector((X0+X1)*.5,(Y0+Y1)*.5,H+10),FVector(X1-X0,Y1-Y0,20),true,true);};
    Deck(-BW*.5,StairX-110,Rear,Front);Deck(StairX+110,BW*.5,Rear,Front);
    Deck(StairX-110,StairX+110,Rear,StairStart);Deck(StairX-110,StairX+110,StairEnd,Front);
    for(int32 I=0;I<Steps;++I)BaseBox(FVector(StairX,StairStart+(I+.5)*30,(I+.5)*Rise),FVector(180,30,Rise),true,true);
    for(float Sign:{-1.f,1.f})BaseBox(FVector(Sign*(BW*.5-10),0,RoofZ+50),FVector(20,BD,100),false,false,true);
    for(float Y:{Rear+10,Front-10})BaseBox(FVector(0,Y,RoofZ+50),FVector(BW-40,20,100),false,false,true);
    // Guard the sides of the intentional stair opening, keeping the upper exit
    // open. These rails touch the deck and cannot float or fill the stairwell.
    for(float X:{StairX-120,StairX+120})BaseBox(FVector(X,(StairStart+StairEnd)*.5,RoofZ+50),FVector(20,StairEnd-StairStart,100),true);
    for(int32 Copy=0;Copy<2;++Copy)
    {
        FERTemplateBase Base;Base.Frame=FTransform(FRotator(0,Copy ? 180:0,0),Copy ? FVector(-Origin.X,-Origin.Y,0):Origin);
        Base.Size=FVector(BW,BD,H);Base.StairX=StairX;Base.StairStartY=StairStart;Base.StepCount=Steps;Base.RoofZ=RoofZ;L.Bases.Add(Base);
        TArray<FVector> Route{Base.Point(FVector(StairX,Rear-100,100)),Base.Point(FVector(StairX,Rear+45,100))};
        for(int32 I=0;I<Steps;++I)Route.Add(Base.Point(FVector(StairX,StairStart+(I+.5)*30,(I+1)*Rise+100)));
        Route.Add(Base.Point(FVector(StairX,StairEnd+120,RoofZ+100)));L.StairRoutes.Add(Route);
    }
    // Symmetric team starts stay behind the moving base, with two clear rows.
    for(int32 I=0;I<10;++I)L.AlphaSpawns.Add(FTransform(FRotator(0,90,0),FVector((I%5-2)*250,-Half.Y+150+(I/5)*200,100)));
    // An outer circulation loop is reserved before placing any battle modules.
    const float Lane=Half.X-180,End=Half.Y-250;
    Patrol={FVector(-Lane,-End,100),FVector(-Lane,0,100),FVector(-Lane,End,100),FVector(Lane,End,100),FVector(Lane,0,100),FVector(Lane,-End,100)};
    // Sample complete motifs inside disjoint cells. Gaps connect all cell rows
    // to the side loop and the central aisle; no retry-dependent placement.
    const float FieldMin=Origin.Y+Front+180,FieldMax=-180;
    const int32 MaxRows=FMath::Min(V.MaxRows,FMath::FloorToInt((FieldMax-FieldMin)/600));
    if(MaxRows<V.MinRows)return false;
    const int32 Rows=R.RandRange(V.MinRows,MaxRows);const float CellH=(FieldMax-FieldMin)/Rows;
    const int32 RequiredRoom=V.Modules.Contains(EERTemplateModule::Room) ? R.RandRange(0,Rows*2-1):-1;
    for(int32 Row=0;Row<Rows;++Row)for(int32 Col=0;Col<2;++Col)
    {
        const int32 Index=Row*2+Col;
        const auto Kind=Index==RequiredRoom ? EERTemplateModule::Room:V.Modules[R.RandRange(0,V.Modules.Num()-1)];L.ModuleKinds.Add(Kind);
        const float CellW=Half.X-700;
        const FVector Center((Col ? 1:-1)*(350+CellW*.5),FieldMin+(Row+.5)*CellH,0);
        const float W=Pick(650,FMath::Min(1000.f,CellW-100)),D=Pick(450,FMath::Min(800.f,CellH-150));
        const float CoverH=Pick(100,200);const auto M=R.RandRange(0,2)==0 ? EERBlockMaterial::Wood:EERBlockMaterial::Concrete;
        auto Part=[&](FVector P,FVector Size,EERBlockMaterial Mat,bool Fixed=false,UStaticMesh* Mesh=nullptr,float Yaw=0)
        {Box(Center+P,Size,Mat,Fixed,Mesh,Yaw);};
        if(Kind==EERTemplateModule::Room)
        {
            const float RH=Pick(250,300),Door=R.RandRange(-1,1)*100;
            // Side walls own corners; opposing doors create a real through room.
            for(float X:{-W*.5+10,W*.5-10})
            {
                Part(FVector(X,0,50),FVector(20,D,100),M);
                for(float Y:{-(D+120)*.25,(D+120)*.25})Part(FVector(X,Y,150),FVector(20,(D-120)*.5,100),M);
                Part(FVector(X,0,(200+RH)*.5),FVector(20,D,RH-200),M);
            }
            for(float Y:{-D*.5+10,D*.5-10})
            {
                const float Left=-W*.5+20,Right=W*.5-20;
                Part(FVector((Left+Door-80)*.5,Y,RH*.5),FVector(Door-80-Left,20,RH),M);
                Part(FVector((Door+80+Right)*.5,Y,RH*.5),FVector(Right-Door-80,20,RH),M);
                Part(FVector(Door,Y,(225+RH)*.5),FVector(160,20,RH-225),M);
            }
            for(float Sign:{1.f,-1.f})
            {
                const FVector C=FVector(Center.X*Sign,Center.Y*Sign,0);
                L.Rooms.Add({FVector2D(C.X-W*.5,C.Y-D*.5),FVector2D(C.X+W*.5,C.Y+D*.5),L.Rooms.Num()});
                TArray<FVector> Route;
                for(float Y:{-D*.5-65,0.,D*.5+65})Route.Add(FVector((Center.X+Door)*Sign,(Center.Y+Y)*Sign,100));
                L.RoomAccessRoutes.Add(Route);
            }
        }
        else if(Kind==EERTemplateModule::LCover || Kind==EERTemplateModule::UCover)
        {
            const float Y=R.RandRange(0,1) ? D*.5-15:-D*.5+15;
            Part(FVector(0,Y,CoverH*.5),FVector(W,30,CoverH),M);
            Part(FVector(-W*.5+15,0,CoverH*.5),FVector(30,D-60,CoverH),M);
            if(Kind==EERTemplateModule::UCover)Part(FVector(W*.5-15,0,CoverH*.5),FVector(30,D-60,CoverH),M);
            // Same motif can be fixed industrial cover or project destruction.
            if(R.RandRange(0,1))for(int32 I=Pieces.Num()-(Kind==EERTemplateModule::UCover ? 3:2);I<Pieces.Num();++I)
            {Pieces[I].Destructible=false;Pieces[I].FixedMesh=T.ShellWallMesh;Pieces[I].MeshRotation=FRotator(0,Pieces[I].Size.X>Pieces[I].Size.Y ? 90:0,0);Pieces[I].Material=EERBlockMaterial::Metal;}
        }
        else if(Kind==EERTemplateModule::StaggeredWalls)
        {
            for(int32 I=0;I<3;++I)Part(FVector((I-1)*W*.3,(I%2 ? 1:-1)*D*.25,CoverH*.5),FVector(W*.22,30,CoverH),M);
        }
        else if(Kind==EERTemplateModule::Cargo)
        {
            const bool Across=D<650 || R.RandRange(0,1);const float Length=Pick(500,600),Width=Pick(250,FMath::Min(300.f,D-200)),Height=Pick(250,300);
            Part(FVector(0,0,Height*.5),Across ? FVector(Length,Width,Height):FVector(Width,Length,Height),EERBlockMaterial::Metal,true,V.CargoMesh,Across ? 90:0);
            for(float X:{-W*.5+80,W*.5-80})Part(FVector(X,D*.5-50,50),FVector(100,100,100),EERBlockMaterial::Wood);
        }
        else if(Kind==EERTemplateModule::Tanks)
        {
            const int32 Count=R.RandRange(2,3);const float Height=Pick(100,150);
            for(int32 I=0;I<Count;++I)Part(FVector((I-(Count-1)*.5)*220,-D*.18,Height*.5),FVector(150,200,Height),EERBlockMaterial::Metal,true,V.TankMesh);
            for(int32 I=0;I<3;++I)Part(FVector((I-1)*90,D*.3,40),FVector(55,55,80),EERBlockMaterial::Metal,true,V.BarrelMesh);
        }
        else
        {
            const int32 Count=R.RandRange(2,4);
            for(int32 I=0;I<Count;++I)
            {
                const float X=(I-(Count-1)*.5)*170,Y=I%2 ? D*.2:-D*.2,Height=Pick(75,150);
                Part(FVector(X,Y,Height*.5),FVector(140,140,Height),EERBlockMaterial::Wood);
                if(R.RandRange(0,1))Part(FVector(X,Y,Height+25),FVector(90,100,50),EERBlockMaterial::Wood);
            }
        }
    }
    // The watch platform changes proportions, while both teams retain stairs.
    const float TW=Pick(400,550),TD=Pick(450,600),TZ=Pick(300,350);
    Box(FVector(0,0,TZ-10),FVector(TW,TD,20),EERBlockMaterial::Metal,true,nullptr,0,true,false,false);
    for(float X:{-TW*.5+25,TW*.5-25})Box(FVector(X,-TD*.5+25,(TZ-20)*.5),FVector(40,40,TZ-20),EERBlockMaterial::Metal,true,V.ColumnMesh);
    Box(FVector(TW*.5-10,0,TZ+50),FVector(20,TD,100),EERBlockMaterial::Metal,true,V.GuardMesh);
    const int32 TowerSteps=FMath::CeilToInt(TZ/20);const float TowerRise=TZ/TowerSteps,TowerStart=-TD*.5-TowerSteps*30;
    for(int32 I=0;I<TowerSteps;++I)Box(FVector(0,TowerStart+(I+.5)*30,(I+.5)*TowerRise),FVector(180,30,TowerRise),EERBlockMaterial::Metal,true,nullptr,0,true);
    for(float Sign:{1.f,-1.f})
    {
        TArray<FVector> Route{FVector(0,(TowerStart-100)*Sign,100)};
        for(int32 I=0;I<TowerSteps;++I)Route.Add(FVector(0,(TowerStart+(I+.5)*30)*Sign,(I+1)*TowerRise+100));
        Route.Add(FVector(0,-TD*.25*Sign,TZ+100));L.StairRoutes.Add(Route);
    }
    const int32 BeamRows=FMath::FloorToInt(Half.Y/900),BeamCols=FMath::CeilToInt(L.ArenaSize.X/1200);
    for(int32 Row=0;Row<BeamRows;++Row)
    {
        const float Y=-Half.Y+(Row+.5)*Half.Y/BeamRows;
        for(int32 Col=0;Col<BeamCols;++Col)Box(FVector((Col-(BeamCols-1)*.5)*L.ArenaSize.X/BeamCols,Y,T.ArenaSize.Z-30),FVector(L.ArenaSize.X/BeamCols,40,60),EERBlockMaterial::Metal,true,V.BeamMesh);
        for(float X:{-Half.X+25,Half.X-25})Box(FVector(X,Y,(T.ArenaSize.Z-60)*.5),FVector(50,50,T.ArenaSize.Z-60),EERBlockMaterial::Metal,true,V.ColumnMesh);
    }
    return Pieces.Num()<=256;
}
}
