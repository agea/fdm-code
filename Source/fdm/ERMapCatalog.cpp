#include "ERMapCatalog.h"

namespace ERMaps
{
TConstArrayView<FERMapCatalogEntry> All()
{
    static const FERMapCatalogEntry Entries[]={
        {TEXT("central_building"),TEXT("Central Building · Industrial"),TEXT("/Game/EclipseRecoil/Maps/L_CentralBuilding"),true,true},
        {TEXT("depot"),TEXT("Depot · Reference"),TEXT("/Game/EclipseRecoil/Maps/L_Depot"),false,false}};
    return Entries;
}
const FERMapCatalogEntry& At(int32 Index){const auto Entries=All();return Entries[FMath::Clamp(Index,0,Entries.Num()-1)];}
TArray<FString> Labels(){TArray<FString> Result;for(const auto& E:All())Result.Add(E.Label);return Result;}
TArray<FName> Rotation(){TArray<FName> Result;for(const auto& E:All())if(E.InRotation)Result.Add(FName(E.Package));return Result;}
}
