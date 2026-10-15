#include "ERBlockWorld.h"
#include "HAL/PlatformTime.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

UERBlockMaterialCatalog::UERBlockMaterialCatalog()
{
    for (int32 Index = 0; Index < 7; ++Index)
    {
        FERBlockMaterialProperties& Entry = Materials.AddDefaulted_GetRef();
        Entry.Type = static_cast<EERBlockMaterial>(Index);
        const float Health[] = {30,140,300,60,200,400,8};
        Entry.MaxHealth = Health[Index];
        const float EntryCost[] = {5,20,40,7,30,60,1};
        const float PerCm[] = {.5f,5,15,1.5f,7,15,.1f};
        const float BulletScale[] = {.5f,1,1,1,1,.5f,2};
        const float ExplosionScale[] = {2,.8f,.25f,1.2f,.5f,.35f,1.5f};
        Entry.EntryEnergyCost = EntryCost[Index];
        Entry.EnergyCostPerCm = PerCm[Index];
        Entry.BulletDamageScale = BulletScale[Index];
        Entry.ExplosionDamageScale = ExplosionScale[Index];
    }
}
const FERBlockMaterialProperties* UERBlockMaterialCatalog::Find(EERBlockMaterial Type) const
{
    if (const auto* Entry=Materials.FindByPredicate([Type](const auto& R){return R.Type==Type;})) return Entry;
    // Saved M1-M5 catalogs remain usable. New profiles have native gameplay
    // defaults until the additive asset migration supplies their PBR surfaces.
    if (uint8(Type)>=3 && uint8(Type)<7)
        return GetDefault<UERBlockMaterialCatalog>()->Materials.FindByPredicate([Type](const auto& R){return R.Type==Type;});
    return nullptr;
}
bool UERBlockMaterialCatalog::IsValidCatalog() const
{
    if (Materials.Num()<3 || Materials.Num()>7) return false;
    TSet<EERBlockMaterial> Seen;
    for(const auto& E:Materials) {if(uint8(E.Type)>6 || Seen.Contains(E.Type))return false;Seen.Add(E.Type);}
    for (int32 Index = 0; Index < 7; ++Index)
    {
        const auto* Entry = Find(static_cast<EERBlockMaterial>(Index));
        if (!Entry || !FMath::IsFinite(Entry->MaxHealth) || Entry->MaxHealth <= 0) return false;
        for (float Value : {Entry->BulletDamageScale, Entry->EntryEnergyCost, Entry->EnergyCostPerCm, Entry->ExplosionDamageScale})
            if (!FMath::IsFinite(Value) || Value < 0) return false;
    }
    return true;
}
UERBlockWorld::UERBlockWorld() { PrimaryComponentTick.bCanEverTick = false; }
bool UERBlockWorld::GenerateRoom(const FERBlockRoomConfig& Config, const UERBlockMaterialCatalog* Catalog)
{
    const FVector Size = Config.BlockSize;
    if (!Catalog || !Catalog->IsValidCatalog() || Size.ContainsNaN() ||
        !FMath::IsFinite(Config.InteriorSize) || !FMath::IsFinite(Config.Height) || !FMath::IsFinite(Config.ChunkSize) ||
        Config.InteriorSize < 100 || Config.Height < 100 || Config.ChunkSize < 100 ||
        Size.X < 1 || Size.Y < 1 || Size.Z < 1 || Size.Y > Config.InteriorSize) return false;
    // Reject excessive allocation before converting counts to integer indices.
    const double Estimate = 4 * FMath::CeilToDouble((Config.InteriorSize + 2 * Size.Y) / Size.X) * FMath::CeilToDouble(Config.Height / Size.Z);
    if (!FMath::IsFinite(Estimate) || Estimate > 50000) return false;
    Blocks.Reset(); Lookup.Reset(); SpatialIndex.Reset(); DestroyedCount = DetachedCount = 0; DamageRevision=0;
    SpatialChunkSize = Config.ChunkSize; MaxHalfDiagonal = 0;
    const int32 Rows = FMath::CeilToInt(Config.Height / Size.Z);
    for (int32 Wall = 0; Wall < 4; ++Wall)
    {
        // Y-oriented returns include the corners; X-oriented walls stop at the
        // interior edges. End caps are clipped, never overlapping at corners.
        const bool bAlongY = Wall % 2 != 0;
        const float Length = Config.InteriorSize + (bAlongY ? 2 * Size.Y : 0);
        const int32 Columns = FMath::CeilToInt(Length / Size.X);
        const float Offset = (Config.InteriorSize + Size.Y) * .5f;
        const EERBlockMaterial Material = static_cast<EERBlockMaterial>(Wall % 3);
        for (int32 Row = 0; Row < Rows; ++Row)
        {
            const float BlockHeight = FMath::Min(Size.Z, Config.Height - Row * Size.Z);
            for (int32 Column = 0; Column < Columns; ++Column)
            {
                const float Width = FMath::Min(Size.X, Length - Column * Size.X);
                const float Along = -Length * .5f + Column * Size.X + Width * .5f;
                const float Z = Row * Size.Z + BlockHeight * .5f;
                const FVector Center = bAlongY ? FVector(Wall == 1 ? Offset : -Offset, Along, Z) : FVector(Along, Wall == 0 ? -Offset : Offset, Z);
                FERBlockRecord& Block = Blocks.AddDefaulted_GetRef();
                Block.Id.Wall = Wall; Block.Id.Column = Column; Block.Id.Row = Row;
                Block.Material = Material; Block.Health = Catalog->Find(Material)->MaxHealth;
                Block.LocalTransform = FTransform(FRotator(0, bAlongY ? 90 : 0, 0), Center, FVector(Width, Size.Y, BlockHeight) / 100);
                Block.Chunk = FIntVector(FMath::FloorToInt(Center.X / Config.ChunkSize), FMath::FloorToInt(Center.Y / Config.ChunkSize), FMath::FloorToInt(Center.Z / Config.ChunkSize));
                Lookup.Add(Block.Id, Blocks.Num() - 1);
                SpatialIndex.FindOrAdd(Block.Chunk).Add(Blocks.Num() - 1);
                MaxHalfDiagonal = FMath::Max(MaxHalfDiagonal, float(Block.LocalTransform.GetScale3D().Size() * 50));
            }
        }
    }
    return true;
}
const FERBlockRecord* UERBlockWorld::Find(const FERBlockId& Id) const
{
    const int32* Index = Lookup.Find(Id);
    return Index ? &Blocks[*Index] : nullptr;
}
bool UERBlockWorld::Damage(const FERBlockId& Id, float Amount)
{
    const int32* Index = Lookup.Find(Id);
    if (!Index || !FMath::IsFinite(Amount) || Amount <= 0) return false;
    auto& Block = Blocks[*Index];
    if (Block.bIndestructible || !Block.IsAttached()) return false;
    Block.Health = FMath::Max(0.f, Block.Health - Amount);
    Block.State = Block.Health > 0 ? EERBlockState::Damaged : EERBlockState::Destroyed;
    if (Block.State == EERBlockState::Destroyed) {++DestroyedCount;++DamageRevision;}
    return true;
}
bool UERBlockWorld::DestroyBlock(const FERBlockId& Id)
{
    const auto* Block = Find(Id);
    return Block ? Damage(Id, Block->Health) : false;
}
TArray<FERBlockId> UERBlockWorld::QuerySphere(const FVector& LocalOrigin, float LocalRadius) const
{
    TRACE_CPUPROFILER_EVENT_SCOPE(ERBlock_QuerySphere);
    const double Start=FPlatformTime::Seconds(); LastQuery={};
    TArray<FERBlockId> Result;
    if (GetRemovedCount()==Blocks.Num()) return Result;
    if (LocalOrigin.ContainsNaN() || LocalOrigin.GetAbs().GetMax() > 100000000 || !FMath::IsFinite(LocalRadius) || LocalRadius <= 0 || LocalRadius > 1000000) return Result;
    const float Extent = LocalRadius + MaxHalfDiagonal;
    const FIntVector Low(FMath::FloorToInt((LocalOrigin.X - Extent) / SpatialChunkSize), FMath::FloorToInt((LocalOrigin.Y - Extent) / SpatialChunkSize), FMath::FloorToInt((LocalOrigin.Z - Extent) / SpatialChunkSize));
    const FIntVector High(FMath::FloorToInt((LocalOrigin.X + Extent) / SpatialChunkSize), FMath::FloorToInt((LocalOrigin.Y + Extent) / SpatialChunkSize), FMath::FloorToInt((LocalOrigin.Z + Extent) / SpatialChunkSize));
    TArray<int32> Indices;
    auto Add = [&](const TArray<int32>& Bucket)
    {
        ++LastQuery.Buckets; LastQuery.Examined+=Bucket.Num();
        for (int32 Index : Bucket)
            if (Blocks[Index].IsAttached() && FVector::DistSquared(LocalOrigin, Blocks[Index].LocalTransform.GetLocation()) <= double(Extent) * Extent) Indices.Add(Index);
    };
    const double Cells = (double(High.X) - Low.X + 1) * (double(High.Y) - Low.Y + 1) * (double(High.Z) - Low.Z + 1);
    if (Cells <= 100000)
    {
        for (int32 X = Low.X; X <= High.X; ++X)
            for (int32 Y = Low.Y; Y <= High.Y; ++Y)
                for (int32 Z = Low.Z; Z <= High.Z; ++Z)
                    if (const auto* Bucket = SpatialIndex.Find(FIntVector(X, Y, Z))) Add(*Bucket);
    }
    else for (const auto& Pair : SpatialIndex) Add(Pair.Value);
    // Hash iteration never defines gameplay/FX order, including the large-query fallback.
    Indices.Sort();
    for (int32 Index : Indices) Result.Add(Blocks[Index].Id);
    LastQuery.Returned=Result.Num(); LastQuery.Ms=(FPlatformTime::Seconds()-Start)*1000;
    return Result;
}
bool UERBlockWorld::LoadLayout(const TArray<FERBlockRecord>& Records, float ChunkSize, const UERBlockMaterialCatalog* Catalog)
{
    if (!Catalog || !Catalog->IsValidCatalog() || Records.IsEmpty() || Records.Num() > 50000 || !FMath::IsFinite(ChunkSize) || ChunkSize < 100) return false;
    TSet<FERBlockId> Ids;
    for (const auto& Block : Records)
    {
        const FVector Size = Block.LocalTransform.GetScale3D();
        if (Block.Id.Wall < 0 || Block.Id.Column < 0 || Block.Id.Row < 0 || Ids.Contains(Block.Id) ||
            Block.LocalTransform.ContainsNaN() || Size.GetMin() <= 0 || Block.LocalTransform.GetLocation().GetAbs().GetMax() > 100000 || !Catalog->Find(Block.Material)) return false;
        Ids.Add(Block.Id);
    }
    Blocks = Records; Lookup.Reset(); SpatialIndex.Reset(); DestroyedCount = DetachedCount = 0; DamageRevision=0;
    SpatialChunkSize = ChunkSize; MaxHalfDiagonal = 0;
    for (int32 Index = 0; Index < Blocks.Num(); ++Index)
    {
        auto& Block = Blocks[Index];
        Block.Health = Catalog->Find(Block.Material)->MaxHealth; Block.State = EERBlockState::Intact;
        const FVector Center = Block.LocalTransform.GetLocation();
        Block.Chunk = FIntVector(FMath::FloorToInt(Center.X / ChunkSize), FMath::FloorToInt(Center.Y / ChunkSize), FMath::FloorToInt(Center.Z / ChunkSize));
        Lookup.Add(Block.Id, Index); SpatialIndex.FindOrAdd(Block.Chunk).Add(Index);
        MaxHalfDiagonal = FMath::Max(MaxHalfDiagonal, float(Block.LocalTransform.GetScale3D().Size() * 50));
    }
    return true;
}

