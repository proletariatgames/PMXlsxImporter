// Copyright 2022 Proletariat, Inc.

#pragma once

#include "Engine/DeveloperSettings.h"
#include "PMXlsxImporterSettingsEntry.h"
#include "PMXlsxImporterContextLogger.h"
#include "PMXlsxImporterSettings.generated.h"

UCLASS(config = Plugins, defaultconfig, DisplayName="XLSX Import Settings")
class PMXLSXIMPORTER_API UPMXlsxImporterSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
	virtual FName GetSectionName() const override { return TEXT("XLSX Import"); }
#if WITH_EDITOR
	virtual FText GetSectionText() const override { return FText::FromString(TEXT("XLSX Import")); }
	virtual FText GetSectionDescription() const override { return FText::FromString(TEXT("Configure the XLSX importer plugin")); }

	virtual void PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent) override;
#endif

	UPROPERTY(EditAnywhere, Config, Category = XlsxImporter)
	TArray<FPMXlsxImporterSettingsEntry> AssetImportSettings;

	// While importing, up to this many errors will be accumulated and reported before stopping the import process
	UPROPERTY(EditAnywhere, Config, Category = XlsxImporter)
	int32 MaxErrors = 100;

	void ImportCheckedOut(FPMXlsxImporterContextLogger& InOutErrors) const;
	void ImportAll(FPMXlsxImporterContextLogger& InOutErrors) const;
	void ImportEntry(int32 Index, FPMXlsxImporterContextLogger& InOutErrors) const;

	// Unreal will call this function because FPMXlsxImporterSettingsEntry's WorksheetName UPROPERTY has the GetOptions meta tag
	// We can't put this function on that struct because USTRUCTS can't have UFUNCTIONS, so instead it looks for this function
	// on the struct's outer object (this)
	UFUNCTION()
	TArray<FString> GetWorksheetNames() const;

private:
#if WITH_EDITORONLY_DATA
	// Save off the index of the last edited SettingEntry so that when it calls GetWorksheetNames(), we know which worksheet to read
	int32 LastEditedSettingsIndex;
#endif

};
