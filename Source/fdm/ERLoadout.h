#pragma once
#include "CoreMinimal.h"
#include "ERLoadout.generated.h"
class AERCharacter;
struct FERServerRules;

// Catalog identity is independent of inventory order and serialized weapon enums.
USTRUCT()
struct FERLoadoutItem
{
    GENERATED_BODY()
    FERLoadoutItem() = default;
    FERLoadoutItem(int32 Key,int32 Count,int32 Spare):WeaponKey(Key),Loaded(Count),Reserve(Spare) {}
    UPROPERTY() int32 WeaponKey = 0; // Stable input/catalog key, never an inventory position.
    UPROPERTY() int32 Loaded = 0;
    UPROPERTY() int32 Reserve = 0;
    UPROPERTY() FName WeaponId = NAME_None;
    UPROPERTY() TObjectPtr<class UERWeaponData> Data;
};
USTRUCT()
struct FERResolvedLoadout
{
    GENERATED_BODY()
    UPROPERTY() bool bOmega = false;
    UPROPERTY() FName Preset;
    UPROPERTY() FName Variant;
    UPROPERTY() FString Role;
    UPROPERTY() int32 Skin = 0;
    UPROPERTY() float StartingHealth = 100;
    UPROPERTY() float StartingArmor = 100;
    UPROPERTY() bool Helmet = true;
    UPROPERTY() TArray<FERLoadoutItem> Items; // primary, secondary, knife, owned utility
    FString Summary() const;
};
namespace ERLoadouts
{
    FDM_API TArray<FName> Presets();
    FDM_API TArray<FName> Variants();
    FDM_API FName Migrate(int32 OldPrimary);
    FDM_API FERResolvedLoadout Resolve(FName Preset, FName Variant, bool Omega = false, const AERCharacter* Catalog = nullptr, const FERServerRules* Rules = nullptr);
    FDM_API FName WeaponAssetName(int32 StableWeaponId);
    FDM_API FString WeaponName(int32 WeaponKey);
}
