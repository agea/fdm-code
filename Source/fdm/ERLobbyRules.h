#pragma once
#include "CoreMinimal.h"
namespace ERLobby
{
inline bool HasQuorum(int32 Humans, int32 Ready) { return Humans > 0 && Ready >= Humans / 2 + 1; }
inline TArray<int32> LeadingMaps(TConstArrayView<int32> Votes, int32 MapCount)
{
    TArray<int32> Counts; Counts.Init(0, MapCount);
    for (int32 Vote : Votes) if (Counts.IsValidIndex(Vote)) ++Counts[Vote];
    int32 Best = 0;
    for (int32 Count : Counts) Best = FMath::Max(Best, Count);
    TArray<int32> Result;
    for (int32 I = 0; I < Counts.Num(); ++I) if (Counts[I] == Best) Result.Add(I);
    return Result;
}
}
