#include "ERBlockWorld.h"
#include "HAL/PlatformTime.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

UERBlockMaterialCatalog::UERBlockMaterialCatalog()
{
    for (int32 Index = 0; Index < 7; ++Index)
    {
        FERBlockMaterialProperties& Entry = Materials.AddDefaulted_GetRef();
        Entry.Type = static_cast<EERBlockMaterial>(Index);
        // Drywall, Brick (hollow), Concrete (reinforced), Wood panel, Stone,
        // Metal (container sheet), Glass (pane break).
        const float Health[] = {13,65,390,26,650,65,5.2f};
        Entry.MaxHealth = Health[Index];
        const float EntryCost[] = {5,20,40,7,30,60,1};
        const float PerCm[] = {.5f,5,15,1.5f,7,15,.1f};
        const float BulletScale[] = {1,.75f,.1f,1,.05f,.5f,1};
        const float ExplosionScale[] = {2,1.5f,.5f,1.5f,.2f,1,3};
        const float Transmission[] = {.5f,.25f,.05f,.5f,.02f,.5f,.9f};
        Entry.EntryEnergyCost = EntryCost[Index];
        Entry.EnergyCostPerCm = PerCm[Index];
        Entry.BulletDamageScale = BulletScale[Index];
        Entry.ExplosionDamageScale = ExplosionScale[Index];
        Entry.ExplosionTransmission = Transmission[Index];
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
        if (!FMath::IsFinite(Entry->ExplosionTransmission) || Entry->ExplosionTransmission < 0 || Entry->ExplosionTransmission > 1) return false;
    }
    return true;
}
UERBlockWorld::UERBlockWorld() { PrimaryComponentTick.bCanEverTick = false; }
