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
    TArray<int32> LobbyLeaders(TConstArrayView<int32> Votes);
}
