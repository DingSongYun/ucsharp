#include "UCSharp.h"
#include "Core.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/Paths.h"
#include "UCSharpRuntime.h"
#include "UCSharpLogs.h"

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

#define LOCTEXT_NAMESPACE "FUCSharpModule"


class FUCSharpModule : public IUCSharpModule
{

private:
	/** Whether the C# runtime has been initialized */
	bool bCSharpRuntimeInitialized = false;

	FUCSharpRuntime RuntimeHandle;

public:
	virtual void StartupModule() override
	{
		UE_LOG(LogUCSharp, Log, TEXT("UCSharp module starting up..."));

		// Initialize C# runtime (temporarily disabled)
		if (!InitializeCSharpRuntime())
		{
			UE_LOG(LogUCSharp, Error, TEXT("Failed to initialize C# runtime"));
		}
		else
		{
			UE_LOG(LogUCSharp, Log, TEXT("C# runtime initialized successfully"));
		}
		UE_LOG(LogUCSharp, Log, TEXT("UCSharp module loaded"));
	}

	virtual void ShutdownModule() override
	{
		UE_LOG(LogUCSharp, Log, TEXT("UCSharp module shutting down..."));

		UE_LOG(LogUCSharp, Log, TEXT("UCSharp interop system shut down"));

		// Shutdown C# runtime (temporarily disabled)
		ShutdownCSharpRuntime();
	}

	virtual bool IsCSharpRuntimeInitialized() const
	{
		return bCSharpRuntimeInitialized;
	}

	protected:
	virtual bool InitializeCSharpRuntime() override
	{
		// 调用运行时初始化并根据结果设置状态
		const bool bInitOk = RuntimeHandle.Initialize();
		bCSharpRuntimeInitialized = bInitOk;
		return bInitOk;
	}

	virtual void ShutdownCSharpRuntime() override
	{
		if (bCSharpRuntimeInitialized)
		{
			// Close runtime handle
			bCSharpRuntimeInitialized = false;
			RuntimeHandle.Shutdown();
			UE_LOG(LogUCSharp, Log, TEXT("C# runtime shutdown completed"));
		}
	}
}; // End of FUCSharpModule

bool IUCSharpModule::IsAvailable()
{
    return FModuleManager::Get().IsModuleLoaded("UCSharp");
}

IUCSharpModule& IUCSharpModule::Get()
{
    return FModuleManager::LoadModuleChecked<FUCSharpModule>("UCSharp");
}

static FName GetPluginName()
{
	return TEXT("UCSharp");
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FUCSharpModule, UCSharp)
