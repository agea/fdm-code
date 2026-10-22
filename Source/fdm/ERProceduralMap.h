#pragma once
#include "CoreMinimal.h"
#include "ERBlockRoom.h"
#include "ERMapTemplate.h"
#include "ERProceduralMap.generated.h"

struct FERLayoutRect { FVector2D Min, Max; int32 Room = INDEX_NONE; float Z=0; int32 Level=0; };
struct FERLayoutWall { FVector Start; FVector Direction; float Length = 0; EERBlockMaterial Material; };
struct FERLayoutOpening { int32 Wall = INDEX_NONE; float Along = 0, Width = 0, Sill = 0, Height = 0; bool bWindow = false; };
struct FERLayoutFixedPiece { FTransform Transform; EERBlockMaterial Material; class UStaticMesh* Mesh=nullptr; FRotator MeshRotation=FRotator::ZeroRotator; bool Roof=false; bool VisualOnly=false; class UMaterialInterface* Surface=nullptr; bool PawnBarrier=false; bool MeshCollision=false; };
struct FERLayoutHouse { FTransform Frame; class UStaticMesh* Mesh=nullptr; bool RoofAccess=false; };
struct FERLayoutTownGate { FTransform Frame; float Gap=110,Width=300,Angle=25; TArray<FVector> Passage; };
struct FERLayoutLadder { FTransform Frame; float Height=430,DeckHeight=320; };
struct FERAlleyRamp { FBox Bounds; FVector Entry,Exit; };
struct FERLayoutCover
{
    FTransform Frame;
    FVector Size,Hide,Entry,SightFrom,SightTo;
    EERBlockMaterial Material=EERBlockMaterial::Brick;
    float HalfHeight=92.5f;
};
struct FERLayoutContainer { FTransform Frame; FVector Size; };
struct FERLayoutSecondary { FTransform Frame; FVector Size; int32 Floors=1; EERBlockMaterial Material=EERBlockMaterial::Brick; bool Tower=false; bool LadderAccess=false; };
struct FERTemplateBase
{
    FTransform Frame;
    FVector Size;
    float StairX=0,StairStartY=0,RoofZ=0;
    int32 StepCount=0;
    FVector Point(FVector Local) const { return Frame.TransformPosition(Local); }
};
struct FERProceduralLayout
{
    TArray<FERLayoutRect> Rooms, Corridors;
    TArray<FERLayoutWall> Walls;
    TArray<FERLayoutOpening> Openings;
    TArray<TArray<FVector>> Routes; // Room centre -> branch -> common hall centre.
    // Destructible 5cm-cell panels (wall constructions, openings as holes).
    TArray<FERCellPanelDesc> Panels;
    TArray<FTransform> Steps,StairSupports;
    TArray<TArray<FVector>> StairRoutes;
    TArray<FERLayoutFixedPiece> FixedPieces;
    TArray<FTransform> AlphaSpawns, OmegaSpawns;
    FVector ArenaSize=FVector::ZeroVector;
    class UMaterialInterface* GroundSurface=nullptr;
    TArray<FERTemplateBase> Bases;
    TArray<EERTemplateModule> ModuleKinds;
    TArray<TArray<FVector>> RoomAccessRoutes;
    TArray<FBox> TownBuildings,TownCourts;
    TArray<FName> TownStreetNames;
    TArray<FERLayoutTownGate> TownGates;
    TArray<FVector> UpperRoute,LowerRoute;
    TArray<FERLayoutHouse> Houses;
    TArray<TArray<FVector>> RooftopRoutes,GroundRoutes;
    TArray<FBox> TerrainPads;
    bool GradedTerrain=false;
    bool SeededDistrict=false;
    int32 DistrictAccesses=0,DistrictBends=0;
    int32 DistrictWallCount=0;
    float DistrictWallOffset=0,DistrictOpenHalfDepth=0;
    int32 UnderpassAxis=0,CentralLowCovers=0; // 1 base-to-base; 2 west-to-east.
    TArray<FERAlleyRamp> UnderpassRamps;
    TArray<FBox> UnderpassBreaches,LowCovers;
    TArray<FVector> UnderpassRoute,UnderpassApproach;
    TArray<FTransform> DistrictTowers;
    TArray<float> DistrictTowerHeights;
    TArray<TArray<FVector>> DistrictRoutes;
    FVector TownCrossing=FVector::ZeroVector;
    EERCrossingRule CrossingRule=EERCrossingRule::OpenYard;
    int32 BuildingFloors=0;
    FVector CentralBuildingSize=FVector::ZeroVector;
    TArray<TArray<FVector>> CrossingRoutes;
    TArray<FVector> ClimbPoints;
    TArray<FERLayoutContainer> Containers;
    TArray<FERLayoutCover> CoverSites;
    TArray<FERLayoutLadder> Ladders;
    TArray<FERLayoutSecondary> SecondaryStructures;
    TArray<int32> WallAccessFlights;
    TArray<float> CoverAngles;
    TArray<FVector> WallBridges;
    FVector Spawn = FVector::ZeroVector;
};
namespace ERProcedural
{
    bool Build(const FERProceduralConfig& Config, const FERBlockRoomConfig& Blocks, FERProceduralLayout& Out, const FERCellSettings& Cells = FERCellSettings());
    // Axis-aligned destructible box as cell panels: massive slabs, boards in thin
    // slots, 2cm shells (four sides and a top) for volumetric thin-material pieces.
    void AddPieceCells(int32 Id, EERBlockMaterial Material, const FBox& Box, bool bAnchoredBase, const FERCellSettings& Cells, TArray<FERCellPanelDesc>& Out);
    bool BuildTemplateVariants(const class UERMapTemplate& Template,int32 Seed,TArray<struct FERMapTemplatePiece>& Pieces,FERProceduralLayout& Layout,TArray<FVector>& Patrol);
    bool BuildDustTown(const class UERMapTemplate& Template,int32 Seed,TArray<struct FERMapTemplatePiece>& Pieces,FERProceduralLayout& Layout,TArray<FVector>& Patrol);
    bool BuildAlleyDistrict(const class UERMapTemplate& Template,int32 Seed,TArray<struct FERMapTemplatePiece>& Pieces,FERProceduralLayout& Layout,TArray<FVector>& Patrol);
    bool PlanAlleyUnderpass(const UERMapTemplate& Template,int32 Seed,FVector Base,TArray<FERMapTemplatePiece>& Pieces,FERProceduralLayout& Layout);
    bool BuildAlleyLowCover(const UERMapTemplate& Template,int32 Seed,TArray<FERMapTemplatePiece>& Pieces,FERProceduralLayout& Layout);
    float AlleyRampHeight(const FERAlleyRamp& Ramp,FVector2D Position);
    float AlleyGroundHeight(const FERProceduralLayout& Layout,FVector2D Position);
    float AlleyBaseHeight(FVector Arena,FVector2D Position,bool Balanced=false);
    void BuildAlleyTerrain(class UDynamicMeshComponent* Component,const FERProceduralLayout& Layout);
    bool FindAlleyInteriorRoute(class UWorld* World,const FERProceduralLayout& Layout,TArray<FVector>& Route);
    bool BuildCentralBuilding(const class UERMapTemplate& Template,int32 Seed,TArray<struct FERMapTemplatePiece>& Pieces,FERProceduralLayout& Layout,TArray<FVector>& Patrol,int32 CrossingOverride=0);
}

