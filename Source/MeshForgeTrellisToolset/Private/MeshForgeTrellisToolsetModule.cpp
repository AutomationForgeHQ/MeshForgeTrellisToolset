#include "MeshForgeTrellisToolset.h"

#include "Modules/ModuleManager.h"

/**
 * Registers the TRELLIS.2 runner toolset.
 *
 * One line per toolset, and it is explicit: a class that is written, compiles and is never named
 * here does not exist as far as an agent is concerned, with no warning anywhere.
 */
class FMeshForgeTrellisToolsetModule : public IModuleInterface
{
public:

	virtual void StartupModule() override
	{
		// Guarded because ToolsetRegistry is an Experimental engine plugin and this module can load
		// in a project where it is switched off.
		if (UToolsetRegistry::IsAvailable())
		{
			UToolsetRegistry::RegisterToolsetClass(UMeshForgeTrellisToolset::StaticClass());
		}
	}

	virtual void ShutdownModule() override
	{
		if (UToolsetRegistry::IsAvailable())
		{
			UToolsetRegistry::UnregisterToolsetClass(UMeshForgeTrellisToolset::StaticClass());
		}
	}
};

IMPLEMENT_MODULE(FMeshForgeTrellisToolsetModule, MeshForgeTrellisToolset)
