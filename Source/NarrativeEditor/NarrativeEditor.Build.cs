// Copyright XiaoYao

using UnrealBuildTool;

public class NarrativeEditor : ModuleRules
{
	public NarrativeEditor(ReadOnlyTargetRules target) : base(target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"EditorSubsystem",
			"Narrative",
			"MessageLog"
		});

		PrivateDependencyModuleNames.AddRange(new[]
		{
			"ApplicationCore",
			"AssetSearch",
			"AssetTools",
			"BlueprintGraph",
			"ClassViewer",
			"ContentBrowser",
			"Core",
			"CoreUObject",
			"DetailCustomizations",
			"DeveloperSettings",
			"EditorFramework",
			"EditorScriptingUtilities",
			"EditorStyle",
			"Engine",
			"GraphEditor",
			"InputCore",
			"Json",
			"JsonUtilities",
			"Kismet",
			"KismetWidgets",
			"LevelEditor",
			"LevelSequence",
			"MovieScene",
			"MovieSceneTools",
			"MovieSceneTracks",
			"Projects",
			"PropertyEditor",
			"PropertyPath",
			"RenderCore",
			"Sequencer",
			"Slate",
			"SlateCore",
			"SourceControl",
			"ToolMenus",
			"UnrealEd"
		});
	}
}