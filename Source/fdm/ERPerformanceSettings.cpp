#include "ERPerformanceSettings.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ConfigCacheIni.h"

namespace ERPerformance
{
namespace
{
struct FOriginalValue { const TCHAR* Name; float Value; };
TArray<FOriginalValue>& Originals()
{
    static TArray<FOriginalValue> Values;
    if(Values.IsEmpty())
    {
        for(const TCHAR* Name : {TEXT("r.AntiAliasingMethod"),TEXT("r.TemporalAA.Upsampling"),TEXT("r.TSR.History.ScreenPercentage"),TEXT("r.TSR.History.UpdateQuality"),TEXT("r.TSR.ShadingRejection.Flickering"),TEXT("r.DynamicGlobalIlluminationMethod"),TEXT("r.Lumen.HardwareRayTracing"),TEXT("r.RayTracing.ForceAllRayTracingEffects"),TEXT("sg.ShadowQuality"),TEXT("r.ShadowQuality"),TEXT("r.Shadow.Virtual.OnePassProjection.MaxLightsPerPixel"),TEXT("r.Shadow.MaxResolution"),TEXT("r.Shadow.DistanceScale"),TEXT("r.Shadow.Virtual.ResolutionLodBiasLocal"),TEXT("r.Shadow.Virtual.ResolutionLodBiasDirectional"),TEXT("r.ReflectionMethod"),TEXT("r.SSR.Quality"),TEXT("r.ReflectionEnvironment"),TEXT("r.DefaultFeature.MotionBlur"),TEXT("r.MotionBlurQuality"),TEXT("r.DepthOfFieldQuality"),TEXT("sg.EffectsQuality"),TEXT("sg.TextureQuality"),TEXT("r.Streaming.LimitPoolSizeToVRAM"),TEXT("r.Streaming.MipBias"),TEXT("r.MaxAnisotropy")})
            if(auto* CVar=IConsoleManager::Get().FindConsoleVariable(Name)) Values.Add({Name,CVar->GetFloat()});
    }
    return Values;
}
FSettings OriginalSettings()
{
    auto Value=[](const TCHAR* Name,int32 Fallback)
    {
        for(const auto& Original:Originals()) if(FCString::Strcmp(Original.Name,Name)==0) return FMath::RoundToInt(Original.Value);
        return Fallback;
    };
    FSettings S;
    S.Shadows=FMath::Clamp(Value(TEXT("sg.ShadowQuality"),2),0,3);
    S.Effects=FMath::Clamp(Value(TEXT("sg.EffectsQuality"),2),0,3);
    S.Textures=FMath::Clamp(Value(TEXT("sg.TextureQuality"),2),0,3);
    S.GI=Value(TEXT("r.DynamicGlobalIlluminationMethod"),1)==1;
    S.HardwareRT=Value(TEXT("r.Lumen.HardwareRayTracing"),1)!=0;
    S.Reflections=Value(TEXT("r.ReflectionMethod"),1)==1 ? 2:Value(TEXT("r.ReflectionMethod"),1)==2 ? 1:0;
    S.MotionBlur=Value(TEXT("r.DefaultFeature.MotionBlur"),0)!=0;
    S.DepthOfField=Value(TEXT("r.DepthOfFieldQuality"),0)>0;
    return S;
}
const TCHAR* Section=TEXT("EclipseRecoil.Performance");
void Set(const TCHAR* Name, float Value)
{
    if(auto* Variable=IConsoleManager::Get().FindConsoleVariable(Name))
        Variable->Set(Value,ECVF_SetByGameOverride);
}
}
FSettings& Get() { static FSettings Settings; return Settings; }
void FSettings::Load()
{
    if(!GConfig->GetInt(Section,TEXT("Defaults"),Defaults,GGameUserSettingsIni))
    {
        int32 SavedScale=0;
        Defaults=GConfig->GetInt(Section,TEXT("Scale"),SavedScale,GGameUserSettingsIni) ? 0:1;
    }
    Defaults=FMath::Clamp(Defaults,0,1);
    GConfig->GetInt(Section,TEXT("Automatic"),Automatic,GGameUserSettingsIni);
    Automatic=FMath::Clamp(Automatic,0,1);
    GConfig->GetInt(Section,TEXT("Scale"),Scale,GGameUserSettingsIni);
    Scale=FMath::Clamp(Scale,1,100);
    GConfig->GetInt(Section,TEXT("AA"),AA,GGameUserSettingsIni);
    AA=FMath::Clamp(AA,0,2);
    GConfig->GetInt(Section,TEXT("GI"),GI,GGameUserSettingsIni);
    GI=FMath::Clamp(GI,0,1);
    GConfig->GetInt(Section,TEXT("HardwareRT"),HardwareRT,GGameUserSettingsIni);
    HardwareRT=FMath::Clamp(HardwareRT,0,1);
    GConfig->GetInt(Section,TEXT("Shadows"),Shadows,GGameUserSettingsIni);
    Shadows=FMath::Clamp(Shadows,0,3);
    GConfig->GetInt(Section,TEXT("Reflections"),Reflections,GGameUserSettingsIni);
    Reflections=FMath::Clamp(Reflections,0,2);
    GConfig->GetInt(Section,TEXT("MotionBlur"),MotionBlur,GGameUserSettingsIni);
    MotionBlur=FMath::Clamp(MotionBlur,0,1);
    GConfig->GetInt(Section,TEXT("DepthOfField"),DepthOfField,GGameUserSettingsIni);
    DepthOfField=FMath::Clamp(DepthOfField,0,1);
    GConfig->GetInt(Section,TEXT("Effects"),Effects,GGameUserSettingsIni);
    Effects=FMath::Clamp(Effects,0,3);
    GConfig->GetInt(Section,TEXT("Textures"),Textures,GGameUserSettingsIni);
    Textures=FMath::Clamp(Textures,0,3);
    if(Defaults) *this=OriginalSettings();
}
void FSettings::Save() const
{
    GConfig->SetInt(Section,TEXT("Defaults"),Defaults,GGameUserSettingsIni);
    GConfig->SetInt(Section,TEXT("Automatic"),Automatic,GGameUserSettingsIni);
    GConfig->SetInt(Section,TEXT("Scale"),Scale,GGameUserSettingsIni);
    GConfig->SetInt(Section,TEXT("AA"),AA,GGameUserSettingsIni);
    GConfig->SetInt(Section,TEXT("GI"),GI,GGameUserSettingsIni);
    GConfig->SetInt(Section,TEXT("HardwareRT"),HardwareRT,GGameUserSettingsIni);
    GConfig->SetInt(Section,TEXT("Shadows"),Shadows,GGameUserSettingsIni);
    GConfig->SetInt(Section,TEXT("Reflections"),Reflections,GGameUserSettingsIni);
    GConfig->SetInt(Section,TEXT("MotionBlur"),MotionBlur,GGameUserSettingsIni);
    GConfig->SetInt(Section,TEXT("DepthOfField"),DepthOfField,GGameUserSettingsIni);
    GConfig->SetInt(Section,TEXT("Effects"),Effects,GGameUserSettingsIni);
    GConfig->SetInt(Section,TEXT("Textures"),Textures,GGameUserSettingsIni);
    GConfig->Flush(false,GGameUserSettingsIni);
}
void FSettings::Apply() const
{
    auto& Baseline=Originals();
    if(Defaults)
    {
        for(const auto& Original:Baseline) Set(Original.Name,Original.Value);
        return;
    }
    Set(TEXT("r.AntiAliasingMethod"),AA==0 ? 4:2);
    Set(TEXT("r.TemporalAA.Upsampling"),AA==1 ? 0:1);
    Set(TEXT("r.TSR.History.ScreenPercentage"),100);
    Set(TEXT("r.TSR.History.UpdateQuality"),1);
    Set(TEXT("r.TSR.ShadingRejection.Flickering"),0);
    Set(TEXT("r.DynamicGlobalIlluminationMethod"),GI ? 1:0);
    Set(TEXT("r.Lumen.HardwareRayTracing"),HardwareRT);
    Set(TEXT("r.RayTracing.ForceAllRayTracingEffects"),HardwareRT ? -1:0);
    Set(TEXT("sg.ShadowQuality"),Shadows);
    Set(TEXT("r.ShadowQuality"),Shadows==0 ? 0:Shadows==1 ? 2:5);
    Set(TEXT("r.Shadow.Virtual.OnePassProjection.MaxLightsPerPixel"),Shadows<=1 ? 4:16);
    Set(TEXT("r.Shadow.MaxResolution"),Shadows==0 ? 256:Shadows==1 ? 512:Shadows==2 ? 1024:2048);
    Set(TEXT("r.Shadow.DistanceScale"),Shadows==0 ? .25f:Shadows==1 ? .5f:1.f);
    Set(TEXT("r.Shadow.Virtual.ResolutionLodBiasLocal"),Shadows<=1 ? 2:0);
    Set(TEXT("r.Shadow.Virtual.ResolutionLodBiasDirectional"),Shadows<=1 ? 1: -1.5f);
    Set(TEXT("r.ReflectionMethod"),Reflections==2 ? 1:Reflections==1 ? 2:0);
    Set(TEXT("r.SSR.Quality"),Reflections ? 1:0);
    Set(TEXT("r.ReflectionEnvironment"),Reflections ? 1:0);
    Set(TEXT("r.DefaultFeature.MotionBlur"),MotionBlur);
    Set(TEXT("r.MotionBlurQuality"),MotionBlur ? 3:0);
    Set(TEXT("r.DepthOfFieldQuality"),DepthOfField ? 2:0);
    Set(TEXT("sg.EffectsQuality"),Effects);
    Set(TEXT("sg.TextureQuality"),Textures);
    // Let the streaming manager use the detected hardware budget, rather than
    // inventing a dedicated-VRAM figure on unified-memory machines.
    Set(TEXT("r.Streaming.LimitPoolSizeToVRAM"),1);
    Set(TEXT("r.Streaming.MipBias"),3-Textures);
    Set(TEXT("r.MaxAnisotropy"),Textures==0 ? 1:Textures==1 ? 2:Textures==2 ? 4:8);
}
void FSettings::PerformancePreset()
{
    Defaults=0; Automatic=1; Scale=67; AA=2; GI=HardwareRT=Reflections=MotionBlur=DepthOfField=0;
    Shadows=Effects=Textures=1;
    Apply(); Save();
}
void FSettings::SetDefaults()
{
    *this=OriginalSettings();
    Apply(); Save();
}
}
