using UnrealBuildTool;
using System.Collections.Generic;

public class UCSharpProjectEditorTarget : TargetRules
{
	public UCSharpProjectEditorTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		CppStandard = CppStandardVersion.Default;
		bOverrideBuildEnvironment = true;

		ExtraModuleNames.AddRange( new string[] { "UCSharpProject" } );
	}
}