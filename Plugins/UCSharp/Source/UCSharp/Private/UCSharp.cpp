#include "UCSharp.h"
#include "Core.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/Paths.h"

// Temporarily disabled .NET includes for initial build
// TODO: Re-enable after setting up proper .NET SDK
/*
#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <nethost.h>
#include <hostfxr.h>
#include <coreclr_delegates.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif
*/

DEFINE_LOG_CATEGORY(LogUCSharp);

#define LOCTEXT_NAMESPACE "FUCSharpModule"

void FUCSharpModule::StartupModule()
{
	UE_LOG(LogUCSharp, Log, TEXT("UCSharp module starting up..."));

	// Initialize C# runtime (temporarily disabled)
	// TODO: Re-enable after setting up proper .NET runtime libraries
	/*
	if (!InitializeCSharpRuntime())
	{
		UE_LOG(LogUCSharp, Error, TEXT("Failed to initialize C# runtime"));
	}
	else
	{
		UE_LOG(LogUCSharp, Log, TEXT("C# runtime initialized successfully"));
	}
	*/
	UE_LOG(LogUCSharp, Log, TEXT("UCSharp module loaded (C# runtime disabled for initial build)"));
}

void FUCSharpModule::ShutdownModule()
{
	UE_LOG(LogUCSharp, Log, TEXT("UCSharp module shutting down..."));

	// Shutdown C# runtime (temporarily disabled)
	// TODO: Re-enable after setting up proper .NET runtime libraries
	// ShutdownCSharpRuntime();
}

FUCSharpModule& FUCSharpModule::Get()
{
	return FModuleManager::LoadModuleChecked<FUCSharpModule>("UCSharp");
}

bool FUCSharpModule::IsAvailable()
{
	return FModuleManager::Get().IsModuleLoaded("UCSharp");
}

bool FUCSharpModule::InitializeCSharpRuntime()
{
	// Temporarily disabled .NET runtime initialization
	// TODO: Re-enable after setting up proper .NET SDK and headers
	UE_LOG(LogUCSharp, Warning, TEXT("C# runtime initialization temporarily disabled"));
	return false;
}

void FUCSharpModule::ShutdownCSharpRuntime()
{
#if PLATFORM_WINDOWS
	if (bCSharpRuntimeInitialized && RuntimeHandle)
	{
		// Close runtime handle
		// Note: In a full implementation, we would call hostfxr_close here
		RuntimeHandle = nullptr;
		bCSharpRuntimeInitialized = false;
		UE_LOG(LogUCSharp, Log, TEXT("C# runtime shutdown completed"));
	}
#endif
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FUCSharpModule, UCSharp)