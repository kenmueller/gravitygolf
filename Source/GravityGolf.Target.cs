using UnrealBuildTool;

public class GravityGolfTarget : TargetRules
{
	public GravityGolfTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("GravityGolf");
	}
}
