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
			
			// Add menu entries directly
			Section.AddMenuEntry(
				"OpenCSharpEditor",
				LOCTEXT("OpenCSharpEditor", "Open C# Editor"),
				LOCTEXT("OpenCSharpEditorTooltip", "Open the C# script editor"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateRaw(this, &FUCSharpEditorModule::OnOpenCSharpEditor))
			);
			
			Section.AddMenuEntry(
				"CreateCSharpScript",
				LOCTEXT("CreateCSharpScript", "Create C# Script"),
				LOCTEXT("CreateCSharpScriptTooltip", "Create a new C# script"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateRaw(this, &FUCSharpEditorModule::OnCreateCSharpScript))
			);
		}
	}
}

void FUCSharpEditorModule::UnregisterMenuExtensions()
{
	// Menu extensions are automatically cleaned up when the module shuts down
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

	// Temporarily disabled - UCSharp runtime integration pending
	UE_LOG(LogUCSharpEditor, Warning, TEXT("C# assembly reload temporarily disabled"));

	// Show notification
	FNotificationInfo Info(LOCTEXT("ReloadCSharpAssembly", "C# Assembly reload temporarily disabled"));
	Info.ExpireDuration = 3.0f;
	FSlateNotificationManager::Get().AddNotification(Info);

	// TODO: Re-enable after UCSharp runtime is properly integrated
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FUCSharpEditorModule, UCSharpEditor)