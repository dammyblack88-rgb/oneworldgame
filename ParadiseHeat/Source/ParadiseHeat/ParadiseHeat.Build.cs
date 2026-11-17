// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ParadiseHeat : ModuleRules
{
	public ParadiseHeat(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"ChaosVehicles",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"SlateCore", "MediaAssets", "MoviePlayer", "AudioMixer"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });
        if (Target.Platform == UnrealTargetPlatform.Mac) PublicFrameworks.AddRange(new string[] { "AppKit", "UniformTypeIdentifiers" });

		PublicIncludePaths.AddRange(new string[] {
			"ParadiseHeat",
			"ParadiseHeat/Variant_Platforming",
			"ParadiseHeat/Variant_Platforming/Animation",
			"ParadiseHeat/Variant_Combat",
			"ParadiseHeat/Variant_Combat/AI",
			"ParadiseHeat/Variant_Combat/Animation",
			"ParadiseHeat/Variant_Combat/Gameplay",
			"ParadiseHeat/Variant_Combat/Interfaces",
			"ParadiseHeat/Variant_Combat/UI",
			"ParadiseHeat/Variant_SideScrolling",
			"ParadiseHeat/Variant_SideScrolling/AI",
			"ParadiseHeat/Variant_SideScrolling/Gameplay",
			"ParadiseHeat/Variant_SideScrolling/Interfaces",
			"ParadiseHeat/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "MediaAssets", "MoviePlayer", "AudioMixer" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
