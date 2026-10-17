#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERBlockWorld.h"
#include "ERBlockCombat.h"
#include "InstanceDataTypes.h"
#include "ERBlockSupport.h"
#include "ERCellWorld.h"
#include "ERProceduralConfig.h"
#include "ERBlockRoom.generated.h"

USTRUCT(BlueprintType)
struct FERBlockCollapseSettings
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool Enabled=true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool GroundAnchors=true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float GroundZ=0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1",ClampMax="16384")) int32 WorkPerStep=4096;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.01",ClampMax="4")) float GraphTimeMs=1.5;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1",ClampMax="256")) int32 RemovalsPerStep=128;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0",ClampMax="64")) int32 MaxDebris=32;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0",ClampMax="8")) int32 DebrisPerStep=4;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1",ClampMax="20")) float DebrisLifetime=8;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.1",ClampMax="20")) float DebrisSettleAfter=2;
};

USTRUCT()
struct FERBlockNetworkDescriptor
{
    GENERATED_BODY()
    UPROPERTY() uint32 Epoch=0;
    UPROPERTY() uint32 Protocol=6;
    UPROPERTY() uint32 LayoutDigest=0;
    UPROPERTY() uint32 Sequence=0;
    UPROPERTY() int32 BlockCount=0;
    UPROPERTY() FERBlockRoomConfig Blocks;
    UPROPERTY() FERProceduralConfig Layout;
    UPROPERTY() bool Procedural=false;
    UPROPERTY() bool Ceilings=true;
    UPROPERTY() TArray<uint32> PageVersions;
    // Protocol 7: 5cm cells. BlockCount holds the cell count, PageVersions the shard versions.
    UPROPERTY() bool Cells=false;
    UPROPERTY() FERCellSettings CellSettings;
};

// Key lookup changes no enumeration order: groups still follow the first block
// encountered in deterministic logical storage.
struct FERBlockGroupKey
{
    FIntVector Chunk;
    EERBlockMaterial Material;
    bool operator==(const FERBlockGroupKey& Other) const { return Chunk==Other.Chunk && Material==Other.Material; }
    friend uint32 GetTypeHash(const FERBlockGroupKey& Key) { return HashCombine(GetTypeHash(Key.Chunk),::GetTypeHash(uint8(Key.Material))); }
};

USTRUCT()
struct FERBlockRenderGroup
{
    GENERATED_BODY()
    UPROPERTY() TObjectPtr<class UInstancedStaticMeshComponent> Mesh;
    UPROPERTY() FIntVector Chunk = FIntVector::ZeroValue;
    UPROPERTY() EERBlockMaterial Material = EERBlockMaterial::Drywall;
    UPROPERTY() TArray<FERBlockId> Members;
    UPROPERTY() TArray<FERBlockId> InstanceIds;
};
struct FERBlockInstanceLocation { int32 Group = INDEX_NONE; int32 Instance = INDEX_NONE; FPrimitiveInstanceId Handle; };

