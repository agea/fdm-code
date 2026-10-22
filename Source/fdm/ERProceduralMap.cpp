#include "ERProceduralMap.h"
#include "ERLadder.h"
#include "ERMapTemplate.h"
#include "ERIndustrialFence.h"
#include "Components/DynamicMeshComponent.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "Engine/StaticMesh.h"
#include "PhysicsEngine/BodySetup.h"
#include "ERBotController.h"
#include "EngineUtils.h"

namespace ERProcedural { bool BuildTemplate(const FERProceduralConfig&,const FERBlockRoomConfig&,FERProceduralLayout&,const FERCellSettings&); }
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"

static bool BuildLevel(const FERProceduralConfig& C, const FERBlockRoomConfig& B, FERProceduralLayout& Out,bool StairEntrance,bool ReturnEntrance,const FERCellSettings& Cells)
{
    // Bounded, aligned MVP. Reject unsupported dimensions before allocating.
    const FVector S = B.BlockSize;
    if (S != FVector(50,20,25) || !FMath::IsFinite(B.Height) || B.Height < 250 || B.Height > 500 ||
        FMath::Fmod(B.Height,25) != 0 || !FMath::IsFinite(B.ChunkSize) || B.ChunkSize < 100 ||
        C.RoomCount < 6 || C.RoomCount > 10 || C.MinRoomSize < 400 || C.MaxRoomSize > 800 || C.MinRoomSize > C.MaxRoomSize ||
        C.MinRoomSize % 50 || C.MaxRoomSize % 50 || C.CorridorWidth < 150 || C.CorridorWidth > 400 || C.CorridorWidth % 50 ||
        C.BranchLength < 150 || C.BranchLength > 800 || C.BranchLength % 50 || C.DoorWidth < 100 || C.DoorWidth > 200 || C.DoorWidth % 50 ||
        C.DoorHeight < 200 || C.DoorHeight > B.Height - 25 || C.DoorHeight % 25) return false;
    FERProceduralLayout L;
    FRandomStream Random(C.Seed);
    const int32 Columns = (C.RoomCount + 1) / 2;
    const float HalfHall = C.CorridorWidth * .5f;
    const float Near = HalfHall + C.BranchLength;
    L.Corridors.Add({FVector2D(-450,-HalfHall), FVector2D((Columns-1)*1000+450,HalfHall)});
    auto Wall = [&](FVector Start, FVector Direction, float Length, EERBlockMaterial Material)
    {
        return L.Walls.Add({Start, Direction, Length, Material});
    };
    auto Rectangle = [&](const FERLayoutRect& R, EERBlockMaterial M)
    {
        const int32 First = L.Walls.Num();
        // Horizontal sides stop at the inside corners. Vertical returns own them.
        Wall(FVector(R.Min.X,R.Min.Y-10,0), FVector(1,0,0), R.Max.X-R.Min.X, M);
        Wall(FVector(R.Max.X+10,R.Min.Y-20,0), FVector(0,1,0), R.Max.Y-R.Min.Y+40, M);
        Wall(FVector(R.Min.X,R.Max.Y+10,0), FVector(1,0,0), R.Max.X-R.Min.X, M);
        Wall(FVector(R.Min.X-10,R.Min.Y-20,0), FVector(0,1,0), R.Max.Y-R.Min.Y+40, M);
        return First;
    };
    const int32 Hall = Rectangle(L.Corridors[0], EERBlockMaterial::Concrete);
    if(StairEntrance)L.Openings.Add({Hall+3,float(20+(C.CorridorWidth-150)*.5),150,0,B.Height,false});
    if(ReturnEntrance)L.Openings.Add({Hall+2,0,100,0,float(C.DoorHeight),false});
    for (int32 Index = 0; Index < C.RoomCount; ++Index)
    {
        const int32 Col = Index / 2;
        const bool Top = Index % 2 == 0;
        const float X = Col * 1000;
        const float W = Random.RandRange(C.MinRoomSize/50,C.MaxRoomSize/50)*50;
        const float H = Random.RandRange(C.MinRoomSize/50,C.MaxRoomSize/50)*50;
        FERLayoutRect R{FVector2D(X-W*.5f, Top ? Near : -Near-H), FVector2D(X+W*.5f, Top ? Near+H : -Near), Index};
        L.Rooms.Add(R);
        const int32 Choice=(Index+Random.RandRange(0,2))%3;
        const auto Material=static_cast<EERBlockMaterial>(C.AdditionalMaterials ? (Index+uint32(C.Seed)%6)%6 : Choice);
        const int32 First = Rectangle(R, Material);
        const int32 DoorWall = First + (Top ? 0 : 2);
        L.Openings.Add({DoorWall, (W-C.DoorWidth)*.5f, float(C.DoorWidth), 0, float(C.DoorHeight), false});
        // Common hall door coordinates are aligned from its fixed -450cm origin.
        // Width=150 requires an offset25. Split intervals below clip exact jambs.
        L.Openings.Add({Hall+(Top ? 2 : 0), X+450-C.DoorWidth*.5f, float(C.DoorWidth), 0, float(C.DoorHeight), false});
        const int32 WindowColumn = Random.RandRange(1, int32(W/50)-3);
        L.Openings.Add({First+(Top ? 2 : 0), float(WindowColumn*50), 100, 100, 100, true});
        const float Y0 = Top ? HalfHall : -Near, Y1 = Top ? Near : -HalfHall;
        L.Corridors.Add({FVector2D(X-C.DoorWidth*.5f,Y0), FVector2D(X+C.DoorWidth*.5f,Y1)});
        // No end walls: the existing room/hall walls own each doorway. Trim
        // branch sides to touch their outer faces, avoiding corner duplication.
        for (float Side : {-1.f,1.f})
            Wall(FVector(X+Side*(C.DoorWidth*.5f+10),Y0+20,0), FVector(0,1,0), Y1-Y0-40, Material);
        const FVector Center((R.Min.X+R.Max.X)*.5f,(R.Min.Y+R.Max.Y)*.5f,100);
        L.Routes.Add({Center,FVector(X,Top ? Near-50 : -Near+50,100),FVector(X,0,100)});
        if (Index == 0) L.Spawn = Center;
    }
    // Each wall slot becomes its construction's cell panels; openings are holes
    // (door jambs stay exact: openings lie on the 25cm grid, cells on 5cm).
    const float T=B.BlockSize.Y;
    for (int32 W=0; W<L.Walls.Num(); ++W)
    {
        const auto& D=L.Walls[W];
        const FRotator R=D.Direction.Rotation();
        const FVector Across=R.RotateVector(FVector(0,1,0));
        const int32 First=L.Panels.Num();
        FERCellWorld::AddConstruction(W,D.Material,FTransform(R,D.Start-Across*T*.5f),FVector(D.Length,T,B.Height),Cells,L.Panels);
        for (int32 P=First; P<L.Panels.Num(); ++P)
        {
            L.Panels[P].bAnchorBottom=true;
            for (const auto& O:L.Openings) if (O.Wall==W) L.Panels[P].Holes.Add(FBox2D(FVector2D(O.Along,O.Sill-D.Start.Z),FVector2D(O.Along+O.Width,O.Sill-D.Start.Z+O.Height)));
        }
    }
    if (C.WindowGlass)
        for (const auto& O:L.Openings) if (O.bWindow)
        {
            // One pane per window, resting on its fixed frame.
            const auto& D=L.Walls[O.Wall];
            const FRotator R=D.Direction.Rotation();
            const FVector Across=R.RotateVector(FVector(0,1,0));
            const int32 First=L.Panels.Num();
            FERCellWorld::AddConstruction(100000+O.Wall,EERBlockMaterial::Glass,FTransform(R,D.Start-Across*T*.5f+D.Direction*O.Along+FVector(0,0,O.Sill)),FVector(O.Width,T,O.Height),Cells,L.Panels);
            for (int32 P=First; P<L.Panels.Num(); ++P) L.Panels[P].bAnchorBottom=true;
        }
    Out=MoveTemp(L); return true;
}
bool ERProcedural::Build(const FERProceduralConfig& C,const FERBlockRoomConfig& B,FERProceduralLayout& Out,const FERCellSettings& Cells)
{
    if(C.TemplateAsset) return BuildTemplate(C,B,Out,Cells);
    if (C.Floors<1 || C.Floors>3 || (C.Floors>1 && C.BranchLength<250)) return false;
    FERProceduralLayout L;
    const float Stride=B.Height+20;
    for (int32 Level=0;Level<C.Floors;++Level)
    {
        FERProceduralConfig Single=C;Single.Floors=C.Floors;Single.Seed=int32(uint32(C.Seed)+uint32(Level)*7919u);
        FERProceduralLayout Part;if (!BuildLevel(Single,B,Part,Level<C.Floors-1,Level>0,Cells)) return false;
        const float Base=Level*Stride;const int32 WallOffset=L.Walls.Num(),RoomOffset=L.Rooms.Num();
        for (auto R:Part.Rooms){R.Z=Base;R.Level=Level;R.Room+=RoomOffset;L.Rooms.Add(R);}
        for (auto R:Part.Corridors){R.Z=Base;R.Level=Level;L.Corridors.Add(R);}
        for (auto W:Part.Walls){W.Start.Z+=Base;L.Walls.Add(W);}
        for (auto O:Part.Openings){O.Wall+=WallOffset;O.Sill+=Base;L.Openings.Add(O);}
        for (auto Route:Part.Routes){for(auto& P:Route)P.Z+=Base;L.Routes.Add(MoveTemp(Route));}
        // Upper walls rest on their static slab: bottom rows stay anchored.
        for (auto P:Part.Panels){P.Id+=WallOffset*8;P.Transform.AddToTranslation(FVector(0,0,Base));L.Panels.Add(MoveTemp(P));}
        if (Level==0)L.Spawn=Part.Spawn;
    }
    // A straight 20cm-rise stair with an adjacent return lane. Lower hall
    // ceilings are shared upper slabs; the tower opening provides headroom.
    const float LaneMin=C.CorridorWidth*.5+50,LaneMax=LaneMin+150,LaneY=(LaneMin+LaneMax)*.5;
    const int32 Count=FMath::CeilToInt(Stride/20);const float Rise=Stride/Count,End=-450-Count*30;
    for(int32 Level=0;Level<C.Floors-1;++Level)
    {
        const float Base=Level*Stride,Top=Base+Stride;
        TArray<FVector> Route{FVector(-400,0,Base+100)};
        for(int32 I=0;I<Count;++I)
        {
            const float Height=(I+1)*Rise;const float X=-450-I*30-15;
            L.Steps.Add(FTransform(FQuat::Identity,FVector(X,0,Base+Height-Rise*.5),FVector(30,150,Rise)/100));
            Route.Add(FVector(X,0,Base+Height+100));
        }
        const float Run=Count*30;
        L.StairSupports.Add(FTransform(FRotator(-FMath::RadiansToDegrees(FMath::Atan2(Stride,Run)),0,0),FVector(-450-Run*.5,0,Base+Stride*.5-12),FVector(FMath::Sqrt(Run*Run+Stride*Stride),150,12)/100));
        // Disjoint upper landing, return lane and near-hall connector.
        L.Corridors.Add({FVector2D(End-150,-75),FVector2D(End,LaneMax),INDEX_NONE,Top,Level+1});
        L.Corridors.Add({FVector2D(End,LaneMin),FVector2D(-350,LaneMax),INDEX_NONE,Top,Level+1});
        L.Corridors.Add({FVector2D(-450,C.CorridorWidth*.5),FVector2D(-350,LaneMin),INDEX_NONE,Top,Level+1});
        Route.Append({FVector(End-75,0,Top+100),FVector(End-75,LaneY,Top+100),FVector(-400,LaneY,Top+100),FVector(-400,0,Top+100)});
        L.StairRoutes.Add(MoveTemp(Route));
    }
    Out=MoveTemp(L);return true;
}
void ERProcedural::AddPieceCells(int32 Id, EERBlockMaterial Material, const FBox& Box, bool bAnchoredBase, const FERCellSettings& Cells, TArray<FERCellPanelDesc>& Out)
{
    // Exact validated volume: each axis holds round(L/CellSize) equal cells, so stacked
    // pieces keep touching (Source-derived sizes are rarely whole 5cm multiples).
    const FBox& B=Box;
    const FVector Size=B.GetSize();
    if (Size.GetMin()<.1) return;
    const bool bGround=B.Min.Z<=.01, bThin=Material==EERBlockMaterial::Wood || Material==EERBlockMaterial::Drywall || Material==EERBlockMaterial::Metal || Material==EERBlockMaterial::Glass;
    const int32 First=Out.Num();
    if (!bThin || FMath::Min(Size.X,Size.Y)<=30)
    {
        // Slab or wall: X along the longer horizontal side.
        if (Size.X>=Size.Y) FERCellWorld::AddConstruction(Id,Material,FTransform(B.Min),Size,Cells,Out);
        else FERCellWorld::AddConstruction(Id,Material,FTransform(FRotator(0,90,0),FVector(B.Max.X,B.Min.Y,B.Min.Z)),FVector(Size.Y,Size.X,Size.Z),Cells,Out);
        for (int32 P=First;P<Out.Num();++P) Out[P].bAnchorBottom=bGround || bAnchoredBase;
        return;
    }
    // Hollow crate: boards of the construction thickness, never a solid block of wood.
    const float W=Material==EERBlockMaterial::Wood ? Cells.WoodDepth : Material==EERBlockMaterial::Drywall ? Cells.DrywallBoard : Material==EERBlockMaterial::Metal ? Cells.SheetThickness : Cells.GlassThickness;
    auto Board=[&](int32 Layer, const FBox& Part, int32 Thin, bool bBottom)
    {
        const FVector S=Part.GetSize();
        FERCellPanelDesc& P=Out.AddDefaulted_GetRef();
        P.Id=Id*8+Layer; P.Material=Material; P.bAnchorBottom=bBottom;
        // Local Y is the thin axis; frames are right-handed rotations.
        if (Thin==1) { P.Transform=FTransform(Part.Min); P.Size=S; }
        else if (Thin==0) { P.Transform=FTransform(FMatrix(FVector(0,1,0),FVector(-1,0,0),FVector(0,0,1),FVector(Part.Max.X,Part.Min.Y,Part.Min.Z))); P.Size=FVector(S.Y,S.X,S.Z); }
        else { P.Transform=FTransform(FMatrix(FVector(1,0,0),FVector(0,0,1),FVector(0,-1,0),FVector(Part.Min.X,Part.Max.Y,Part.Min.Z))); P.Size=FVector(S.X,S.Z,S.Y); }
        P.Transform.SetScale3D(FVector::OneVector);
        P.Cell=FVector(FERCellWorld::FitCell(P.Size.X,Cells.CellSize),P.Size.Y,FERCellWorld::FitCell(P.Size.Z,Cells.CellSize));
    };
    const bool bBottom=bGround || bAnchoredBase;
    // Side tops meet the roof underside. Overlapping thin shells have no
    // external face contact, leaving the roof unsupported in the cell graph.
    Board(0,FBox(B.Min,FVector(B.Max.X,B.Min.Y+W,B.Max.Z-W)),1,bBottom);
    Board(1,FBox(FVector(B.Min.X,B.Max.Y-W,B.Min.Z),FVector(B.Max.X,B.Max.Y,B.Max.Z-W)),1,bBottom);
    Board(2,FBox(B.Min,FVector(B.Min.X+W,B.Max.Y,B.Max.Z-W)),0,bBottom);
    Board(3,FBox(FVector(B.Max.X-W,B.Min.Y,B.Min.Z),FVector(B.Max.X,B.Max.Y,B.Max.Z-W)),0,bBottom);
    Board(4,FBox(FVector(B.Min.X,B.Min.Y,B.Max.Z-W),B.Max),2,false);
}
AERProceduralMap::AERProceduralMap()
{
    auto Create=[&](const TCHAR* Name)
    {
        auto* Mesh=CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
        Mesh->SetupAttachment(RootComponent); Mesh->SetCollisionProfileName(TEXT("BlockAll"));
        Mesh->SetCanEverAffectNavigation(false); return Mesh;
    };
    TileFloors=Create(TEXT("TileFloors")); WoodFloors=Create(TEXT("WoodFloors")); Ceilings=Create(TEXT("Ceilings")); Stairs=Create(TEXT("IndestructibleStairs"));
    FixedCollision=Create(TEXT("FixedTemplateCollision"));FixedCollision->SetHiddenInGame(true);FixedCollision->SetCastShadow(false);
    FenceCollision=Create(TEXT("FixedFenceCollision"));FenceCollision->SetHiddenInGame(true);FenceCollision->SetCastShadow(false);
    // This is a locomotion volume, not solid WorldStatic cover. Object-type
    // blast queries must see the perforated metal instead of this full box.
    FenceCollision->SetCollisionObjectType(ECC_Pawn);
    FenceCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
    FenceCollision->SetCollisionResponseToChannel(ECC_Pawn,ECR_Block);
    FenceCollision->SetCollisionResponseToChannel(ECC_WorldDynamic,ECR_Block);
}
bool AERProceduralMap::GenerateLogicalWorld()
{
    FERProceduralLayout Next;
    if (!ERProcedural::Build(LayoutConfig,Config,Next,CellSettings) || !BuildCellWorld(Next.Panels)) return false;
    // Live cell state belongs to the cell world; retain only layout metadata.
    Next.Panels.Empty();
    Layout=MoveTemp(Next); return true;
}
void AERProceduralMap::ConfigureFloors()
{
    for(auto Ladder:LadderActors)if(Ladder)Ladder->Destroy();
    LadderActors.Reset();
    if(AlleyTerrain)
    {
        RemoveInstanceComponent(AlleyTerrain);AlleyTerrain->DestroyComponent();
        AlleyTerrain->Rename(*MakeUniqueObjectName(this,UDynamicMeshComponent::StaticClass(),TEXT("DeadAlleyTerrain")).ToString(),nullptr,REN_DontCreateRedirectors|REN_NonTransactional);
        AlleyTerrain=nullptr;
    }
    for(auto& Mesh:FixedMeshes)
    {
        RemoveInstanceComponent(Mesh); Mesh->DestroyComponent();
        // Free the deterministic name before the replacement group claims it.
        Mesh->Rename(*MakeUniqueObjectName(this, UInstancedStaticMeshComponent::StaticClass(), TEXT("DeadTemplateFixed")).ToString(), nullptr, REN_DontCreateRedirectors | REN_NonTransactional);
    }
    FixedMeshes.Empty();FixedCollision->ClearInstances();FenceCollision->ClearInstances();Ceilings->SetHiddenInGame(false);
    for(auto& Metal:FenceMetal)
    {
        RemoveInstanceComponent(Metal);Metal->DestroyComponent();
        Metal->Rename(*MakeUniqueObjectName(this,UDynamicMeshComponent::StaticClass(),TEXT("DeadFenceMetal")).ToString(),nullptr,REN_DontCreateRedirectors|REN_NonTransactional);
    }
    FenceMetal.Empty();
    if(LayoutConfig.TemplateAsset) {ConfigureTemplateFloors(); return;}
    // A foundation touches the underside of the 20cm interior floor slabs.
    // Its exposed asphalt apron is one walkable step down after an outer breach.
    FVector2D Min=Layout.Corridors[0].Min, Max=Layout.Corridors[0].Max;
    for (const auto& R:Layout.Rooms) { Min.X=FMath::Min(Min.X,R.Min.X);Min.Y=FMath::Min(Min.Y,R.Min.Y);Max.X=FMath::Max(Max.X,R.Max.X);Max.Y=FMath::Max(Max.Y,R.Max.Y); }
    Floor->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); Floor->SetVisibility(true);
    Floor->SetRelativeLocation(FVector((Min.X+Max.X)*.5f,(Min.Y+Max.Y)*.5f,-30));
    Floor->SetRelativeScale3D(FVector(Max.X-Min.X+400,Max.Y-Min.Y+400,20)/100);
    if (ExteriorFloorSurface) Floor->SetMaterial(0,ExteriorFloorSurface);
    for (auto* Mesh : {TileFloors.Get(),WoodFloors.Get(),Ceilings.Get(),Stairs.Get()}) { Mesh->ClearInstances(); Mesh->SetStaticMesh(BlockMesh); }
    if (FloorSurface) TileFloors->SetMaterial(0,FloorSurface);
    if (WoodFloorSurface) WoodFloors->SetMaterial(0,WoodFloorSurface);
    if (CeilingSurface) Ceilings->SetMaterial(0,CeilingSurface);
    auto Add=[&](const FERLayoutRect& R, UInstancedStaticMeshComponent* Mesh)
    {
        const FVector Center((R.Min.X+R.Max.X)*.5f,(R.Min.Y+R.Max.Y)*.5f,R.Z-10);
        const FVector Scale(R.Max.X-R.Min.X,R.Max.Y-R.Min.Y,20);
        Mesh->AddInstance(FTransform(FQuat::Identity,Center,Scale/100));
        if (bCeilings && R.Level==LayoutConfig.Floors-1) Ceilings->AddInstance(FTransform(FQuat::Identity,FVector(Center.X,Center.Y,R.Z+Config.Height+5),FVector(Scale.X,Scale.Y,10)/100));
    };
    for (const auto& R:Layout.Rooms) Add(R,R.Room%2 ? WoodFloors.Get() : TileFloors.Get());
    for (const auto& R:Layout.Corridors) Add(R,TileFloors);
    if (FloorSurface) Stairs->SetMaterial(0,FloorSurface);
    for (const auto& T:Layout.Steps) Stairs->AddInstance(T);
    for (const auto& T:Layout.StairSupports) Stairs->AddInstance(T);
}
FString AERProceduralMap::GetLayoutStatistics() const
{
    if(LayoutConfig.TemplateAsset) return FString::Printf(TEXT("template=%s rev=%d seed=%d arena=%.0fx%.0f rooms=%d modules=%d fixed=%d starts=%d/%d"),*LayoutConfig.TemplateAsset->TemplateId,LayoutConfig.TemplateAsset->Revision,LayoutConfig.Seed,Layout.ArenaSize.X,Layout.ArenaSize.Y,Layout.Rooms.Num(),Layout.ModuleKinds.Num()*2,Layout.FixedPieces.Num(),Layout.AlphaSpawns.Num(),Layout.OmegaSpawns.Num());
    return FString::Printf(TEXT("seed=%d rooms=%d corridors=%d openings=%d floors=%d steps=%d"),LayoutConfig.Seed,Layout.Rooms.Num(),Layout.Corridors.Num(),Layout.Openings.Num(),LayoutConfig.Floors,Layout.Steps.Num());
}

