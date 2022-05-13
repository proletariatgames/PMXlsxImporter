// Copyright 2022 Proletariat, Inc.

#include "PMXlsxImporterSettings.h"
#include "PMXlsxImporterSettingsEntry.h"
#include "PMXlsxImporterPythonBridge.h"
#include "PMXlsxImporterLog.h"
#include "Containers/List.h"

#if WITH_EDITOR
void UPMXlsxImporterSettings::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	// Unlike PostEditChangeProperty, this function gives us the entire chain of properties:
	// this.AssetImportSettings[index].XlsxFile.FilePath

	// Save this off so that we know which worksheet to read from when GetWorksheetNames() is called
	LastEditedSettingsIndex = PropertyChangedEvent.GetArrayIndex(TEXT("AssetImportSettings"));
	if (!AssetImportSettings.IsValidIndex(LastEditedSettingsIndex))
	{
		return;
	}

	FPMXlsxImporterSettingsEntry& Entry = AssetImportSettings[LastEditedSettingsIndex];
	// GetActiveMemberNode corresponds to a property on this (probably AssetImportSettings).
	// GetNextNode() corresponds to a property on Entry.
	auto* Node = PropertyChangedEvent.PropertyChain.GetActiveMemberNode()->GetNextNode();
	if (Node == nullptr)
	{
		return;
	}

	FPropertyChangedEvent EntryEvent = FPropertyChangedEvent(Node->GetValue());
	Entry.PostEditChangeProperty(EntryEvent);
}
#endif

void UPMXlsxImporterSettings::ImportCheckedOut(FPMXlsxImporterContextLogger& InOutErrors) const
{
	// First, create all autogenerated objects so that they can reference each other
	for (const FPMXlsxImporterSettingsEntry& AssetImportData : AssetImportSettings)
	{
		FSourceControlState State = AssetImportData.GetXlsxFileSourceControlState(/*bSilent:*/ false);
		if (State.bIsValid && State.bIsCheckedOut)
		{
			UE_LOG(LogPMXlsxImporter, Log, TEXT("File %s is checked out"), *AssetImportData.XlsxFile.FilePath);
			AssetImportData.SyncAssets(InOutErrors, MaxErrors);
			if (InOutErrors.Num() >= MaxErrors)
			{
				return;
			}
		}
		else
		{
			UE_LOG(LogPMXlsxImporter, Verbose, TEXT("File %s is NOT checked out. Skipping."), *AssetImportData.XlsxFile.FilePath);
		}
	}

	// Then get each of them to parse data from xlsx
	for (const FPMXlsxImporterSettingsEntry& AssetImportData : AssetImportSettings)
	{
		FSourceControlState State = AssetImportData.GetXlsxFileSourceControlState(/*bSilent:*/ true);
		if (State.bIsValid && State.bIsCheckedOut)
		{
			AssetImportData.ParseData(InOutErrors, MaxErrors);
			if (InOutErrors.Num() >= MaxErrors)
			{
				return;
			}
		}
	}

	// Then validate the data
	for (const FPMXlsxImporterSettingsEntry& AssetImportData : AssetImportSettings)
	{
		FSourceControlState State = AssetImportData.GetXlsxFileSourceControlState(/*bSilent:*/ true);
		if (State.bIsValid && State.bIsCheckedOut)
		{
			AssetImportData.Validate(InOutErrors, MaxErrors);
			if (InOutErrors.Num() >= MaxErrors)
			{
				return;
			}
		}
	}
}

void UPMXlsxImporterSettings::ImportAll(FPMXlsxImporterContextLogger& InOutErrors) const
{
	UE_LOG(LogPMXlsxImporter, Log, TEXT("Importing all XLSX files"));

	// First, create all autogenerated objects so that they can reference each other
	for (const FPMXlsxImporterSettingsEntry& AssetImportData : AssetImportSettings)
	{
		AssetImportData.SyncAssets(InOutErrors, MaxErrors);
		if (InOutErrors.Num() >= MaxErrors)
		{
			return;
		}
	}

	// Then get each of them to parse data from xlsx
	for (const FPMXlsxImporterSettingsEntry& AssetImportData : AssetImportSettings)
	{
		AssetImportData.ParseData(InOutErrors, MaxErrors);
		if (InOutErrors.Num() >= MaxErrors)
		{
			return;
		}
	}

	// Then validate the data
	for (const FPMXlsxImporterSettingsEntry& AssetImportData : AssetImportSettings)
	{
		AssetImportData.Validate(InOutErrors, MaxErrors);
		if (InOutErrors.Num() >= MaxErrors)
		{
			return;
		}
	}
}

void UPMXlsxImporterSettings::ImportEntry(int32 Index, FPMXlsxImporterContextLogger& InOutErrors) const
{
	UE_LOG(LogPMXlsxImporter, Log, TEXT("Importing entry %i"), Index);

	if (!AssetImportSettings.IsValidIndex(Index))
	{
		InOutErrors.Logf(TEXT("Invalid index %i"), Index);
		return;
	}

	const FPMXlsxImporterSettingsEntry& Entry = AssetImportSettings[Index];

	// First, create all autogenerated objects so that they can reference each other
	Entry.SyncAssets(InOutErrors, MaxErrors);
	if (InOutErrors.Num() >= MaxErrors)
	{
		return;
	}

	// Then get each of them to parse data from xlsx
	Entry.ParseData(InOutErrors, MaxErrors);
	if (InOutErrors.Num() >= MaxErrors)
	{
		return;
	}

	// Then validate the data
	Entry.Validate(InOutErrors, MaxErrors);
}

TArray<FString> UPMXlsxImporterSettings::GetWorksheetNames() const
{
	// Assume that we're getting the names of the last entry to be edited.
	// It's possible that we're getting the names of a different entry,
	// but there's no way to tell that from here due to how this function gets called
	// from generated code (see meta = (GetOptions="GetWorksheetNames") in FPMXlsxImporterSettingsEntry)
	// If the user picks an invalid worksheet name, it'll get deleted in FPMXlsxImporterSettingsEntry::PostEditChangeProperty
	// and we'll get the list right for next time.
	if (AssetImportSettings.IsValidIndex(LastEditedSettingsIndex))
	{
		return AssetImportSettings[LastEditedSettingsIndex].GetWorksheetNames();
	}

	return TArray<FString>();
}
