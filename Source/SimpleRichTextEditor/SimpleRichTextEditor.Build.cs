// Copyright UnrealEssence. All Rights Reserved.

using UnrealBuildTool;

public class SimpleRichTextEditor : ModuleRules
{
    public SimpleRichTextEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "SimpleRichText"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "InputCore",
                "UMG",
                "PropertyEditor",
            }
        );
    }
}