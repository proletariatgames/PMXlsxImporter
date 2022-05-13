// Copyright 2022 Proletariat, Inc.

#include "PMXlsxImporterImportSelectionWindow.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/Button.h"
#include "PMXlsxImporterSettings.h"
#include "PMXlsxImporterLog.h"
#include "PMXlsxImporterContextLogger.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "EditorUtilitySubsystem.h"

void UPMXlsxImporterImportSelectionWindow::Open()
{
	const TCHAR* WINDOW_PATH = TEXT("/PMXlsxImporter/ImportXlsxWindow.ImportXlsxWindow");
	UEditorUtilityWidgetBlueprint* Window = LoadObject<UEditorUtilityWidgetBlueprint>(nullptr, WINDOW_PATH);
	if (Window == nullptr)
	{
		UE_LOG(LogPMXlsxImporter, Error, TEXT("Could not open import selection window at %s"), WINDOW_PATH);
	}
	else
	{
		UEditorUtilitySubsystem* EditorUtilitySubsystem = GEditor->GetEditorSubsystem<UEditorUtilitySubsystem>();
		EditorUtilitySubsystem->SpawnAndRegisterTab(Window);
	}
}

void UPMXlsxImporterImportSelectionWindow::NativeConstruct()
{
	CheckedOutOption->OnCheckStateChanged.AddDynamic(this, &UPMXlsxImporterImportSelectionWindow::OnCheckedOutOptionClicked);
	AllFilesOption->OnCheckStateChanged.AddDynamic(this, &UPMXlsxImporterImportSelectionWindow::OnAllFilesOptionClicked);
	OneWorksheetOption->OnCheckStateChanged.AddDynamic(this, &UPMXlsxImporterImportSelectionWindow::OnOneWorksheetOptionClicked);
	WorksheetSelector->OnOpening.AddDynamic(this, &UPMXlsxImporterImportSelectionWindow::OnWorksheetSelectorOpened);
	ImportButton->OnClicked.AddDynamic(this, &UPMXlsxImporterImportSelectionWindow::OnImportButtonClicked);

	PopulateWorksheetSelector();

	OnOptionClicked(CheckedOutOption);
}

// TODO call this whenever UPMXlsxImporterSettings changes
void UPMXlsxImporterImportSelectionWindow::PopulateWorksheetSelector()
{
	WorksheetSelector->ClearSelection();
	WorksheetSelector->ClearOptions();

	const UPMXlsxImporterSettings* SettingsCDO = GetDefault<UPMXlsxImporterSettings>();
	for (const FPMXlsxImporterSettingsEntry& Entry : SettingsCDO->AssetImportSettings)
	{
		WorksheetSelector->AddOption(FString::Printf(TEXT("%s:%s → %s"), *Entry.XlsxFile.FilePath, *Entry.WorksheetName, *Entry.OutputDir.Path));
	}

	if (WorksheetSelector->GetOptionCount() > 0)
	{
		WorksheetSelector->SetSelectedIndex(0);
	}
}

void UPMXlsxImporterImportSelectionWindow::OnCheckedOutOptionClicked(bool bSelected)
{
	OnOptionClicked(CheckedOutOption);
}

void UPMXlsxImporterImportSelectionWindow::OnAllFilesOptionClicked(bool bSelected)
{
	OnOptionClicked(AllFilesOption);
}

void UPMXlsxImporterImportSelectionWindow::OnOneWorksheetOptionClicked(bool bSelected)
{
	OnOptionClicked(OneWorksheetOption);
}

void UPMXlsxImporterImportSelectionWindow::OnOptionClicked(UCheckBox* Option)
{
	CheckedOutOption->SetIsChecked(Option == CheckedOutOption);
	AllFilesOption->SetIsChecked(Option == AllFilesOption);
	OneWorksheetOption->SetIsChecked(Option == OneWorksheetOption);
}

void UPMXlsxImporterImportSelectionWindow::OnWorksheetSelectorOpened()
{
	OnOptionClicked(OneWorksheetOption);
}

void UPMXlsxImporterImportSelectionWindow::OnImportButtonClicked()
{
	const UPMXlsxImporterSettings* SettingsCDO = GetDefault<UPMXlsxImporterSettings>();
	FPMXlsxImporterContextLogger Errors;

	if (CheckedOutOption->IsChecked())
	{
		SettingsCDO->ImportCheckedOut(Errors);
	}
	else if (AllFilesOption->IsChecked())
	{
		SettingsCDO->ImportAll(Errors);
	}
	else if (OneWorksheetOption->IsChecked())
	{
		int SelectedIndex = WorksheetSelector->GetSelectedIndex();
		SettingsCDO->ImportEntry(SelectedIndex, Errors);
	}
	else
	{
		UE_LOG(LogPMXlsxImporter, Error, TEXT("No import option checked"));
	}

	UE_LOG(LogPMXlsxImporter, Log, TEXT("Import run completed with %i errors"), Errors.Num());
	Errors.Flush();
}
