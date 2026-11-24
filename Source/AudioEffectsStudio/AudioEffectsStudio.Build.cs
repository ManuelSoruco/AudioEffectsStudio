// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

using UnrealBuildTool;

public class AudioEffectsStudio : ModuleRules
{
	public AudioEffectsStudio(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"CoreUObject",
			"Engine",
			"Slate",
			"SlateCore",
			"InputCore",
			"UnrealEd",
			"PropertyEditor",
			"ToolMenus",
			"AudioWidgets",
			"AudioMixer",
			"SignalProcessing",
			"Synthesis",
			"AssetDefinition",
			"AssetTools",
			"ApplicationCore",
			"DeveloperSettings",
			"AssetRegistry",
			"Settings",
			"ContentBrowser",  // SyncBrowserToAssets after a preset is marked dirty
		});
	}
}
