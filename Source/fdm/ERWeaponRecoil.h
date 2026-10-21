#pragma once
#include "CoreMinimal.h"
#include "ERWeaponRecoil.generated.h"

// Exact worksheet parameters. The workbook does not supply spray seeds or
// recoil integration/decay; those are explicit project adaptations below.
USTRUCT(BlueprintType)
struct FERWeaponRecoilMode
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float AngleVariance = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Magnitude = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float MagnitudeVariance = 0;
};

USTRUCT()
struct FERWeaponRecoilState
{
    GENERATED_BODY()
    // X = upward pitch, Y = yaw; degrees and degrees/second respectively.
    UPROPERTY() FVector2D Angle = FVector2D::ZeroVector;
    UPROPERTY() FVector2D Velocity = FVector2D::ZeroVector;
    UPROPERTY() double ServerTime = -100;
};

namespace ERWeaponRecoil
{
// Project response, independent of FPS and worksheet accuracy recovery.
// dAngle/dt = Velocity - 8*Angle; dVelocity/dt = -4.5*Velocity.
inline FERWeaponRecoilState At(const FERWeaponRecoilState& State,double Now)
{
    const double T=FMath::Max(0.,Now-State.ServerTime);
    const double A=FMath::Exp(-8.*T),V=FMath::Exp(-4.5*T);
    FERWeaponRecoilState Result;
    Result.Angle=State.Angle*A+State.Velocity*((V-A)/3.5);
    Result.Velocity=State.Velocity*V;Result.ServerTime=Now;
    return Result;
}
inline FVector2D Impulse(const FERWeaponRecoilMode& P,float AngleUnit,float MagnitudeUnit)
{
    const float Angle=FMath::DegreesToRadians(FMath::Lerp(-P.AngleVariance,P.AngleVariance,AngleUnit));
    const float Magnitude=FMath::Max(0.f,P.Magnitude+FMath::Lerp(-P.MagnitudeVariance,P.MagnitudeVariance,MagnitudeUnit));
    // Adopt one worksheet magnitude unit as one degree/second of impulse.
    return FVector2D(FMath::Cos(Angle),FMath::Sin(Angle))*Magnitude;
}
inline FVector2D Sample(const FERWeaponRecoilMode& P,bool RandomPattern,int32 Seed,int32 Index,FRandomStream& Random)
{
    FRandomStream Pattern{int32(HashCombineFast(uint32(Seed),uint32(Index)))};
    auto& Stream=RandomPattern?Random:Pattern;
    const float Angle=Stream.FRand(),Magnitude=Stream.FRand();
    return Impulse(P,Angle,Magnitude);
}
inline FRotator Rotation(const FERWeaponRecoilState& State)
{ return FRotator(State.Angle.X,State.Angle.Y,0); }
constexpr double PatternResetSeconds=.55;
}
