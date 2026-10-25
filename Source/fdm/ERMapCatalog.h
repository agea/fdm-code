#pragma once
#include "CoreMinimal.h"

struct FERMapCatalogEntry
{
    const TCHAR* Id;
    const TCHAR* Label;
    const TCHAR* Package;
    bool Procedural;
    bool InRotation;
};

// One catalog supplies session travel, menu choices and the actual match pool.
namespace ERMaps
{
    TConstArrayView<FERMapCatalogEntry> All();
    const FERMapCatalogEntry& At(int32 Index);
    TArray<FString> Labels();
    TArray<FName> Rotation();
    // Record only actual arrivals: failed travel must not consume a template.
    void RecordRoundMap(FName CurrentPackage,TArray<FName>& Played);
    TArray<FName> NextRoundMaps(FName CurrentPackage,TConstArrayView<FName> Played);
    TArray<int32> LobbyLeaders(TConstArrayView<int32> Votes);
}
