#include "ERPerformanceSettings.h"
#include "Misc/AutomationTest.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FERPerformancePreferencesTest,"EclipseRecoil.Menu.PerformancePreferences",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FERPerformancePreferencesTest::RunTest(const FString&)
{
    const FString TestIni=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Testing/PerformancePreferences.ini"));
    GConfig->Add(TestIni,FConfigFile());
    TGuardValue<FString> IsolatedIni(GGameUserSettingsIni,TestIni);
    ERPerformance::FSettings S;
    S.Defaults=0; S.Automatic=0; S.Scale=1; S.AA=2; S.Textures=3; S.MotionBlur=1; S.DepthOfField=1;
    S.Save();
    if(!FParse::Param(FCommandLine::Get(),TEXT("Multiprocess")) || FParse::Param(FCommandLine::Get(),TEXT("MultiprocessSaveConfig")))
    {
        GConfig->UnloadFile(TestIni);
        GConfig->LoadFile(TestIni);
    }
    else AddInfo(TEXT("Disk reload requires -MultiprocessSaveConfig; checking cached preferences in this run."));
    ERPerformance::FSettings Loaded; Loaded.Load();
    TestEqual(TEXT("Manual mode survives restart"),Loaded.Automatic,0);
    TestEqual(TEXT("1 percent survives restart"),Loaded.Scale,1);
    TestEqual(TEXT("Temporal method survives restart"),Loaded.AA,2);
    TestEqual(TEXT("Texture quality survives restart"),Loaded.Textures,3);
    TestEqual(TEXT("Post processing survives restart"),Loaded.DepthOfField,1);
    TestEqual(TEXT("Temporal scale stays in supported range"),Loaded.ManualPrimary(),.5f);
    Loaded.Scale=100; TestEqual(TEXT("Full resolution"),Loaded.ManualPrimary(),1.f);
    Loaded.Scale=1; Loaded.AA=1; TestEqual(TEXT("TAA uses spatial scale"),Loaded.ManualPrimary(),1.f);
    const int32 OriginalAA=IConsoleManager::Get().FindConsoleVariable(TEXT("r.AntiAliasingMethod"))->GetInt();
    const float OriginalMipBias=IConsoleManager::Get().FindConsoleVariable(TEXT("r.Streaming.MipBias"))->GetFloat();
    const int32 OriginalShadowQuality=IConsoleManager::Get().FindConsoleVariable(TEXT("sg.ShadowQuality"))->GetInt();
    Loaded.PerformancePreset();
    auto Value=[](const TCHAR* Name) { return IConsoleManager::Get().FindConsoleVariable(Name)->GetInt(); };
    TestEqual(TEXT("Preset disables Lumen in renderer"),Value(TEXT("r.DynamicGlobalIlluminationMethod")),0);
    TestEqual(TEXT("Preset disables hardware RT in renderer"),Value(TEXT("r.RayTracing.ForceAllRayTracingEffects")),0);
    TestEqual(TEXT("Preset selects temporal AA"),Value(TEXT("r.AntiAliasingMethod")),2);
    TestEqual(TEXT("Preset disables motion blur"),Value(TEXT("r.MotionBlurQuality")),0);
    TestEqual(TEXT("Preset disables depth of field"),Value(TEXT("r.DepthOfFieldQuality")),0);
    ERPerformance::FSettings Preset; Preset.Load();
    TestEqual(TEXT("Preset is saved"),Preset.HardwareRT,0);
    Loaded.SetDefaults();
    TestEqual(TEXT("Default restores original renderer AA"),Value(TEXT("r.AntiAliasingMethod")),OriginalAA);
    TestEqual(TEXT("Default restores original texture bias"),IConsoleManager::Get().FindConsoleVariable(TEXT("r.Streaming.MipBias"))->GetFloat(),OriginalMipBias);
    TestEqual(TEXT("Default restores original shadow quality"),Value(TEXT("sg.ShadowQuality")),OriginalShadowQuality);
    ERPerformance::FSettings Restored; Restored.Load();
    TestEqual(TEXT("Default choice is saved"),Restored.Defaults,1);
    TestEqual(TEXT("Default restores automatic resolution"),Restored.Automatic,1);
    GConfig->UnloadFile(TestIni);
    IFileManager::Get().Delete(*TestIni);
    return true;
}
#endif
