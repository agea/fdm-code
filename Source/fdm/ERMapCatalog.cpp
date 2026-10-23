#include "ERMapCatalog.h"
#include "ERLobbyRules.h"

namespace ERMaps
{
TConstArrayView<FERMapCatalogEntry> All()
{
    static const FERMapCatalogEntry Entries[]={
        {TEXT("central_building"),TEXT("Central Building · Industrial"),TEXT("/Game/EclipseRecoil/Maps/L_CentralBuilding"),true,true},
        {TEXT("depot"),TEXT("Depot · Reference"),TEXT("/Game/EclipseRecoil/Maps/L_Depot"),false,false},
        {TEXT("middle_eastern_alleys"),TEXT("Middle Eastern Alleys · Desert"),TEXT("/Game/EclipseRecoil/Maps/L_MiddleEasternAlleys"),true,true}};
    return Entries;
}
const FERMapCatalogEntry& At(int32 Index){const auto Entries=All();return Entries[FMath::Clamp(Index,0,Entries.Num()-1)];}
TArray<FString> Labels(){TArray<FString> Result;for(const auto& E:All())Result.Add(E.Label);return Result;}
TArray<FName> Rotation(){TArray<FName> Result;for(const auto& E:All())if(E.InRotation)Result.Add(FName(E.Package));return Result;}
TArray<int32> LobbyLeaders(TConstArrayView<int32> Votes)
{
    TArray<int32> Eligible;
    for (int32 I=0; I<All().Num(); ++I) if (At(I).InRotation) Eligible.Add(I);
    TArray<int32> MappedVotes;
    for (int32 Vote : Votes) MappedVotes.Add(Eligible.IndexOfByKey(Vote));
    TArray<int32> Result;
    for (int32 Leader : ERLobby::LeadingMaps(MappedVotes,Eligible.Num())) Result.Add(Eligible[Leader]);
    return Result;
}

}
