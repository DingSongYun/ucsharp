#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogUCSharp, Log, All);

/**
 * UCSharp Module Interface
 * Main module for C# scripting support in Unreal Engine 5
 */
class FUCSharpModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/**
	 * Get the UCSharp module instance
	 */
	static FUCSharpModule& Get();

	/**
	 * Check if the UCSharp module is loaded
	 */
	static bool IsAvailable();

	/**
	 * Initialize the C# runtime
	 */
	bool InitializeCSharpRuntime();

	/**
	 * Shutdown the C# runtime
	 */
	void ShutdownCSharpRuntime();

	/**
	 * Check if C# runtime is initialized
	 */
	bool IsCSharpRuntimeInitialized() const { return bCSharpRuntimeInitialized; }

private:
	/** Whether the C# runtime has been initialized */
	bool bCSharpRuntimeInitialized = false;

	/** Handle to the loaded .NET runtime */
	void* RuntimeHandle = nullptr;
};