#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Framework/Commands/Commands.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

DECLARE_LOG_CATEGORY_EXTERN(LogUCSharpEditor, Log, All);

/**
 * UCSharp Editor Commands
 */
class FUCSharpEditorCommands : public TCommands<FUCSharpEditorCommands>
{
public:
	FUCSharpEditorCommands();

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	/** Open C# Script Editor */
	TSharedPtr<FUICommandInfo> OpenCSharpEditor;

	/** Create New C# Script */
	TSharedPtr<FUICommandInfo> CreateCSharpScript;

	/** Build C# Scripts */
	TSharedPtr<FUICommandInfo> BuildCSharpScripts;

	/** Reload C# Assembly */
	TSharedPtr<FUICommandInfo> ReloadCSharpAssembly;
};

/**
 * UCSharp Editor Module Interface
 * Editor module for C# scripting support in Unreal Engine 5
 */
class FUCSharpEditorModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/**
	 * Get the UCSharp Editor module instance
	 */
	static FUCSharpEditorModule& Get();

	/**
	 * Check if the UCSharp Editor module is loaded
	 */
	static bool IsAvailable();

private:
	/** Initialize editor UI */
	void InitializeEditorUI();

	/** Shutdown editor UI */
	void ShutdownEditorUI();

	/** Register menu extensions */
	void RegisterMenuExtensions();

	/** Unregister menu extensions */
	void UnregisterMenuExtensions();

	/** Create C# menu entries */
	void CreateCSharpMenuEntries(FMenuBuilder& MenuBuilder);

	/** Command callbacks */
	void OnOpenCSharpEditor();
	void OnCreateCSharpScript();
	void OnBuildCSharpScripts();
	void OnReloadCSharpAssembly();

private:
	/** Command list for C# editor actions */
	TSharedPtr<FUICommandList> CommandList;

	/** Menu extender for adding C# options */
	TSharedPtr<FExtender> MenuExtender;
};