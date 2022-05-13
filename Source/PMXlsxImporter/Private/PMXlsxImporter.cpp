// Copyright 2022 Proletariat, Inc.

#include "PMXlsxImporter.h"
#include "PMXlsxImporterStyle.h"
#include "PMXlsxImporterCommands.h"
#include "PMXlsxImporterSettings.h"
#include "PMXlsxImporterSettingsEntry.h"
#include "PMXlsxImporterImportSelectionWindow.h"
#include "ToolMenus.h"

#define LOCTEXT_NAMESPACE "FPMXlsxImporterModule"

void FPMXlsxImporterModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FPMXlsxImporterStyle::Initialize();
	FPMXlsxImporterStyle::ReloadTextures();

	FPMXlsxImporterCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FPMXlsxImporterCommands::Get().PluginAction,
		FExecuteAction::CreateRaw(this, &FPMXlsxImporterModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FPMXlsxImporterModule::RegisterMenus));
}

void FPMXlsxImporterModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FPMXlsxImporterStyle::Shutdown();

	FPMXlsxImporterCommands::Unregister();
}

void FPMXlsxImporterModule::PluginButtonClicked()
{
	UPMXlsxImporterImportSelectionWindow::Open();
}

void FPMXlsxImporterModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);
	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FPMXlsxImporterCommands::Get().PluginAction, PluginCommands);
		}
	}

#if ENGINE_MAJOR_VERSION == 4
	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("Settings");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FPMXlsxImporterCommands::Get().PluginAction));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
#elif ENGINE_MAJOR_VERSION == 5
	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("PluginTools");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FPMXlsxImporterCommands::Get().PluginAction));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
#else
#	error Unknown engine version
#endif
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FPMXlsxImporterModule, PMXlsxImporter)
