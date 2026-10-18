using UnrealBuildTool;
public class RespawnAndRageEditorTarget : TargetRules
{
    public RespawnAndRageEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        ExtraModuleNames.Add("fdm");
        string Python = BuildHostPlatform.Current.Platform == UnrealTargetPlatform.Win64 ? "python" : "python3";
        PreBuildSteps.Add(Python + " \"$(ProjectDir)/scripts/stamp-build-version.py\"");
    }
}
