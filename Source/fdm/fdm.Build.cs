using UnrealBuildTool;
public class fdm : ModuleRules
{
    public fdm(ReadOnlyTargetRules Target) : base(Target)
    {
        // Slate and the native Dock icon read these loose files at runtime.
        RuntimeDependencies.Add("$(ProjectDir)/Branding/background.jpg", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(ProjectDir)/Branding/icon.png", StagedFileType.NonUFS);
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new [] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG", "Slate", "SlateCore", "AIModule", "Niagara" });
        PrivateDependencyModuleNames.Add("RenderCore");
        PrivateDependencyModuleNames.Add("ApplicationCore");
        PrivateDependencyModuleNames.Add("PhysicsCore");
        PrivateDependencyModuleNames.AddRange(new [] { "GeometryCore", "GeometryFramework", "NetCore" });
        if (Target.bBuildEditor) PrivateDependencyModuleNames.AddRange(new [] { "MeshDescription", "StaticMeshDescription", "SkeletalMeshDescription", "AnimationCore", "AssetRegistry", "NiagaraEditor" });
    }
}
