#pragma once
#include "CoreMinimal.h"

namespace ERWeaponBallistics
{
// CS Source units are inches. RangeModifier applies every 500 units (12.7m).
inline float AtRange(float Damage, float Modifier, float DistanceCm)
{
    return FMath::Max(0.f, Damage) * FMath::Pow(FMath::Clamp(Modifier, .01f, 1.f), FMath::Max(0.f, DistanceCm) / 1270.f);
}
inline float AbsorbArmor(float Damage, float Ratio, float& Armor)
{
    Damage = FMath::Max(0.f, Damage);
    if (Armor <= 0) return Damage;
    const float HealthDamage = Damage * FMath::Clamp(Ratio * .5f, 0.f, 1.f);
    const float Cost = (Damage - HealthDamage) * .5f;
    const float Spent = FMath::Min(Armor, Cost);
    Armor = FMath::Max(0.f, Armor - Spent);
    return Damage - Spent * 2.f;
}
}
