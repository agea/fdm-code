#include "ERResolutionPolicy.h"
#include "ERPerformanceSettings.h"
#include "DynamicResolutionState.h"
#include "LegacyScreenPercentageDriver.h"
#include "Engine/Engine.h"
#include "RHI.h"
#include "RHICommandList.h"
#include "GPUProfiler.h"
#include "Misc/App.h"
#include "RenderingThread.h"
#include "StereoRendering.h"
#include "HAL/IConsoleManager.h"

namespace
{
struct FERTiming
{
    FRHIGPUFrameTimeHistory::FState History;
    TAtomic<float> Ms{0};
};
class FERResolutionState final : public IDynamicResolutionState
{
    FERResolutionPolicy Policy;
    TSharedRef<FERTiming,ESPMode::ThreadSafe> Timing=MakeShared<FERTiming,ESPMode::ThreadSafe>();
    bool Enabled=false;
    float Min=.4f,Max=1;
    double NextAudit=0;
    DynamicRenderScaling::TMap<float> Fractions(float Fraction) const
    {DynamicRenderScaling::TMap<float> Values;Values.SetAll(1.f);Values[GDynamicPrimaryResolutionFraction]=Fraction;return Values;}
public:
    virtual void ResetHistory() override {Policy.Reset();Timing->Ms.Store(0);}
    virtual bool IsSupported() const override
    {return GRHISupportsDynamicResolution && !(GEngine->StereoRenderingDevice.IsValid() && GEngine->StereoRenderingDevice->IsStereoEnabled());}
    virtual void SetupMainViewFamily(FSceneViewFamily& Family) override
    {if(Enabled)Family.SetScreenPercentageInterface(new FLegacyScreenPercentageDriver(Family,Policy.Fraction,Max));}
    virtual void SetTemporalUpscaler(const UE::Renderer::Private::ITemporalUpscaler* Upscaler) override
    {
        Min=Upscaler ? FMath::Max(.4f,Upscaler->GetMinUpsampleResolutionFraction()):.4f;
        Max=Upscaler ? FMath::Min(1.f,Upscaler->GetMaxUpsampleResolutionFraction()):1.f;
        Policy.Fraction=FMath::Clamp(Policy.Fraction,Min,Max);
    }
    virtual DynamicRenderScaling::TMap<float> GetResolutionFractionsApproximation() const override {return Fractions(Policy.Fraction);}
    virtual DynamicRenderScaling::TMap<float> GetResolutionFractionsUpperBound() const override {return Fractions(Max);}
    virtual void SetEnabled(bool Value) override {Enabled=Value;}
    virtual bool IsEnabled() const override {return Enabled;}
    virtual void ProcessEvent(EDynamicResolutionStateEvent Event) override
    {
        if(!Enabled || Event!=EDynamicResolutionStateEvent::BeginFrame)return;
        if(ERPerformance::Get().Automatic) Policy.Update(FApp::GetDeltaTime(),Timing->Ms.Load(),Min,Max);
        else Policy.Fraction=ERPerformance::Get().ManualPrimary();
#if !UE_BUILD_SHIPPING
        if(FParse::Param(FCommandLine::Get(),TEXT("ERPerformanceAudit")) && FPlatformTime::Seconds()>NextAudit)
        {
            NextAudit=FPlatformTime::Seconds()+1;
            UE_LOG(LogTemp,Display,TEXT("ER_RENDER_POLICY gpu=%.2f filtered=%.2f fraction=%.3f direction=%d pending=%.2f stable=%.2f hold=%.2f"),Timing->Ms.Load(),Policy.FilteredMs,Policy.Fraction,Policy.Direction,Policy.PendingSeconds,Policy.StableSeconds,Policy.HoldSeconds);
        }
#endif
        ENQUEUE_RENDER_COMMAND(ERReadGPUFrameTime)([Sample=Timing](FRHICommandListImmediate&)
        {
            uint64 Cycles;
            while(Sample->History.PopFrameCycles(Cycles)!=FRHIGPUFrameTimeHistory::EResult::Empty)
                Sample->Ms.Store(FPlatformTime::ToMilliseconds64(Cycles));
        });
    }
};
}
void ERInstallResolutionGovernor()
{
    static TWeakPtr<IDynamicResolutionState> Installed;
    if(!GEngine || !GDynamicRHI || GUsingNullRHI || Installed.IsValid())return;
    auto State=MakeShared<FERResolutionState>();Installed=State;
    GEngine->ChangeDynamicResolutionStateAtNextFrame(State);
    UE_LOG(LogTemp,Display,TEXT("ER_RENDER_GOVERNOR target=60 tolerance=56..66 delay=3s stable=2s hold=4s"));
}
