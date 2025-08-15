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
#if PLATFORM_WINDOWS
	if (bCSharpRuntimeInitialized)
	{
		return true;
	}

	try
	{
		// Get plugin directory
		TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("UCSharp"));
		if (!Plugin.IsValid())
		{
			UE_LOG(LogUCSharp, Error, TEXT("UCSharp plugin not found"));
			return false;
		}

		FString PluginDir = Plugin->GetBaseDir();
		FString DotNetDir = FPaths::Combine(PluginDir, TEXT("Binaries"), TEXT("DotNet"));
		FString RuntimeConfigPath = FPaths::Combine(DotNetDir, TEXT("UCSharp.Managed.runtimeconfig.json"));

		// Convert to ANSI for .NET hosting APIs
		std::string RuntimeConfigPathAnsi = TCHAR_TO_UTF8(*RuntimeConfigPath);

		// Initialize .NET runtime
		char_t* dotnet_root = nullptr;
		char_t* dotnet_path = nullptr;
		int rc = get_hostfxr_path(nullptr, &dotnet_path, nullptr, nullptr);
		if (rc != 0)
		{
			UE_LOG(LogUCSharp, Error, TEXT("Failed to get hostfxr path. Error code: %d"), rc);
			return false;
		}

		// Load hostfxr library
		HMODULE hostfxr_lib = LoadLibraryA(dotnet_path);
		if (!hostfxr_lib)
		{
			UE_LOG(LogUCSharp, Error, TEXT("Failed to load hostfxr library"));
			return false;
		}

		// Get hostfxr functions
		hostfxr_initialize_for_runtime_config_fn init_fptr = 
			(hostfxr_initialize_for_runtime_config_fn)GetProcAddress(hostfxr_lib, "hostfxr_initialize_for_runtime_config");
		hostfxr_get_runtime_delegate_fn get_delegate_fptr = 
			(hostfxr_get_runtime_delegate_fn)GetProcAddress(hostfxr_lib, "hostfxr_get_runtime_delegate");
		hostfxr_close_fn close_fptr = 
			(hostfxr_close_fn)GetProcAddress(hostfxr_lib, "hostfxr_close");

		if (!init_fptr || !get_delegate_fptr || !close_fptr)
		{
			UE_LOG(LogUCSharp, Error, TEXT("Failed to get hostfxr function pointers"));
			FreeLibrary(hostfxr_lib);
			return false;
		}

		// Initialize runtime
		hostfxr_handle cxt = nullptr;
		rc = init_fptr(RuntimeConfigPathAnsi.c_str(), nullptr, &cxt);
		if (rc != 0 || cxt == nullptr)
		{
			UE_LOG(LogUCSharp, Error, TEXT("Failed to initialize .NET runtime. Error code: %d"), rc);
			FreeLibrary(hostfxr_lib);
			return false;
		}

		// Store runtime handle
		RuntimeHandle = cxt;
		bCSharpRuntimeInitialized = true;

		UE_LOG(LogUCSharp, Log, TEXT("C# runtime initialized successfully"));
		return true;
	}
	catch (...)
	{
		UE_LOG(LogUCSharp, Error, TEXT("Exception occurred during C# runtime initialization"));
		return false;
	}
#else
	UE_LOG(LogUCSharp, Warning, TEXT("C# runtime initialization not supported on this platform"));
	return false;
#endif
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