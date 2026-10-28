#pragma once
#include "CoreMinimal.h"
#include "ERServerSettings.generated.h"

USTRUCT()
struct FERServerValue
{
    GENERATED_BODY()
    UPROPERTY() FName Key;
    UPROPERTY() float Value = 0;
};

// Sparse overrides: an absent asset field keeps the authored asset value.
// Only the host creates this snapshot; clients receive it with GameState.
USTRUCT()
struct FERServerRules
{
    GENERATED_BODY()
    UPROPERTY() TArray<FERServerValue> Values;
    UPROPERTY() bool Ready = false;
    float Get(FName Key, float AssetDefault = 0) const;
    bool Has(FName Key) const;
    void Set(FName Key, float Value);
    void Reset(FName Key);
    void Normalize();
    bool IsStandard() const { return Values.IsEmpty(); }
};

struct FERServerSetting
{
    FName Key;
    FString Section, Label, Unit;
    float Default = 0, Min = 0, Max = 1;
    bool Advanced = false, Integer = false, AssetDefault = false;
    // Optional weapon field; a dot separates nested USTRUCT properties.
    FName Weapon;
    FString Property;
    float Factor = 1;
};

class UERWeaponData;
class UERBlockMaterialCatalog;

namespace ERServer
{
    constexpr int32 DefaultMaxPlayers = 16;
    // Capacity includes the listen host, with no fixed upper human cap.
    inline int32 ValidMaxPlayers(int32 Value) { return FMath::Max(1, Value); }
    FDM_API const TArray<FERServerSetting>& Settings();
    FDM_API const FERServerSetting* Find(FName Key);
    FDM_API const FERServerRules& Rules(const UWorld* World);
    FDM_API float Get(const UWorld* World, FName Key, float AssetDefault = 0);
    FDM_API void Load(FERServerRules& Rules, const FString& File, const FString& Section);
    FDM_API void Save(const FERServerRules& Rules, const FString& File, const FString& Section);
    FDM_API FString Summary(const FERServerRules& Rules);
    FDM_API void TuneWeapon(UERWeaponData* Target, const FERServerRules& Rules, FName AssetId);
    FDM_API void TuneMaterials(UERBlockMaterialCatalog* Target, const FERServerRules& Rules);
    FDM_API float ReadNumber(const void* Container, const UStruct* Type, const FString& Property);
}
