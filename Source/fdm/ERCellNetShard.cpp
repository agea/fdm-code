#include "ERCellNetShard.h"
#include "ERBlockRoom.h"
#include "Net/UnrealNetwork.h"

AERCellNetShard::AERCellNetShard()
{
    bReplicates = true; bAlwaysRelevant = true; SetNetUpdateFrequency(20); PrimaryActorTick.bCanEverTick = false;
}
bool AERCellNetShard::SetChunk(int32 Chunk, uint32 Revision, TArray<uint8>&& Mask)
{
    FERCellChunkItem* Item = Chunks.Items.FindByPredicate([Chunk](const auto& I) { return I.Chunk == Chunk; });
    if (Item && Item->Revision == Revision && Item->Mask == Mask) return false;
    if (!Item) { Item = &Chunks.Items.AddDefaulted_GetRef(); Item->Chunk = Chunk; }
    Item->Revision = Revision; Item->Mask = MoveTemp(Mask);
    Chunks.MarkItemDirty(*Item);
    return true;
}
SIZE_T AERCellNetShard::GetPayloadBytes() const
{
    SIZE_T Bytes = Chunks.Items.GetAllocatedSize();
    for (const auto& I : Chunks.Items) Bytes += I.Mask.GetAllocatedSize();
    return Bytes;
}
void AERCellNetShard::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AERCellNetShard, Room); DOREPLIFETIME(AERCellNetShard, Header); DOREPLIFETIME(AERCellNetShard, Chunks);
}
void AERCellNetShard::OnRep_State()
{
    if (Room) Room->ReceiveCellShard(this);
}
