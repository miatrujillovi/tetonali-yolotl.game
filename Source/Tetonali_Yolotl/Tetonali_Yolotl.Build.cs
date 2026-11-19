// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Tetonali_Yolotl : ModuleRules
{
	public Tetonali_Yolotl(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"NavigationSystem",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"Niagara",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"Tetonali_Yolotl",
			"Tetonali_Yolotl/Variant_Strategy",
			"Tetonali_Yolotl/Variant_Strategy/UI",
			"Tetonali_Yolotl/Variant_TwinStick",
			"Tetonali_Yolotl/Variant_TwinStick/AI",
			"Tetonali_Yolotl/Variant_TwinStick/Gameplay",
			"Tetonali_Yolotl/Variant_TwinStick/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
