#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataAsset.h"
#include "ERBlockWorld.generated.h"

UENUM(BlueprintType)
enum class EERBlockMaterial : uint8 { Drywall, Brick, Concrete, Wood, Stone, Metal, Glass };
// Gameplay preset, not physical data. HP are per logical 5cm cell.
USTRUCT(BlueprintType)
struct FERBlockMaterialProperties
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EERBlockMaterial Type = EERBlockMaterial::Drywall;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1")) float MaxHealth = 20;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) float BulletDamageScale = 1;
    // Penetration energy stays independent from structural damage.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) float EntryEnergyCost = 5;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) float EnergyCostPerCm = .5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) float ExplosionDamageScale = 1;
    // Fraction of blast exposure passed to cells behind this one. A prototype
    // coefficient to tune, unrelated to the damage multipliers above.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="1")) float ExplosionTransmission = .25f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<class UMaterialInterface> ImpactDecal;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<class UNiagaraSystem> ImpactSystem;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<class UMaterialInterface> Surface;
};

// Discretisation the cell HP were tuned for. Cell worlds whose settings differ
// refuse to generate until HP are retuned and this stamp is updated with them.
USTRUCT(BlueprintType)
struct FERCellCalibration
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float CellSize = 5;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float WoodDepth = 2;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float DrywallBoard = 1.25f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float SheetThickness = .2f;
    // ExplosionTransmission is the share passed by this many cm of intact
    // material; cells attenuate by the path length they actually cross.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1")) float TransmissionReferenceCm = 20;
};

UCLASS(BlueprintType)
class FDM_API UERBlockMaterialCatalog : public UDataAsset
{
    GENERATED_BODY()
public:
    UERBlockMaterialCatalog();
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FERBlockMaterialProperties> Materials;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FERCellCalibration CellCalibration;
    const FERBlockMaterialProperties* Find(EERBlockMaterial Type) const;
    bool IsValidCatalog() const;
};

USTRUCT(BlueprintType)
struct FERBlockRoomConfig
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Seed = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="100")) float InteriorSize = 500;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="100")) float Height = 300;
    // Y: wall slot thickness in centimetres (X/Z are the legacy block grid of the
    // procedural generators: openings and room sizes stay on 50x25cm steps).
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector BlockSize = FVector(50, 20, 25);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="100")) float ChunkSize = 400;
};

// Legacy subobject of saved destruction maps. Live state belongs to the 5cm
// cell world of AERBlockRoom; this component stores nothing.
UCLASS(ClassGroup=(EclipseRecoil))
class FDM_API UERBlockWorld : public UActorComponent
{
    GENERATED_BODY()
public:
    UERBlockWorld();
};
