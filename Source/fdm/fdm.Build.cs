using UnrealBuildTool;
public class fdm : ModuleRules
{
    public fdm(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new [] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG", "Slate", "SlateCore", "AIModule", "Niagara" });
        PrivateDependencyModuleNames.Add("RenderCore");
        PrivateDependencyModuleNames.Add("ApplicationCore");
        PrivateDependencyModuleNames.Add("PhysicsCore");
        if (Target.bBuildEditor) PrivateDependencyModuleNames.AddRange(new [] { "MeshDescription", "StaticMeshDescription", "SkeletalMeshDescription", "AnimationCore", "AssetRegistry" });
    }
}
