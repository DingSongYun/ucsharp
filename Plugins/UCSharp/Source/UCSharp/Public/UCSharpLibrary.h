

#pragma once

#include "CoreMinimal.h"

/**
 * UCSharpLibrary
 * A utility class for UCSharp plugin
 */
class UCSHARP_API UCSharpLibrary
{
public:
	static TSharedPtr<IPlugin> GetPlugin();
	static FString GetPluginDirectory();
	static FString GetConfigDirectory();
	static FString GetRuntimeConfigPath();
};
