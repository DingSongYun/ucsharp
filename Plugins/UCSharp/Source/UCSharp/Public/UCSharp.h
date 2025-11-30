#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "UCSharpInterop.h"

/**
 * UCSharp Module Interface
 * Main module for C# scripting support in Unreal Engine 5
 */
class UCSHARP_API IUCSharpModule : public IModuleInterface
{
public:
	/**
	 * Get the UCSharp module instance
	 */
	static IUCSharpModule& Get();

	/**
	 * Check if the UCSharp module is loaded
	 */
	static bool IsAvailable();

	/**
	 * Get the plugin name
	 */
	static FName GetPluginName();

	/**
	 * Check if C# runtime is initialized
	 */
	virtual bool IsCSharpRuntimeInitialized() const = 0;

protected:
	/**
	 * Initialize the C# runtime
	 */
	virtual bool InitializeCSharpRuntime() = 0;

	/**
	 * Shutdown the C# runtime
	 */
	virtual void ShutdownCSharpRuntime() = 0;
};
