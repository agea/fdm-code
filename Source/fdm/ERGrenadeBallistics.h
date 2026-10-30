#pragma once
#include "CoreMinimal.h"

namespace ERGrenadeBallistics
{
inline FVector Throw(FRotator Aim,float Speed,float Range,FVector Movement)
{
    // Keep the original overarm arc and horizontal range tuning at level aim.
    FVector Velocity=(Aim.Vector()+FVector(0,0,.35f)).GetSafeNormal()*Speed;
    const float HorizontalRange=FMath::Clamp(Range,1.f,2.f);
    Velocity.X*=HorizontalRange;Velocity.Y*=HorizontalRange;
    // Pitch controls effort, reaching +20% at +45 and -20% at -45 degrees.
    const float Effort=1.f+.2f*FMath::Clamp(FRotator::NormalizeAxis(Aim.Pitch)/45.f,-1.f,1.f);
    return Velocity*Effort+Movement;
}
}
