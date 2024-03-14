using UnrealBuildTool;

public class BasiliskEditor : ModuleRules
{
	public BasiliskEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
                "AssetTools",
                "AudioEditor",
                "Blutility",
                "ContentBrowser",
                "Core",
				"CoreUObject",
                "DesktopPlatform",
                "DirectoryWatcher",
                "EditorScriptingUtilities",
                "EditorStyle",
                "EditorSubsystem",
                "Engine",
                "HTTP",
                "IKRig",
                "IKRigEditor",
                "Json",
                "JsonUtilities",
                "LevelEditor",
                "Networking",
                "OSC",
                "Projects",
                "Slate",
				"SlateCore",
                "Sockets",
                "ToolMenus",
                "UnrealEd",
                "UMG",
                "UMGEditor"
            }
		);
    }
}