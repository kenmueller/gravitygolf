using UnrealBuildTool;

public class GravityGolfEditorTarget : TargetRules
{
	public GravityGolfEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("GravityGolf");
	}
}
