#pragma once
#include "CoreMinimal.h"

namespace ERPerformance
{
enum class EPreset : uint8 { Competitive, Performance, Balanced, Quality };
// Local graphics preferences only; never change gameplay smoke or destruction.
struct FSettings
{
    int32 Defaults=1, Automatic=1, Scale=100, AA=0, GI=1, HardwareRT=1, Shadows=2,
        Reflections=2, MotionBlur=0, DepthOfField=0, Effects=2, Textures=2, PostProcess=3, LumenQuality=1, FrameCap=1;
    float ManualPrimary() const { return AA==1 ? 1.f:FMath::Max(.5f,Scale/100.f); }
    void Load();
    void Save() const;
    void Apply() const;
    // Local decorative budgets; replicated gameplay state is unaffected.
    int32 BulletHoleLimit() const { return Defaults ? 128:Effects==0 ? 16:Effects==1 ? 32:Effects==2 ? 64:128; }
    int32 ImpactEffectLimit() const { return Defaults ? 64:Effects==0 ? 4:Effects==1 ? 8:Effects==2 ? 16:64; }
    void ApplyPreset(EPreset Preset);
    void PerformancePreset();
    void SetDefaults();
};
FSettings& Get();
}
