using UnrealBuildTool;

public class UCSharp : ModuleRules
{
	public UCSharp(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
		);

		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
		);

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"Projects"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Slate",
				"SlateCore",
				"ToolMenus",
				"EditorStyle",
				"EditorWidgets",
				"UnrealEd",
				"PropertyEditor"
			}
		);

		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
		);

		// .NET Core hosting support (temporarily disabled for initial build)
		// TODO: Re-enable after setting up proper .NET runtime libraries
		/*
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			// Add .NET Core runtime libraries
			PublicAdditionalLibraries.Add("nethost.lib");
			PublicDelayLoadDLLs.Add("hostfxr.dll");
			RuntimeDependencies.Add("$(TargetOutputDir)/hostfxr.dll");
		}
		*/
	}
}