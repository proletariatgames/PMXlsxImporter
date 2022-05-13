// Copyright 2022 Proletariat, Inc.

using System.IO;
using UnrealBuildTool;

public class PMXlsxImporter : ModuleRules
{
    public PMXlsxImporter(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		bEnforceIWYU = true;

        PublicIncludePaths.AddRange(
            new string[] {
                // required by https://www.unrealengine.com/en-US/marketplace-guidelines 2.6.3.e
                Path.Combine(ModuleDirectory, "Public")
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
				// ... add other public dependencies that you statically link with here ...
			}
            );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Projects",
                "InputCore",
                "UnrealEd",
                "ToolMenus",
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "DeveloperSettings",
                "EditorScriptingUtilities",
				"PropertyEditor",
				"Blutility",
				"UMG",
				"UMGEditor",
				"SourceControl"
				// ... add private dependencies that you statically link with here ...	
			}
            );


        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
				// ... add any modules that your module loads dynamically here ...
			}
            );
    }
}
