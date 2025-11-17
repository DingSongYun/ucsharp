
#include "UCSharpLibrary.h"
#include "Interfaces/IPluginManager.h"
#include "UCSharpLogs.h"
#include "UCSharp.h"

TSharedPtr<IPlugin> UCSharpLibrary::GetPlugin()
{
	TSharedPtr<IPlugin> sPlugin = nullptr;
	if (!sPlugin.IsValid())
	{
		sPlugin = IPluginManager::Get().FindPlugin(IUCSharpModule::GetPluginName().ToString());
		if (!sPlugin.IsValid())
		{
			UE_LOG(LogUCSharp, Error, TEXT("Failed to find UCSharp plugin"));
			return nullptr;
		}
	}
	return sPlugin;
}

FString UCSharpLibrary::GetPluginDirectory()
{
	return GetPlugin()->GetBaseDir();
}

FString UCSharpLibrary::GetConfigDirectory()
{
	return FPaths::Combine(GetPluginDirectory(), TEXT("Config"));
}

FString UCSharpLibrary::GetRuntimeConfigPath()
{
	return FPaths::Combine(GetConfigDirectory(), TEXT("UCSharpRuntimeConfig.json"));
}
