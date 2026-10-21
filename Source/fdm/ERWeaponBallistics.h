#pragma once
#include "CoreMinimal.h"

namespace ERWeaponBallistics
{
enum class EHitZone : uint8 { Head, Stomach, ChestArms, Legs };
inline EHitZone ZoneForBone(FName Bone)
{
    const FString Name=Bone.ToString().ToLower();
    if(Name.Contains(TEXT("head")) || Name.Contains(TEXT("neck"))) return EHitZone::Head;
    if(Name.Contains(TEXT("thigh")) || Name.Contains(TEXT("calf")) || Name.Contains(TEXT("foot")) || Name.Contains(TEXT("ball"))) return EHitZone::Legs;
    if(Name==TEXT("pelvis") || Name==TEXT("spine_01")) return EHitZone::Stomach;
    return EHitZone::ChestArms;
}
inline float ZoneMultiplier(EHitZone Zone, float HeadshotMultiplier = 4.f)
{
    switch(Zone) { case EHitZone::Head:return HeadshotMultiplier; case EHitZone::Stomach:return 1.25f; case EHitZone::Legs:return .75f; default:return 1; }
}
inline FVector PelletDirection(const FVector& Aim,int32 Index,int32 Count,float Spread,FRandomStream& Random)
{
    // Six stable radial slots, plus a small independent disk jitter. No guaranteed centre pellet.
    const float Angle=2*PI*Index/FMath::Max(1,Count);
    const float Radius=(Index%2 ? .85f : .65f);
    const float JitterAngle=Random.FRandRange(0,2*PI),JitterRadius=.12f*FMath::Sqrt(Random.FRand());
    const float X=Radius*FMath::Cos(Angle)+JitterRadius*FMath::Cos(JitterAngle);
    const float Y=Radius*FMath::Sin(Angle)+JitterRadius*FMath::Sin(JitterAngle);
    const auto Basis=FRotationMatrix::MakeFromX(Aim);
    return (Aim+FMath::Tan(FMath::DegreesToRadians(Spread))*(Basis.GetScaledAxis(EAxis::Y)*X+Basis.GetScaledAxis(EAxis::Z)*Y)).GetSafeNormal();
}
// Gameplay fragmentation envelope: full close damage and a smooth tail to zero.
inline float GrenadeDamage(float Peak, float DistanceCm, float InnerCm, float OuterCm)
{
    if (!FMath::IsFinite(DistanceCm) || !FMath::IsFinite(Peak) || OuterCm<=0 || DistanceCm>=OuterCm) return 0;
    InnerCm=FMath::Clamp(InnerCm,0.f,OuterCm-.01f);
    const float Tail=1.f-FMath::Clamp((DistanceCm-InnerCm)/(OuterCm-InnerCm),0.f,1.f);
    return FMath::Max(0.f,Peak)*Tail*Tail;
}
inline float GrenadeArmorRatio(float Ratio) { return FMath::Clamp(Ratio,1.6f,2.f); }
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