// Reuses the lab's 5cm cells, hitscan, explosion, support and replication.
UCLASS(Blueprintable)
class FDM_API AERProceduralMap : public AERBlockRoom
{
    GENERATED_BODY()
public:
    AERProceduralMap();
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Procedural") FERProceduralConfig LayoutConfig;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Procedural") TObjectPtr<class UMaterialInterface> WoodFloorSurface;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Procedural") TObjectPtr<class UMaterialInterface> CeilingSurface;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Procedural") TObjectPtr<class UMaterialInterface> ExteriorFloorSurface;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Procedural") bool bCeilings = true;
    uint32 GetTemplateGeometryDigest() const;
    const FERProceduralLayout& GetLayout() const { return Layout; }
    virtual FVector GetSpawnLocation() const override { return GetActorTransform().TransformPosition(Layout.Spawn); }
protected:
    virtual bool GenerateLogicalWorld() override;
    virtual void ConfigureFloors() override;
    virtual FString GetLayoutStatistics() const override;
    virtual SIZE_T GetLayoutBytes() const override;
private:
    FERProceduralLayout Layout;
    UPROPERTY() TObjectPtr<class UInstancedStaticMeshComponent> TileFloors;
    UPROPERTY() TObjectPtr<class UInstancedStaticMeshComponent> WoodFloors;
    UPROPERTY() TObjectPtr<class UInstancedStaticMeshComponent> Ceilings;
    UPROPERTY() TObjectPtr<class UInstancedStaticMeshComponent> Stairs;
    UPROPERTY() TArray<TObjectPtr<class UInstancedStaticMeshComponent>> FixedMeshes;
    UPROPERTY(Transient) TArray<TObjectPtr<class AERLadder>> LadderActors;
    UPROPERTY() TObjectPtr<class UInstancedStaticMeshComponent> FixedCollision;
    UPROPERTY() TObjectPtr<class UInstancedStaticMeshComponent> FenceCollision;
    UPROPERTY() TArray<TObjectPtr<class UDynamicMeshComponent>> FenceMetal;
    UPROPERTY() TObjectPtr<class UDynamicMeshComponent> AlleyTerrain;
    void ConfigureTemplateFloors();
};
