using UnrealBuildTool;

public class MeshForgeTrellisToolset : ModuleRules
{
	public MeshForgeTrellisToolset(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"MeshForge",           // FMeshProviderCaps, for reporting readiness
				"MeshForgeTrellis",    // the provider and its container control
				"ToolsetRegistry",
			}
			);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"UnrealEd",
			}
			);
	}
}