UCLASS(Blueprintable)
class FDM_API AERBlockRoom : public AActor
{
    GENERATED_BODY()
public:
    AERBlockRoom();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UERBlockWorld> BlockWorld;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<class UStaticMeshComponent> Floor;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Blocks") FERBlockRoomConfig Config;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Blocks") TObjectPtr<UERBlockMaterialCatalog> MaterialCatalog;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Blocks") TObjectPtr<class UStaticMesh> BlockMesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Blocks") TObjectPtr<class UMaterialInterface> FloorSurface;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Blocks|Effects") TSubclassOf<class AERGrenadeExplosion> ExplosionEffectClass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Blocks|Effects", meta=(ClampMin="0", ClampMax="64")) int32 MaxImpactEffects = 24;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Blocks|Effects", meta=(ClampMin="0")) float EffectDistance = 1500;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Blocks|Collapse") FERBlockCollapseSettings Collapse;
    // Prototype: the lab walls as 5cm cells (chunk meshes + box collision) instead of blocks.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Blocks|Cells") bool bCellMode=false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Blocks|Cells") FERCellSettings CellSettings;
    UFUNCTION(BlueprintCallable, Category="Blocks") bool Regenerate();
    void RequestSupportCheck();
    void StepSupport();
    bool IsSupportPending() const { return bSupportPending; }
    void GetDebrisCounts(int32& Alive,int32& Simulating) const;
    double LastSupportMs=0, PeakSupportMs=0;
    int32 SupportSteps=0, LastUnsupported=0, LastUnsupportedComponents=0;
    int32 LastSupportWork=0, LastDetached=0, LastSpawned=0;
    double GetSupportBuildMs() const { return SupportGraph.BuildMs; }
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual float TakeDamage(float Amount, const FDamageEvent& Event, AController* Instigator, AActor* Causer) override;
    bool ResolveHit(const FHitResult& Hit, FERBlockId& Id) const;
    bool IsCellMode() const { return bCellMode; }
    const FERCellWorld& GetCellWorld() const { return CellWorld; }
    bool ResolveCell(const FHitResult& Hit, FERCellId& Id) const;
    // Blocks or cells under a hit, for point damage and console tools.
    bool ApplyHitDamage(const FHitResult& Hit, float Amount);
    FERBlockBatchResult ApplyCellBatch(const TMap<FERCellId, float>& Damage);
    struct FERCellShotStep { bool bStop=true, bLimited=false; int32 Cells=0, Runs=0; FVector Exit=FVector::ZeroVector; };
    // Voxel DDA through the hit panel: entry cost once per material run, energy per cm,
    // each crossed cell damaged with the residual energy at its entry.
    FERCellShotStep ShootCells(const FHitResult& Hit, const FVector& Cursor, const FVector& Ray, double Range, float Structural,
        const FERBlockShotSettings& Settings, float& Energy, int32 RunsLeft);
    // Pre-event snapshot only: never mutates. Query must ignore every cell chunk component.
    void CollectCellExplosion(const FVector& Origin, const FERBlockExplosionProfile& Profile, const FCollisionQueryParams& Query,
        TMap<FERCellId, float>& Out, FERBlockBatchResult& Result) const;
    void GetCellComponents(TArray<UPrimitiveComponent*>& Out) const;
    bool ValidateCells() const;
    FString DescribeCell(const FERCellId& Id) const;
    void SpawnCellImpact(EERBlockMaterial Material, const FVector& Position, const FVector& Normal);
    void ReceiveCellShard(class AERCellNetShard* Shard);
    bool ApplyBlockDamage(const FERBlockId& Id, float Amount, bool bDestroy = false);
    FERBlockBatchResult ApplyDamageBatch(const TArray<FERBlockDamageRequest>& Requests);
    static FERBlockBatchResult ApplyWorldExplosion(UWorld* World, const FVector& Origin, float Radius, float Damage, AActor* IgnoredActor = nullptr);
    // Structural blast: F(d) * profile * material multiplier * exposure per cell.
    static FERBlockBatchResult ApplyWorldExplosion(UWorld* World, const FVector& Origin, const FERBlockExplosionProfile& Profile, AActor* IgnoredActor = nullptr);
    FTransform GetBlockTransform(const FERBlockId& Id) const;
    void SpawnBlockImpact(const FERBlockId& Id, const FVector& Position, const FVector& Normal);
    void SpawnExplosionEffect(const FVector& Origin);
    bool IsStandaloneLab() const; // Standalone compatibility helper.
    bool CanMutateBlocks() const;
    bool IsMapReady() const { return HasAuthority() || bMapReady; }
    bool IsSnapshotCurrent() const { return HasAuthority() || (bMapReady && AppliedSequence==Network.Sequence); }
    uint32 GetMapEpoch() const { return Network.Epoch; }
    uint32 GetMapSequence() const { return Network.Sequence; }
    uint32 GetAppliedMapSequence() const {return AppliedSequence;}
    int32 GetNetworkPageCount() const {return Network.PageVersions.Num();}
    uint32 GetStateDigest() const;
    uint32 GetLayoutDigest() const;
    void ReceiveNetworkPage(class AERBlockNetPage* Page);
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    bool ValidateMappings() const;
    int32 GetGroupCount() const { return Groups.Num(); }
    int32 GetChunkCount() const;
    int32 GetInstanceCount() const { return Instances.Num(); }
    FERBlockOperationProfile GenerationProfile, UpdateProfile;
    FERBlockMemoryProfile GetMemoryProfile() const;
    void GetEffectCounts(int32& Decals,int32& Emitters,int32& Bursts) const;
    float GenerationMs = 0;
    float LastUpdateMs = 0;
    FString LastHit = TEXT("none");
    FString LastCombat = TEXT("none");
    int32 LastGroupsRebuilt = 0;
    FString GetStatistics() const;
    virtual FVector GetSpawnLocation() const { return GetActorTransform().TransformPosition(FVector(0, 0, 100)); }
