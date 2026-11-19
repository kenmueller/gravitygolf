using UnrealBuildTool;

public class GravityGolf : ModuleRules
{
	public GravityGolf(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"Json",
			"ProceduralMeshComponent",
			"RenderCore",
			"RHI",
		});
	}
}
