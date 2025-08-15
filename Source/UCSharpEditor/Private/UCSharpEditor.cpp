#include "UCSharpEditor.h"
#include "UCSharp.h"
#include "Core.h"
#include "Modules/ModuleManager.h"
#include "LevelEditor.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "Styling/AppStyle.h"
#include "Interfaces/IPluginManager.h"
#include "DesktopPlatformModule.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

DEFINE_LOG_CATEGORY(LogUCSharpEditor);

#define LOCTEXT_NAMESPACE "FUCSharpEditorModule"

//////////////////////////////////////////////////////////////////////////
// FUCSharpEditorCommands

FUCSharpEditorCommands::FUCSharpEditorCommands()
	: TCommands<FUCSharpEditorCommands>(
		TEXT("UCSharpEditor"),
		NSLOCTEXT("Contexts", "UCSharpEditor", "UCSharp Editor"),
		NAME_None,
		FAppStyle::GetAppStyleSetName())
{
}

void FUCSharpEditorCommands::RegisterCommands()
{
	UI_COMMAND(OpenCSharpEditor, "Open C# Editor", "Open the C# script editor", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(CreateCSharpScript, "Create C# Script", "Create a new C# script", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(BuildCSharpScripts, "Build C# Scripts", "Build all C# scripts in the project", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ReloadCSharpAssembly, "Reload C# Assembly", "Reload the C# assembly", EUserInterfaceActionType::Button, FInputChord());
}

//////////////////////////////////////////////////////////////////////////
// FUCSharpEditorModule

void FUCSharpEditorModule::StartupModule()
{
	UE_LOG(LogUCSharpEditor, Log, TEXT("UCSharp Editor module starting up..."));

	// Initialize editor UI
	InitializeEditorUI();

	UE_LOG(LogUCSharpEditor, Log, TEXT("UCSharp Editor module started successfully"));
}

void FUCSharpEditorModule::ShutdownModule()
{
	UE_LOG(LogUCSharpEditor, Log, TEXT("UCSharp Editor module shutting down..."));

	// Shutdown editor UI
	ShutdownEditorUI();
}

FUCSharpEditorModule& FUCSharpEditorModule::Get()
{
	return FModuleManager::LoadModuleChecked<FUCSharpEditorModule>("UCSharpEditor");
}

bool FUCSharpEditorModule::IsAvailable()
{
	return FModuleManager::Get().IsModuleLoaded("UCSharpEditor");
}

void FUCSharpEditorModule::InitializeEditorUI()
{
	// Register commands
	FUCSharpEditorCommands::Register();

	// Create command list
	CommandList = MakeShareable(new FUICommandList);

	// Bind commands
	CommandList->MapAction(
		FUCSharpEditorCommands::Get().OpenCSharpEditor,
		FExecuteAction::CreateRaw(this, &FUCSharpEditorModule::OnOpenCSharpEditor),
		FCanExecuteAction());

	CommandList->MapAction(
		FUCSharpEditorCommands::Get().CreateCSharpScript,
		FExecuteAction::CreateRaw(this, &FUCSharpEditorModule::OnCreateCSharpScript),
		FCanExecuteAction());

	CommandList->MapAction(
		FUCSharpEditorCommands::Get().BuildCSharpScripts,
		FExecuteAction::CreateRaw(this, &FUCSharpEditorModule::OnBuildCSharpScripts),
		FCanExecuteAction());

	CommandList->MapAction(
		FUCSharpEditorCommands::Get().ReloadCSharpAssembly,
		FExecuteAction::CreateRaw(this, &FUCSharpEditorModule::OnReloadCSharpAssembly),
		FCanExecuteAction());

	// Register menu extensions
	RegisterMenuExtensions();
}

void FUCSharpEditorModule::ShutdownEditorUI()
{
	// Unregister menu extensions
	UnregisterMenuExtensions();

	// Unregister commands
	FUCSharpEditorCommands::Unregister();

	// Clear command list
	CommandList.Reset();
}

void FUCSharpEditorModule::RegisterMenuExtensions()
{
	// Extend the main menu
	UToolMenus* ToolMenus = UToolMenus::Get();
	if (ToolMenus)
	{
		// Add to Tools menu
		UToolMenu* ToolsMenu = ToolMenus->ExtendMenu("LevelEditor.MainMenu.Tools");
		if (ToolsMenu)
		{
			FToolMenuSection& Section = ToolsMenu->FindOrAddSection("Programming");
			Section.AddSubMenu(
				"UCSharp",
				LOCTEXT("UCSharpMenu", "C# Scripting"),
				LOCTEXT("UCSharpMenuTooltip", "C# scripting tools and utilities"),
				FNewToolMenuDelegate::CreateRaw(this, &FUCSharpEditorModule::CreateCSharpMenuEntries)
			);
		}
	}
}

void FUCSharpEditorModule::UnregisterMenuExtensions()
{
	// Menu extensions are automatically cleaned up when the module shuts down
}

void FUCSharpEditorModule::CreateCSharpMenuEntries(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.BeginSection("UCSharpActions", LOCTEXT("UCSharpActionsSection", "C# Actions"));
	{
		MenuBuilder.AddMenuEntry(FUCSharpEditorCommands::Get().OpenCSharpEditor);
		MenuBuilder.AddMenuEntry(FUCSharpEditorCommands::Get().CreateCSharpScript);
		MenuBuilder.AddSeparator();
		MenuBuilder.AddMenuEntry(FUCSharpEditorCommands::Get().BuildCSharpScripts);
		MenuBuilder.AddMenuEntry(FUCSharpEditorCommands::Get().ReloadCSharpAssembly);
	}
	MenuBuilder.EndSection();
}

void FUCSharpEditorModule::OnOpenCSharpEditor()
{
	UE_LOG(LogUCSharpEditor, Log, TEXT("Opening C# Editor..."));

	// Show notification
	FNotificationInfo Info(LOCTEXT("OpenCSharpEditor", "Opening C# Editor..."));
	Info.ExpireDuration = 3.0f;
	FSlateNotificationManager::Get().AddNotification(Info);

	// TODO: Implement C# editor opening logic
	// This could open an external IDE or an integrated editor
}

void FUCSharpEditorModule::OnCreateCSharpScript()
{
	UE_LOG(LogUCSharpEditor, Log, TEXT("Creating new C# Script..."));

	// Show notification
	FNotificationInfo Info(LOCTEXT("CreateCSharpScript", "Creating new C# Script..."));
	Info.ExpireDuration = 3.0f;
	FSlateNotificationManager::Get().AddNotification(Info);

	// TODO: Implement script creation dialog
	// This could show a dialog to create new C# script files
}

void FUCSharpEditorModule::OnBuildCSharpScripts()
{
	UE_LOG(LogUCSharpEditor, Log, TEXT("Building C# Scripts..."));

	// Show notification
	FNotificationInfo Info(LOCTEXT("BuildCSharpScripts", "Building C# Scripts..."));
	Info.ExpireDuration = 3.0f;
	FSlateNotificationManager::Get().AddNotification(Info);

	// TODO: Implement C# build system
	// This would compile all C# scripts in the project
}

void FUCSharpEditorModule::OnReloadCSharpAssembly()
{
	UE_LOG(LogUCSharpEditor, Log, TEXT("Reloading C# Assembly..."));

	// Check if UCSharp runtime is available
	if (!FUCSharpModule::IsAvailable())
	{
		UE_LOG(LogUCSharpEditor, Warning, TEXT("UCSharp runtime is not available"));
		return;
	}

	FUCSharpModule& UCSharpModule = FUCSharpModule::Get();
	if (!UCSharpModule.IsCSharpRuntimeInitialized())
	{
		UE_LOG(LogUCSharpEditor, Warning, TEXT("C# runtime is not initialized"));
		return;
	}

	// Show notification
	FNotificationInfo Info(LOCTEXT("ReloadCSharpAssembly", "Reloading C# Assembly..."));
	Info.ExpireDuration = 3.0f;
	FSlateNotificationManager::Get().AddNotification(Info);

	// TODO: Implement assembly reload logic
	// This would reload the compiled C# assembly
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FUCSharpEditorModule, UCSharpEditor)