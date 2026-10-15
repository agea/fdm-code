#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataAsset.h"
#include "ERBlockProfile.h"
#include "ERBlockWorld.generated.h"

UENUM(BlueprintType)
enum class EERBlockMaterial : uint8 { Drywall, Brick, Concrete, Wood, Stone, Metal, Glass };
UENUM(BlueprintType)
enum class EERBlockState : uint8 { Intact, Damaged, Destroyed, Detached };

// Wall-local coordinates remain stable when rendering instances are reordered.
USTRUCT(BlueprintType)
struct FERBlockId
{
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 Wall = INDEX_NONE;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 Column = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 Row = 0;
    bool operator==(const FERBlockId& Other) const { return Wall == Other.Wall && Column == Other.Column && Row == Other.Row; }
    friend uint32 GetTypeHash(const FERBlockId& Id) { return HashCombine(HashCombine(::GetTypeHash(Id.Wall), ::GetTypeHash(Id.Column)), ::GetTypeHash(Id.Row)); }
    FString ToString() const { return FString::Printf(TEXT("W%d:C%d:R%d"), Wall, Column, Row); }
};

USTRUCT(BlueprintType)
struct FERBlockMaterialProperties
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EERBlockMaterial Type = EERBlockMaterial::Drywall;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1")) float MaxHealth = 30;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) float BulletDamageScale = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) float EntryEnergyCost = 5;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) float EnergyCostPerCm = .5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) float ExplosionDamageScale = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<class UMaterialInterface> ImpactDecal;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<class UNiagaraSystem> ImpactSystem;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<class UMaterialInterface> Surface;
};

UCLASS(BlueprintType)
class FDM_API UERBlockMaterialCatalog : public UDataAsset
{
    GENERATED_BODY()
public:
    UERBlockMaterialCatalog();
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FERBlockMaterialProperties> Materials;
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
    // X: width along a wall, Y: thickness, Z: course height, in centimetres.
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector BlockSize = FVector(50, 20, 25);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="100")) float ChunkSize = 400;
};

USTRUCT(BlueprintType)
struct FERBlockRecord
{
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FERBlockId Id;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) EERBlockMaterial Material = EERBlockMaterial::Drywall;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FTransform LocalTransform;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FIntVector Chunk = FIntVector::ZeroValue;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float Health = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) EERBlockState State = EERBlockState::Intact;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bIndestructible = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bStructuralAnchor = false;
    bool IsAttached() const { return State==EERBlockState::Intact || State==EERBlockState::Damaged; }
};

// Logical storage has no dependency on components, instance indices or traces.
UCLASS(ClassGroup=(EclipseRecoil), meta=(BlueprintSpawnableComponent))
class FDM_API UERBlockWorld : public UActorComponent
{
    GENERATED_BODY()
public:
    UERBlockWorld();
    bool GenerateRoom(const FERBlockRoomConfig& Config, const UERBlockMaterialCatalog* Catalog);
    // Validates the complete replacement before touching live state.
    bool LoadLayout(const TArray<FERBlockRecord>& Records, float ChunkSize, const UERBlockMaterialCatalog* Catalog);
    const TArray<FERBlockRecord>& GetBlocks() const { return Blocks; }
    const FERBlockRecord* Find(const FERBlockId& Id) const;
    bool Damage(const FERBlockId& Id, float Amount);
    bool DestroyBlock(const FERBlockId& Id);
    bool DetachBlock(const FERBlockId& Id);
    int32 GetBlockIndex(const FERBlockId& Id) const {const auto* I=Lookup.Find(Id);return I ? *I : INDEX_NONE;}
    bool ValidateNetworkState(int32 Index,uint8 State,float Health,const UERBlockMaterialCatalog* Catalog) const;
    void ApplyNetworkState(int32 Index,uint8 State,float Health);
    int32 GetDetachedCount() const { return DetachedCount; }
    int32 GetRemovedCount() const { return DestroyedCount+DetachedCount; }
    uint64 GetDamageRevision() const { return DamageRevision; }
    TArray<FERBlockId> QuerySphere(const FVector& LocalOrigin, float LocalRadius) const;
    SIZE_T GetStorageBytes() const;
    mutable FERBlockQueryProfile LastQuery;
    int32 GetDestroyedCount() const { return DestroyedCount; }
private:
    UPROPERTY() TArray<FERBlockRecord> Blocks;
    TMap<FERBlockId, int32> Lookup;
    TMap<FIntVector, TArray<int32>> SpatialIndex;
    float SpatialChunkSize = 400;
    float MaxHalfDiagonal = 0;
    int32 DestroyedCount = 0,DetachedCount=0;
    uint64 DamageRevision=0;
};
