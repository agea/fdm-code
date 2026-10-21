#pragma once
#include "CoreMinimal.h"
#include "ERWeaponAccuracy.generated.h"

// Raw Values worksheet units: tangent-plane offsets multiplied by 1000.
// Recovery times are seconds to remove 90% of the accumulated inaccuracy.
USTRUCT(BlueprintType)
struct FERWeaponAccuracyMode
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Spread = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Stand = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Crouch = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Move = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Ladder = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Jump = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float JumpInitial = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float JumpApex = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Land = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Fire = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float RecoveryStand = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float RecoveryStandFinal = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float RecoveryCrouch = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float RecoveryCrouchFinal = 1;
    // Source units/s; Unreal velocity is cm/s (one Source unit = 2.54cm).
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float MaxSpeed = 250;
};

namespace ERWeaponAccuracy
{
inline float Degrees(float Offset)
{ return FMath::RadiansToDegrees(FMath::Atan(FMath::Max(0.f,Offset)*.001f)); }
inline float Recover(float Amount, double Seconds, float RecoveryTime)
{ return FMath::Max(0.f,Amount)*FMath::Pow(.1f,float(FMath::Max(0.,Seconds))/FMath::Max(.001f,RecoveryTime)); }
inline float Recovery(const FERWeaponAccuracyMode& P,float Low,int32 Shots)
{
    // Selected workbook rows: AK's 4th/5th bullets interpolate by 1/3 and 2/3.
    const float Final=FMath::Clamp((Shots-2)/3.f,0.f,1.f);
    return FMath::Lerp(FMath::Lerp(P.RecoveryStand,P.RecoveryStandFinal,Final),
        FMath::Lerp(P.RecoveryCrouch,P.RecoveryCrouchFinal,Final),FMath::Clamp(Low,0.f,1.f));
}
inline float Base(const FERWeaponAccuracyMode& P,float Low,float SpeedCm,bool Ladder,bool Air,float VerticalCm,float JumpCm)
{
    if(Ladder)return 2.f*P.Ladder; // Simple worksheet: Spread + 2*InaccuracyLadder.
    const float Motion=P.Move*FMath::Clamp(SpeedCm/FMath::Max(1.f,P.MaxSpeed*2.54f),0.f,1.f);
    const float Jump=Air ? P.Jump+FMath::Lerp(P.JumpApex,P.JumpInitial,
        FMath::Clamp(FMath::Abs(VerticalCm)/FMath::Max(1.f,JumpCm),0.f,1.f)) : 0;
    return FMath::Lerp(P.Stand,P.Crouch,FMath::Clamp(Low,0.f,1.f))+Motion+Jump;
}
inline FVector Sample(const FVector& Aim,float Spread,float Inaccuracy,FRandomStream& Random)
{
    // Independent weapon spread and player inaccuracy offsets. The HUD shows
    // their sum as the enclosing half-angle, not an unrelated cosmetic cone.
    const float A=Random.FRandRange(0,2*PI),R=Random.FRand()*FMath::Max(0.f,Spread)*.001f;
    const float B=Random.FRandRange(0,2*PI),S=Random.FRand()*FMath::Max(0.f,Inaccuracy)*.001f;
    const auto Basis=FRotationMatrix::MakeFromX(Aim);
    return (Aim+Basis.GetScaledAxis(EAxis::Y)*(R*FMath::Cos(A)+S*FMath::Cos(B))+
        Basis.GetScaledAxis(EAxis::Z)*(R*FMath::Sin(A)+S*FMath::Sin(B))).GetSafeNormal();
}
}
