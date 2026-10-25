#pragma once

#include "CoreMinimal.h"

// Reference coordinates are fractions of the live viewport, never fixed pixels.
// Uniform sizes use the fitting axis so circles, icons and text never stretch.
struct FERHUDLayout
{
    FVector2D Viewport;
    float Scale;

    explicit FERHUDLayout(FVector2D Size)
        : Viewport(Size), Scale(FMath::Min(Size.X / 1280.f, Size.Y / 720.f)) {}

    FVector2D Position(float X, float Y) const { return Viewport * FVector2D(X / 1280.f, Y / 720.f); }
    FVector2D Edge() const { return Position(40, 40); }
    float MinimapRadius() const { return 97.92f * Scale; }
    FVector2D MinimapCenter() const
    {
        const float Inset=30.f*Scale;
        return FVector2D(Viewport.X-Inset-MinimapRadius(),Inset+MinimapRadius());
    }
    static FVector2D SpawnBearing(float SpawnYaw,float ViewYaw)
    {
        const float Angle=FMath::DegreesToRadians(SpawnYaw-ViewYaw);
        return FVector2D(FMath::Sin(Angle),-FMath::Cos(Angle));
    }
};
