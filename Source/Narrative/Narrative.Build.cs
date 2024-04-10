// Copyright XiaoYao

using UnrealBuildTool;

public class Narrative : ModuleRules
{
	public Narrative(ReadOnlyTargetRules target) : base(target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"LevelSequence"
		});

		PrivateDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"DeveloperSettings",
			"Engine",
			"GameplayTags",
			"MovieScene",
			"MovieSceneTracks",
			"Slate",
			"SlateCore"
		});

		if (target.Type == TargetType.Editor)
		{
			PublicDependencyModuleNames.AddRange(new[]
			{
				"MessageLog",
				"UnrealEd"
			});
		}
	}
}