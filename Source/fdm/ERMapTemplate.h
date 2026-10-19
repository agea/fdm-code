#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ERBlockWorld.h"
#include "ERMapTemplate.generated.h"

UENUM(BlueprintType)
enum class EERTemplateSymmetry : uint8 { None, HalfTurn };

UENUM(BlueprintType)
enum class EERMapArchetype : uint8 { LegacyAssembly, CentralBuilding };

UENUM(BlueprintType)
enum class EERCrossingRule : uint8 { Seeded, OpenYard, ClimbableWall, FencedCrossings };

// Gameplay rules and industrial art are independent from any particular seed.
USTRUCT(BlueprintType)
struct FERCentralBuildingRecipe
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector2D ArenaMin=FVector2D(6400,8400);
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector2D ArenaMax=FVector2D(8000,10000);
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector2D BuildingWidth=FVector2D(1800,2400);
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector2D BuildingDepth=FVector2D(2200,3000);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1",ClampMax="3")) int32 MinFloors=1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1",ClampMax="3")) int32 MaxFloors=3;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<class UStaticMesh> FenceMesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<class UStaticMesh> ContainerMesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<class UStaticMesh> TankMesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<class UStaticMesh> BarrelMesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<class UMaterialInterface> DeckSurface;
    // Visual palette only: steel calibration and container reinforcement are unchanged.
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<class UMaterialInterface> SteelSurface;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<TObjectPtr<class UMaterialInterface>> ContainerSurfaces;
};

UENUM(BlueprintType)
enum class EERTemplateModule : uint8 { Room, LCover, UCover, StaggeredWalls, Cargo, Tanks, Crates };

// Reference-plan coordinates are percentages, X east, Y south. Shared axis
// warping changes parcels/lanes without changing the named route graph.
USTRUCT(BlueprintType)
struct FERPlanStreet
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Name;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FVector2D> Points;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector2D Width=FVector2D(300,400);
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool Elevated=false;
};
USTRUCT(BlueprintType)
struct FERPlanCourt
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Name;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector2D Center=FVector2D::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector2D Size=FVector2D(12,12);
};
USTRUCT(BlueprintType)
struct FERPlanGate
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Street;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector2D Position=FVector2D::ZeroVector;
};
USTRUCT(BlueprintType)
struct FERDustTownVariation
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool Enabled=false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector2D ArenaMin=FVector2D(7600,8400);
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector2D ArenaMax=FVector2D(8600,9600);
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float AxisWarp=2;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector2D BuildingHeight=FVector2D(450,800);
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector2D DoorGap=FVector2D(110,130);
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FERPlanStreet> Streets;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FERPlanCourt> Courts;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FERPlanGate> Gates;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FVector2D> Patrol;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector2D Crossing=FVector2D(68,30);
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<class UMaterialInterface> WallSurface;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<class UMaterialInterface> RoofSurface;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<class UMaterialInterface> RoadSurface;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<class UMaterialInterface> WoodSurface;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<class UMaterialInterface> IronSurface;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<class UStaticMesh> ArchMesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<class UStaticMesh> LeftDoorMesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<class UStaticMesh> RightDoorMesh;
};

// Ranges and art palette belong to the template, never to runtime file paths.
USTRUCT(BlueprintType)
struct FERMapTemplateVariation
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool Enabled=false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector2D ArenaMin=FVector2D(3600,7000);
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector2D ArenaMax=FVector2D(4800,9000);
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector2D BaseWidth=FVector2D(1000,1400);
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector2D BaseDepth=FVector2D(850,1050);
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MinRows=2;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MaxRows=3;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<EERTemplateModule> Modules={EERTemplateModule::Room,EERTemplateModule::LCover,EERTemplateModule::UCover,EERTemplateModule::StaggeredWalls,EERTemplateModule::Cargo,EERTemplateModule::Tanks,EERTemplateModule::Crates};
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<class UStaticMesh> CargoMesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<class UStaticMesh> TankMesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<class UStaticMesh> BarrelMesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<class UStaticMesh> ColumnMesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<class UStaticMesh> BeamMesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<class UStaticMesh> GuardMesh;
};

// Authored assembly boxes, in centimetres. A shared group moves a complete
// bunker/prop together; different groups vary independently on a 50cm grid.
USTRUCT(BlueprintType)
struct FERMapTemplatePiece
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector Center=FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector Size=FVector(100);
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector2D Jitter=FVector2D::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Group=0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EERBlockMaterial Material=EERBlockMaterial::Wood;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool Destructible=true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool Mirrored=false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool Anchored=false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1",ClampMax="16")) float Strength=1;
    // Fixed boxes or horizontal destructible construction panels, never fitted art.
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool WalkableSurface=false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FRotator MeshRotation=FRotator::ZeroRotator;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float RotationYaw=0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool VisualOnly=false;
    // Permanent movement barrier; visible metal has separate perforated collision.
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool PawnBarrier=false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<class UMaterialInterface> FixedSurface;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<class UMaterialInterface> CellSurface;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<class UMaterialInterface> MirroredCellSurface;
    // Fixed industrial art is fitted to Size; the authored box owns collision.
    // Destructible geometry always uses the project's block mesh/catalog.
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<class UStaticMesh> FixedMesh;
};

UCLASS(BlueprintType)
class FDM_API UERMapTemplate : public UDataAsset
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Template") bool ValidateLayout(int32 Seed=42) const;
    // PCG/PCGEx authoring receives the same validated recipe as runtime collision.
    UFUNCTION(BlueprintCallable, Category="Template|PCG") class UPCGDataAsset* CreatePCGPreview(int32 Seed=42) const;
    UFUNCTION(BlueprintCallable, Category="Template|PCG") class UPCGDataAsset* SavePCGPreview(const FString& PackageName,int32 Seed=42) const;
    UFUNCTION(BlueprintCallable, Category="Template|Maintenance") static bool DeleteArchivedProceduralLab();
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EERMapArchetype Archetype=EERMapArchetype::LegacyAssembly;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FERCentralBuildingRecipe CentralBuilding;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString TemplateId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString SourceReference;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Revision=1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EERTemplateSymmetry Symmetry=EERTemplateSymmetry::None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<class UStaticMesh> ShellWallMesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<class UStaticMesh> RoofMesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector ArenaSize=FVector(4100,7800,1050);
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FERMapTemplateVariation Variation;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FERDustTownVariation Town;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FERMapTemplatePiece> Pieces;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FTransform> AlphaSpawns;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FTransform> OmegaSpawns;
    // Pawn-centre route waypoints; validation checks spawn-to-route access.
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FVector> PatrolRoute;
};
