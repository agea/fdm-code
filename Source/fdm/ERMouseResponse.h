#pragma once

#include "CoreMinimal.h"

namespace ERMouseResponse
{
// Input is the mouse displacement after the authored IA_Look scalar (0.18).
// Estimate speed before user sensitivity: changing the sensitivity slider must
// not move the acceleration thresholds. Apply one gain to preserve diagonals.
inline float Gain(const FVector2D& LookDelta, double FrameSeconds, float Strength)
{
    if (!FMath::IsFinite(FrameSeconds) || FrameSeconds <= 0 ||
        !FMath::IsFinite(LookDelta.X) || !FMath::IsFinite(LookDelta.Y) || !FMath::IsFinite(Strength)) return 1.f;
    Strength = FMath::Clamp(Strength, 0.f, 1.f);
    const double Speed = LookDelta.Size() / (0.18 * FrameSeconds);
    const double T = FMath::Clamp((Speed - 180.0) / 1620.0, 0.0, 1.0);
    const float Blend = static_cast<float>(T * T * (3.0 - 2.0 * T));
    return FMath::Lerp(1.f - .5f * Strength, 1.f + 1.6f * Strength, Blend);
}
}