SIZE_T UERBlockWorld::GetStorageBytes() const
{
    SIZE_T Bytes=Blocks.GetAllocatedSize()+Lookup.GetAllocatedSize()+SpatialIndex.GetAllocatedSize();
    for (const auto& Bucket:SpatialIndex) Bytes+=Bucket.Value.GetAllocatedSize();
    return Bytes;
}

bool UERBlockWorld::DetachBlock(const FERBlockId& Id)
{
    const int32* Index=Lookup.Find(Id);if (!Index) return false;
    auto& Block=Blocks[*Index];
    if (!Block.IsAttached() || Block.bIndestructible || Block.bStructuralAnchor) return false;
    Block.State=EERBlockState::Detached;++DetachedCount;return true;
}

bool UERBlockWorld::ValidateNetworkState(int32 Index,uint8 State,float Health,const UERBlockMaterialCatalog* Catalog) const
{
    if(!Blocks.IsValidIndex(Index) || State>uint8(EERBlockState::Detached) || !FMath::IsFinite(Health) || Health<0)return false;
    const auto& B=Blocks[Index];const auto* P=Catalog ? Catalog->Find(B.Material):nullptr;
    if(!P || Health>P->MaxHealth)return false;
    if(State==uint8(EERBlockState::Destroyed))return Health==0 && !B.bIndestructible;
    if(State==uint8(EERBlockState::Detached))return Health>0 && !B.bIndestructible && !B.bStructuralAnchor;
    return Health>0 && (State!=uint8(EERBlockState::Intact) || Health==P->MaxHealth);
}
void UERBlockWorld::ApplyNetworkState(int32 Index,uint8 State,float Health)
{
    auto& B=Blocks[Index];
    if(B.State==EERBlockState::Destroyed)--DestroyedCount;
    if(B.State==EERBlockState::Detached)--DetachedCount;
    B.State=static_cast<EERBlockState>(State);B.Health=Health;
    if(B.State==EERBlockState::Destroyed)++DestroyedCount;
    if(B.State==EERBlockState::Detached)++DetachedCount;
}