SIZE_T AERProceduralMap::GetLayoutBytes() const
{
    SIZE_T Bytes=Layout.FixedPieces.GetAllocatedSize()+Layout.AlphaSpawns.GetAllocatedSize()+Layout.OmegaSpawns.GetAllocatedSize()+Layout.Rooms.GetAllocatedSize()+Layout.Corridors.GetAllocatedSize()+Layout.Walls.GetAllocatedSize()+Layout.Openings.GetAllocatedSize()+Layout.Routes.GetAllocatedSize()+Layout.Panels.GetAllocatedSize()+Layout.Steps.GetAllocatedSize()+Layout.StairSupports.GetAllocatedSize()+Layout.StairRoutes.GetAllocatedSize();
    for(const auto& Route:Layout.StairRoutes) Bytes+=Route.GetAllocatedSize();
    Bytes+=Layout.Bases.GetAllocatedSize()+Layout.ModuleKinds.GetAllocatedSize()+Layout.RoomAccessRoutes.GetAllocatedSize();
    Bytes+=Layout.TownBuildings.GetAllocatedSize()+Layout.TownCourts.GetAllocatedSize()+Layout.TownGates.GetAllocatedSize()+Layout.TownStreetNames.GetAllocatedSize()+Layout.UpperRoute.GetAllocatedSize()+Layout.LowerRoute.GetAllocatedSize();
    for(const auto& Gate:Layout.TownGates)Bytes+=Gate.Passage.GetAllocatedSize();
    for(const auto& Route:Layout.RoomAccessRoutes)Bytes+=Route.GetAllocatedSize();
    Bytes+=Layout.Houses.GetAllocatedSize()+Layout.TerrainPads.GetAllocatedSize()+Layout.Ladders.GetAllocatedSize()+Layout.DistrictTowers.GetAllocatedSize()+Layout.DistrictTowerHeights.GetAllocatedSize();
    Bytes+=Layout.UnderpassRamps.GetAllocatedSize()+Layout.UnderpassBreaches.GetAllocatedSize()+Layout.LowCovers.GetAllocatedSize()+Layout.UnderpassRoute.GetAllocatedSize()+Layout.UnderpassApproach.GetAllocatedSize();
    for(const auto* Routes:{&Layout.RooftopRoutes,&Layout.GroundRoutes,&Layout.DistrictRoutes})
    {Bytes+=Routes->GetAllocatedSize();for(const auto& Route:*Routes)Bytes+=Route.GetAllocatedSize();}
    Bytes+=Layout.CrossingRoutes.GetAllocatedSize()+Layout.ClimbPoints.GetAllocatedSize();
    Bytes+=Layout.CoverSites.GetAllocatedSize()+Layout.Containers.GetAllocatedSize()+Layout.SecondaryStructures.GetAllocatedSize()+Layout.CoverAngles.GetAllocatedSize()+Layout.WallAccessFlights.GetAllocatedSize()+Layout.WallBridges.GetAllocatedSize();
    for(const auto& Route:Layout.CrossingRoutes)Bytes+=Route.GetAllocatedSize();
    for (const auto& Route:Layout.Routes) Bytes+=Route.GetAllocatedSize();
    return Bytes;
}

