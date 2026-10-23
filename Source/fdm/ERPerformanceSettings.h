#pragma once
#include "CoreMinimal.h"

namespace ERPerformance
{
// Local graphics preferences only; never change gameplay smoke or destruction.
struct FSettings
{
    int32 Defaults=1, Automatic=1, Scale=100, AA=0, GI=1, HardwareRT=1, Shadows=2,
        Reflections=2, MotionBlur=0, DepthOfField=0, Effects=2, Textures=2;
    float ManualPrimary() const { return AA==1 ? 1.f:FMath::Max(.5f,Scale/100.f); }
    void Load();
    void Save() const;
    void Apply() const;
    void PerformancePreset();
    void SetDefaults();
};
FSettings& Get();
}
