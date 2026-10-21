using UnrealBuildTool;
public class fdm : ModuleRules
{
    public fdm(ReadOnlyTargetRules Target) : base(Target)
    {
        // Slate and the native Dock icon read these loose files at runtime.
        RuntimeDependencies.Add("$(ProjectDir)/Branding/background.jpg", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(ProjectDir)/Branding/icon.png", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(ProjectDir)/Branding/Credits.txt", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(ProjectDir)/Build/GameVersion.txt", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(ProjectDir)/Branding/Fonts/Teko/Fonts/Teko-SemiBold.ttf", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(ProjectDir)/Branding/Fonts/Teko/ThirdPartyLicenses/Teko-OFL.txt", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(ProjectDir)/Branding/Fonts/Teko/CREDITS.txt", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(ProjectDir)/Config/GameplayMessages/respawn-and-rage-gameplay-messages.en-US.json", StagedFileType.NonUFS);
        PrivateDependencyModuleNames.Add("Json");
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new [] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG", "Slate", "SlateCore", "AIModule", "Niagara" });
        PrivateDependencyModuleNames.AddRange(new [] { "RenderCore", "RHI", "Sockets", "Networking" });
        PrivateDependencyModuleNames.Add("ApplicationCore");
        PrivateDependencyModuleNames.Add("PhysicsCore");
        PrivateDependencyModuleNames.AddRange(new [] { "PCG", "PCGExCore" });
        PrivateDependencyModuleNames.AddRange(new [] { "GeometryCore", "GeometryFramework", "NetCore" });
        if (Target.bBuildEditor) PrivateDependencyModuleNames.AddRange(new [] { "MeshDescription", "StaticMeshDescription", "SkeletalMeshDescription", "AnimationCore", "AssetRegistry", "NiagaraEditor" });
    }
}