void AERProceduralMap::ConfigureTemplateFloors()
{
    const auto* Template=LayoutConfig.TemplateAsset.Get();const FVector Size=Layout.ArenaSize;
    Floor->SetStaticMesh(Layout.GradedTerrain ? nullptr:BlockMesh.Get());Floor->SetRelativeLocation(FVector(0,0,-10));Floor->SetRelativeScale3D(FVector(Size.X+40,Size.Y+40,20)/100);
    Floor->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);Floor->SetVisibility(true);
    if(auto* Surface=Layout.GroundSurface ? Layout.GroundSurface:FloorSurface.Get())Floor->SetMaterial(0,Surface);
    if(Layout.GradedTerrain)
    {
        AlleyTerrain=NewObject<UDynamicMeshComponent>(this,TEXT("AlleyTerrain"));AddInstanceComponent(AlleyTerrain);
        AlleyTerrain->SetMobility(EComponentMobility::Movable);AlleyTerrain->SetNetAddressable();AlleyTerrain->SetupAttachment(RootComponent);
        AlleyTerrain->SetCanEverAffectNavigation(false);AlleyTerrain->SetDeferredCollisionUpdatesEnabled(true,false);
        AlleyTerrain->SetComplexAsSimpleCollisionEnabled(true,false);AlleyTerrain->SetCollisionProfileName(TEXT("BlockAll"));
        AlleyTerrain->SetTangentsType(EDynamicMeshComponentTangentsMode::AutoCalculated);
        AlleyTerrain->SetMaterial(0,Layout.GroundSurface);AlleyTerrain->RegisterComponent();ERProcedural::BuildAlleyTerrain(AlleyTerrain,Layout);
    }
    for(auto* Mesh:{TileFloors.Get(),WoodFloors.Get(),Ceilings.Get(),Stairs.Get()}){Mesh->ClearInstances();Mesh->SetStaticMesh(BlockMesh);}
    FixedCollision->SetStaticMesh(BlockMesh);
    FenceCollision->SetStaticMesh(BlockMesh);
    UE::Geometry::FDynamicMesh3 Metal;Metal.EnableAttributes();
    auto FlushMetal=[&]()
    {
        if(Metal.TriangleCount()==0)return;
        // Bound each cooked mesh well below Unreal's complex-collision limit.
        // Cooking is synchronous, so no client can open a gate on missing wire.
        auto* Part=NewObject<UDynamicMeshComponent>(this,*FString::Printf(TEXT("IndustrialFenceMetal_%d"),FenceMetal.Num()));
        AddInstanceComponent(Part);Part->SetMobility(EComponentMobility::Movable);Part->SetNetAddressable();
        Part->SetupAttachment(RootComponent);Part->SetCanEverAffectNavigation(false);
        Part->SetDeferredCollisionUpdatesEnabled(true,false);Part->SetComplexAsSimpleCollisionEnabled(true,false);
        Part->SetCollisionProfileName(TEXT("BlockAll"));
        Part->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore);Part->SetCollisionResponseToChannel(ECC_WorldDynamic,ECR_Ignore);
        Part->SetTangentsType(EDynamicMeshComponentTangentsMode::AutoCalculated);
        Part->SetMaterial(0,Template->CentralBuilding.SteelSurface ? Template->CentralBuilding.SteelSurface.Get():MaterialCatalog->Find(EERBlockMaterial::Metal)->Surface.Get());
        Part->RegisterComponent();Part->SetMesh(MoveTemp(Metal));Part->UpdateCollision(false);FenceMetal.Add(Part);
        Metal=UE::Geometry::FDynamicMesh3();Metal.EnableAttributes();
    };
    Ceilings->SetHiddenInGame(Template->RoofMesh!=nullptr);
    if(CeilingSurface)Ceilings->SetMaterial(0,CeilingSurface);
    if(bCeilings)Ceilings->AddInstance(FTransform(FQuat::Identity,FVector(0,0,Size.Z+10),FVector(Size.X+40,Size.Y+40,20)/100));
    for(const auto& Piece:Layout.FixedPieces)
    {
        if(Piece.Roof && !bCeilings)continue;
        if(Piece.PawnBarrier)
        {
            FenceCollision->AddInstance(Piece.Transform);
            ERIndustrialFence::Append(Piece,Metal);
            if(Metal.TriangleCount()>=50000)FlushMetal();
            continue;
        }
        UStaticMesh* Asset=Piece.Mesh ? Piece.Mesh:BlockMesh.Get();
        auto* Surface=Piece.Surface ? Piece.Surface:MaterialCatalog->Find(Piece.Material)->Surface.Get();
        auto* ExpectedSurface=Piece.Surface ? Piece.Surface:(Piece.Mesh ? Asset->GetMaterial(0):Surface);
        UInstancedStaticMeshComponent* Group=nullptr;
        const bool Visual=(!Piece.MeshCollision && Piece.Mesh) || Piece.VisualOnly || Piece.PawnBarrier;
        for(auto& Existing:FixedMeshes)if(Existing->GetStaticMesh()==Asset && Existing->ComponentHasTag(TEXT("ERTemplateRoof"))==Piece.Roof && Existing->ComponentHasTag(TEXT("ERTemplateVisual"))==Visual && Existing->GetMaterial(0)==ExpectedSurface){Group=Existing;break;}
        if(!Group)
        {
            // Groups are created in FixedPieces order, identical on host and clients:
            // pawns stand on them, so corrections carry them by name.
            Group=NewObject<UInstancedStaticMeshComponent>(this,*FString::Printf(TEXT("TemplateFixed_%d"),FixedMeshes.Num()));AddInstanceComponent(Group);Group->SetMobility(EComponentMobility::Movable);
            Group->SetNetAddressable();
            Group->SetupAttachment(RootComponent);Group->SetStaticMesh(Asset);Group->SetCollisionProfileName(Visual ? TEXT("NoCollision"):TEXT("BlockAll"));
            if(Visual)Group->ComponentTags.Add(TEXT("ERTemplateVisual"));
            if(Piece.Roof)Group->ComponentTags.Add(TEXT("ERTemplateRoof"));
            Group->SetCanEverAffectNavigation(false);
            if(Piece.Surface)for(int32 Slot=0;Slot<FMath::Max(1,Asset->GetStaticMaterials().Num());++Slot)Group->SetMaterial(Slot,Surface);
            else if(!Piece.Mesh && Surface)Group->SetMaterial(0,Surface);
            Group->RegisterComponent();FixedMeshes.Add(Group);
        }
        FTransform T=Piece.Transform;
        if(Piece.Mesh)
        {
            const FBox Box=Asset->GetBoundingBox();const FQuat MeshRotation=Piece.MeshRotation.Quaternion(),Rotation=T.GetRotation()*MeshRotation;
            const FVector Scale=MeshRotation.UnrotateVector(T.GetScale3D()*100).GetAbs()/Box.GetSize();
            T.SetRotation(Rotation);T.SetScale3D(Scale);T.AddToTranslation(-Rotation.RotateVector(Box.GetCenter()*Scale));
            if(!Piece.VisualOnly && !Piece.PawnBarrier && !Piece.MeshCollision)FixedCollision->AddInstance(Piece.Transform);
        }
        Group->AddInstance(T);
    }
    FlushMetal();
    for(const auto& Desc:Layout.Ladders)
    {
        FActorSpawnParameters Params;Params.Owner=this;
        auto* Ladder=GetWorld()->SpawnActor<AERLadder>(AERLadder::StaticClass(),Desc.Frame*GetActorTransform(),Params);
        if(Ladder){Ladder->Height=Desc.Height;Ladder->DeckHeight=Desc.DeckHeight;LadderActors.Add(Ladder);}
    }
    if(HasAuthority() && Layout.Routes.Num()>1 && (Template->Archetype==EERMapArchetype::CentralBuilding || Template->Archetype==EERMapArchetype::MiddleEasternAlleys))
    {
        // Crossing rules have different route lengths. Never leave saved
        // placeholder points at the origin or feed bots obsolete waypoints.
        TSet<int32> Existing;
        for(TActorIterator<AERBotPatrolPoint> It(GetWorld());It;++It)
        {
            if(!Layout.Routes[1].IsValidIndex(It->RouteIndex) || Existing.Contains(It->RouteIndex)){It->Destroy();continue;}
            Existing.Add(It->RouteIndex);
            It->GetRootComponent()->SetMobility(EComponentMobility::Movable);
            It->SetActorLocation(GetActorTransform().TransformPosition(Layout.Routes[1][It->RouteIndex]));
            It->AcceptanceRadius=Layout.TownGates.IsEmpty() ? 20:10;
        }
        for(int32 I=0;I<Layout.Routes[1].Num();++I)if(!Existing.Contains(I))
        {
            auto* Point=GetWorld()->SpawnActor<AERBotPatrolPoint>(GetActorTransform().TransformPosition(Layout.Routes[1][I]),FRotator::ZeroRotator);
            Point->RouteIndex=I;Point->AcceptanceRadius=Layout.TownGates.IsEmpty() ? 20:10;
        }
    }
    else if(HasAuthority() && Layout.Routes.Num()>1)
        for(TActorIterator<AERBotPatrolPoint> It(GetWorld());It;++It)
            if(Layout.Routes[1].IsValidIndex(It->RouteIndex))
            {
                It->GetRootComponent()->SetMobility(EComponentMobility::Movable);
                It->SetActorLocation(GetActorTransform().TransformPosition(Layout.Routes[1][It->RouteIndex]));
                // Tight doglegs must be reached before turning; the ordinary
                // 90cm patrol radius would cut across a wooden leaf.
                if(!Layout.TownGates.IsEmpty())It->AcceptanceRadius=10;
            }
}
uint32 AERProceduralMap::GetTemplateGeometryDigest() const
{
    if(!LayoutConfig.TemplateAsset)return 0;
    uint32 H=GetTypeHash(LayoutConfig.TemplateAsset->Revision);
    H=HashCombine(H,FCrc::StrCrc32(*LayoutConfig.TemplateAsset->TemplateId));
    H=HashCombine(H,GetTypeHash(uint8(LayoutConfig.TemplateAsset->Archetype)));
    if(Layout.GroundSurface)H=HashCombine(H,FCrc::StrCrc32(*Layout.GroundSurface->GetPathName()));
    auto MixVector=[&](FVector V){for(int32 I=0;I<3;++I)H=HashCombine(H,GetTypeHash(FMath::RoundToInt(V[I]*100)));};
    MixVector(Layout.ArenaSize);H=HashCombine(H,GetTypeHash(bCeilings));H=HashCombine(H,GetTypeHash(uint8(LayoutConfig.TemplateAsset->Symmetry)));
    for(const auto& P:Layout.FixedPieces)
    {
        H=HashCombine(H,GetTypeHash(P.PawnBarrier));
        H=HashCombine(H,GetTypeHash(P.MeshCollision));
        if(P.PawnBarrier)H=HashCombine(H,ERIndustrialFence::GeometryVersion);
        MixVector(P.Transform.GetLocation());MixVector(P.Transform.GetScale3D());MixVector(FVector(P.MeshRotation.Pitch,P.MeshRotation.Yaw,P.MeshRotation.Roll));H=HashCombine(H,GetTypeHash(P.Roof));H=HashCombine(H,GetTypeHash(P.VisualOnly));H=HashCombine(H,GetTypeHash(uint8(P.Material)));
        MixVector(P.Transform.GetRotation().GetForwardVector());
        if(P.Surface)H=HashCombine(H,FCrc::StrCrc32(*P.Surface->GetPathName()));
        H=HashCombine(H,FCrc::StrCrc32(P.Mesh ? *P.Mesh->GetPathName():TEXT("cube")));
        if(P.Mesh && P.Mesh->GetBodySetup())H=HashCombine(H,GetTypeHash(P.Mesh->GetBodySetup()->BodySetupGuid));
    }
    for(const auto* Starts:{&Layout.AlphaSpawns,&Layout.OmegaSpawns})for(const auto& P:*Starts){MixVector(P.GetLocation());MixVector(P.GetRotation().GetForwardVector());}
    for(const auto& Route:Layout.Routes)for(const auto& P:Route)MixVector(P);
    if(LayoutConfig.TemplateAsset->Town.Enabled)
    {
        H=HashCombine(H,GetTypeHash(Layout.GradedTerrain));
        H=HashCombine(H,GetTypeHash(Layout.SeededDistrict));
        H=HashCombine(H,GetTypeHash(Layout.DistrictAccesses));H=HashCombine(H,GetTypeHash(Layout.DistrictBends));
        H=HashCombine(H,GetTypeHash(Layout.DistrictWallCount));
        H=HashCombine(H,GetTypeHash(Layout.DistrictWallOffset));H=HashCombine(H,GetTypeHash(Layout.DistrictOpenHalfDepth));
        H=HashCombine(H,GetTypeHash(Layout.UnderpassAxis));H=HashCombine(H,GetTypeHash(Layout.CentralLowCovers));
        for(const auto& R:Layout.UnderpassRamps){MixVector(R.Bounds.Min);MixVector(R.Bounds.Max);MixVector(R.Entry);MixVector(R.Exit);}
        for(const auto* Bounds:{&Layout.UnderpassBreaches,&Layout.LowCovers})for(const auto& P:*Bounds){MixVector(P.Min);MixVector(P.Max);}
        for(const auto& P:Layout.UnderpassRoute)MixVector(P);
        for(const auto& Tower:Layout.DistrictTowers){MixVector(Tower.GetLocation());MixVector(Tower.GetRotation().GetForwardVector());}
        for(float Height:Layout.DistrictTowerHeights)H=HashCombine(H,GetTypeHash(Height));
        for(const auto& Route:Layout.DistrictRoutes)for(const auto& P:Route)MixVector(P);
        for(const auto& P:Layout.TerrainPads){MixVector(P.Min);MixVector(P.Max);}
        for(const auto* Routes:{&Layout.RooftopRoutes,&Layout.GroundRoutes})for(const auto& Route:*Routes)for(const auto& P:Route)MixVector(P);
        MixVector(Layout.TownCrossing);
        for(const auto* Paths:{&Layout.RoomAccessRoutes,&Layout.StairRoutes})for(const auto& Route:*Paths)for(const auto& P:Route)MixVector(P);
        for(const auto& P:Layout.UpperRoute)MixVector(P);
        for(const auto& G:Layout.TownGates){MixVector(G.Frame.GetLocation());MixVector(FVector(G.Gap,G.Width,G.Angle));for(const auto& P:G.Passage)MixVector(P);}
    }
    return H;
}