protected:
    virtual bool GenerateLogicalWorld();
    virtual void ConfigureFloors();
    virtual FString GetLayoutStatistics() const;
    virtual SIZE_T GetLayoutBytes() const { return 0; }
    // Lab walls and two samples; templates/procedural maps are phase 2.
    virtual void GenerateCellLayout(TArray<FERCellPanelDesc>& Out) const;
private:
    friend class FERBlockNetTransactionTest;
    friend class FERCellNetTransactionTest;
    FERCellWorld CellWorld;
    UPROPERTY() TArray<TObjectPtr<class UDynamicMeshComponent>> CellMeshes;
    TMap<const UPrimitiveComponent*, int32> CellComponentChunks;
    TArray<int32> ChunkQuads, ChunkBoxes;
    TArray<TWeakObjectPtr<class AERCellNetShard>> AuthorityShards;
    TMap<int32, TWeakObjectPtr<class AERCellNetShard>> ReceivedShards;
    TArray<uint32> AppliedChunkRevisions;
    bool GenerateCells();
    void BuildCellComponents();
    void ClearCells();
    void RebuildCellChunk(int32 Chunk);
    void PublishCellGeneration();
    void PublishCellChanges(const TArray<int32>& Chunks);
    void TryApplyCellNetwork();
    bool OnRepCellDescriptor();
    void SpawnMaterialImpact(const FERBlockMaterialProperties& Profile, bool bDecal, const FVector& Position, const FVector& Normal);
    UFUNCTION(NetMulticast,Unreliable) void MulticastCellImpact(uint32 Epoch,uint8 Material,FVector Position,FVector Normal);
    UPROPERTY(ReplicatedUsing=OnRep_Network) FERBlockNetworkDescriptor Network;
    UFUNCTION() void OnRep_Network();
    TArray<TWeakObjectPtr<class AERBlockNetPage>> AuthorityPages;
    TMap<int32,TWeakObjectPtr<class AERBlockNetPage>> ReceivedPages;
    TArray<uint32> AppliedPageVersions;
    uint32 BuiltEpoch=0,AppliedSequence=0;
    bool bApplyingNetwork=false,bMapReady=false,bNetworkError=false,bNetworkBegunPlay=false,bInCosmeticMulticast=false;
    void PublishNetworkGeneration();
    void PublishNetworkChanges(const TArray<FERBlockId>& Ids);
    void TryApplyNetwork();
    void SetMapCollision(bool Enabled);
    UFUNCTION(NetMulticast,Unreliable) void MulticastBlockImpact(uint32 Epoch,FERBlockId Id,FVector Position,FVector Normal);
    UPROPERTY() TArray<FERBlockRenderGroup> Groups;
    TMap<FERBlockId, FERBlockInstanceLocation> Instances;
    TMap<const UPrimitiveComponent*, int32> ComponentGroups;
    int32 CachedChunks=0;
    TArray<TWeakObjectPtr<class UDecalComponent>> ImpactDecals;
    TArray<TWeakObjectPtr<class UNiagaraComponent>> ImpactEmitters;
    FERBlockOperationProfile RebuildProfile;
    TArray<TWeakObjectPtr<class AERGrenadeExplosion>> OwnedBursts;
    FERBlockSupportGraph SupportGraph;
    FTimerHandle SupportTimer;
    bool bGraphValid=false, bSupportPending=false, bSupportRunning=false;
    uint64 SupportRevision=0;
    int32 DetachCursor=0;
    TArray<TWeakObjectPtr<class AERBlockDebris>> OwnedDebris;
    void ClearCollapse();
    void SpawnDebris(const FERBlockRecord& Block);
    void ClearOwnedEffects();
    bool CanShowEffects(const FVector& Position) const;
    void RebuildGroup(int32 Index);
    void UpdateGroup(int32 Index);
};
