#include "PMXlsxImporterSettingsEntry.h"
#include "PMXlsxDataAsset.h"
#include "PMXlsxImporterLog.h"
#include "EditorAssetLibrary.h"
#include "Engine/AssetManager.h"
#include "UObject/SavePackage.h"
#include "FileHelpers.h"

void FPMXlsxImporterSettingsEntry::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.MemberProperty->GetNameCPP() == TEXT("WorksheetName"))
	{
		// Double check that the user picked a valid worksheet name. It's possible they didn't. See
		// the comment in UPMXlsxImporterSettings::GetWorksheetNames() 
		const TArray<FString> WorksheetNames = GetWorksheetNames();
		if (!WorksheetNames.Contains(WorksheetName))
		{
			WorksheetName = FString();
		}
	}

	if (PropertyChangedEvent.MemberProperty->GetNameCPP() == TEXT("XlsxFile") &&
		!GetXlsxAbsolutePath().IsEmpty())
	{
		const TArray<FString> WorksheetNames = GetWorksheetNames();
		WorksheetName = WorksheetNames.Num() == 0 ? TEXT("") : WorksheetNames[0];
	}

	if (PropertyChangedEvent.MemberProperty->GetNameCPP() != TEXT("OutputDir") &&
		DataAssetType.IsValid() && 
		!GetXlsxAbsolutePath().IsEmpty() &&
		!WorksheetName.IsEmpty())
	{
		const FString XlsxFilename = FPaths::GetBaseFilename(XlsxFile.FilePath);
		OutputDir.Path = FString::Printf(TEXT("Generated/%s/%s/%s"), *DataAssetType.ToString(), *XlsxFilename, *WorksheetName);
	}
}

TArray<FString> FPMXlsxImporterSettingsEntry::GetWorksheetNames() const
{
	const FString& XlsxAbsolutePath = GetXlsxAbsolutePath();
	if (XlsxAbsolutePath.IsEmpty())
	{
		UE_LOG(LogPMXlsxImporter, Warning, TEXT("Could not get worksheet names: xlsx file not set"));
		return TArray<FString>();
	}

	UPMXlsxImporterPythonBridge* PythonBridge = UPMXlsxImporterPythonBridge::Get();
	return PythonBridge->ReadWorksheetNames(XlsxAbsolutePath);
}

void FPMXlsxImporterSettingsEntry::SyncAssets(FPMXlsxImporterContextLogger& InOutErrors, int32 MaxErrors) const
{
	auto ScopedErrorContext = InOutErrors.PushContext(FString::Printf(TEXT("%s:%s"), *XlsxFile.FilePath, *WorksheetName));

	if (!DataAssetType.IsValid())
	{
		InOutErrors.Log(TEXT("Could not sync assets: invalid data asset type"));
		return;
	}

	const FString& XlsxAbsolutePath = GetXlsxAbsolutePath();
	if (XlsxAbsolutePath.IsEmpty())
	{
		InOutErrors.Log(TEXT("Could not sync assets: xlsx file not set"));
		return;
	}

	if (WorksheetName.IsEmpty())
	{
		InOutErrors.Log(TEXT("Could not sync assets: no worksheet name set"));
		return;
	}

	if (OutputDir.Path.IsEmpty())
	{
		InOutErrors.Log(TEXT("Could not sync assets: no output dir set"));
		return;
	}

	UAssetManager& AssetManager = UAssetManager::Get();
	FPrimaryAssetTypeInfo TypeInfo;
	if (!AssetManager.GetPrimaryAssetTypeInfo(DataAssetType, TypeInfo))
	{
		InOutErrors.Logf(TEXT("Could not sync assets: could not get type info for %s"), *DataAssetType.ToString());
		return;
	}
	UClass* Class = TypeInfo.AssetBaseClassLoaded;

	IFileManager& FileManager = IFileManager::Get();

	UPMXlsxImporterPythonBridge* PythonBridge = UPMXlsxImporterPythonBridge::Get();
	TArray<FPMXlsxImporterPythonBridgeDataAssetInfo> ParsedWorksheet = PythonBridge->ReadWorksheet(XlsxAbsolutePath, WorksheetName);

	for (const FPMXlsxImporterPythonBridgeDataAssetInfo& Info : ParsedWorksheet)
	{
		const FString AssetPath = GetProjectRootOutputPath(Info.AssetName);
		// Note that DoesAssetExist is case-insensitive.
		// This is good - perforce will have issues if you change the case of a file.
		if (!UEditorAssetLibrary::DoesAssetExist(AssetPath))
		{
			// https://isaratech.com/save-a-procedurally-generated-texture-as-a-new-asset/
			UPackage* Package = CreatePackage(*AssetPath);
			Package->FullyLoad();
			UPMXlsxDataAsset* Asset = NewObject<UPMXlsxDataAsset>(Package, Class, FName(Info.AssetName), RF_Public | RF_Standalone | RF_MarkAsRootSet);
			Package->MarkPackageDirty();
			FAssetRegistryModule::AssetCreated(Asset);
			const FString PackageFileName = FPackageName::LongPackageNameToFilename(AssetPath, FPackageName::GetAssetPackageExtension());

#if ENGINE_MAJOR_VERSION == 4
			if (!UPackage::SavePackage(Package, Asset, EObjectFlags::RF_NoFlags, *PackageFileName))
#elif ENGINE_MAJOR_VERSION == 5
			FSavePackageArgs SaveArgs;
			if (!UPackage::SavePackage(Package, Asset, *PackageFileName, SaveArgs))
#else
#	error Unknown engine version
#endif
			{
				InOutErrors.Logf(TEXT("Unable to save file %s"), *PackageFileName);
				if (InOutErrors.Num() < MaxErrors)
				{
					continue;
				}
				else
				{
					return;
				}
			}
			UE_LOG(LogPMXlsxImporter, Log, TEXT("Created new asset %s"), *AssetPath);

			const FString AssetAbsolutePath = FileManager.ConvertToAbsolutePathForExternalAppForWrite(*PackageFileName);
			USourceControlHelpers::MarkFileForAdd(AssetAbsolutePath);
		}
	}

	TArray<FString> ExistingAssets = UEditorAssetLibrary::ListAssets(GetProjectRootOutputDir(), /*bRecursive:*/ false, /*bIncludeFolder:*/ false);
	for (const FString& ExistingAssetPath : ExistingAssets)
	{
		// ExistingAssetPath = (e.g.) "/Game/Generated/TestData/test/Sheet1/TestDataFromXLS1.TestDataFromXLS1"

		if (!ShouldAssetExist(ExistingAssetPath, ParsedWorksheet))
		{
			// Convert ExistingAssetPath an absolute file path for USourceControlHelpers. There must be a better way to do this.
			// USourceControlHelpers does try to do this conversion, but it doesn't always work.
			// PackageName = "/Game/Generated/TestData/test/Sheet1/TestDataFromXLS1"
			const FString PackageName = FEditorFileUtils::ExtractPackageName(ExistingAssetPath);
			// RelativePath = "../../../PluginDev/Content/Generated/TestData/test/Sheet1/TestDataFromXLS1.uasset"
			const FString RelativePath = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
			// AbsolutePath = "C:/dev/plugindev-main/PluginDev/Content/Generated/TestData/test/Sheet1/TestDataFromXLS1.uasset"
			const FString AbsolutePath = FileManager.ConvertToAbsolutePathForExternalAppForWrite(*RelativePath);

			FSourceControlState State = USourceControlHelpers::QueryFileState(AbsolutePath);
			if (!State.bIsValid)
			{
				InOutErrors.Logf(TEXT("Source control state is invalid for %s. Refusing to delete this file."), *AbsolutePath);
				if (InOutErrors.Num() < MaxErrors)
				{
					continue;
				}
				else
				{
					return;
				}
			}

			// Internally marks the file for delete in source control and logs what it's doing
			if (!UEditorAssetLibrary::DeleteAsset(ExistingAssetPath))
			{
				InOutErrors.Logf(TEXT("Unable to delete asset %s"), *ExistingAssetPath);
				if (InOutErrors.Num() >= MaxErrors)
				{
					return;
				}
			}
		}
	}

	// Force the AssetManager to rescan now so that it's up to date when we try to validate FPrimaryAssetIds in ParseData().
	TArray<FString> PathToScan;
	PathToScan.Add(GetProjectRootOutputDir());
	AssetManager.ScanPathsSynchronous(PathToScan);
}

void FPMXlsxImporterSettingsEntry::ParseData(FPMXlsxImporterContextLogger& InOutErrors, int32 MaxErrors) const
{
	auto ScopedErrorContext = InOutErrors.PushContext(FString::Printf(TEXT("%s:%s"), *XlsxFile.FilePath, *WorksheetName));

	if (!DataAssetType.IsValid())
	{
		InOutErrors.Log(TEXT("Could not parse data: invalid data asset type"));
		return;
	}

	const FString& XlsxAbsolutePath = GetXlsxAbsolutePath();
	if (XlsxAbsolutePath.IsEmpty())
	{
		InOutErrors.Log(TEXT("Could not parse asset data: xlsx file not set"));
		return;
	}

	if (WorksheetName.IsEmpty())
	{
		InOutErrors.Log(TEXT("Could not parse asset data: no worksheet name set"));
		return;
	}

	if (OutputDir.Path.IsEmpty())
	{
		InOutErrors.Log(TEXT("Could not parse asset data: no output dir set"));
		return;
	}

	UPMXlsxImporterPythonBridge* PythonBridge = UPMXlsxImporterPythonBridge::Get();
	TArray<FPMXlsxImporterPythonBridgeDataAssetInfo> ParsedWorksheet = PythonBridge->ReadWorksheet(XlsxAbsolutePath, WorksheetName);

	for (const FPMXlsxImporterPythonBridgeDataAssetInfo& Info : ParsedWorksheet)
	{
		FString AssetPath = GetProjectRootOutputPath(Info.AssetName);
		UPMXlsxDataAsset* Asset = Cast<UPMXlsxDataAsset>(UEditorAssetLibrary::LoadAsset(AssetPath));
		if (Asset == nullptr)
		{
			InOutErrors.Logf(TEXT("Asset %s is not a UPMXlsxDataAsset"), *AssetPath);
			continue;
		}

		Asset->ImportFromXLSX(Info.Data, InOutErrors);
		if (InOutErrors.Num() >= MaxErrors)
		{
			return;
		}
	}
}

void FPMXlsxImporterSettingsEntry::Validate(FPMXlsxImporterContextLogger& InOutErrors, int32 MaxErrors) const
{
	auto ScopedErrorContext = InOutErrors.PushContext(FString::Printf(TEXT("%s:%s"), *XlsxFile.FilePath, *WorksheetName));

	if (!DataAssetType.IsValid())
	{
		InOutErrors.Log(TEXT("Could not validate data: invalid data asset type"));
		return;
	}

	const FString& XlsxAbsolutePath = GetXlsxAbsolutePath();
	if (XlsxAbsolutePath.IsEmpty())
	{
		InOutErrors.Log(TEXT("Could not validate asset data: xlsx file not set"));
		return;
	}

	if (WorksheetName.IsEmpty())
	{
		InOutErrors.Log(TEXT("Could not validate asset data: no worksheet name set"));
		return;
	}

	if (OutputDir.Path.IsEmpty())
	{
		InOutErrors.Log(TEXT("Could not validate asset data: no output dir set"));
		return;
	}

	UPMXlsxImporterPythonBridge* PythonBridge = UPMXlsxImporterPythonBridge::Get();
	TArray<FPMXlsxImporterPythonBridgeDataAssetInfo> ParsedWorksheet = PythonBridge->ReadWorksheet(XlsxAbsolutePath, WorksheetName);

	UPMXlsxDataAsset* PreviousAsset = nullptr;
	for (const FPMXlsxImporterPythonBridgeDataAssetInfo& Info : ParsedWorksheet)
	{
		FString AssetPath = GetProjectRootOutputPath(Info.AssetName);
		UPMXlsxDataAsset* Asset = Cast<UPMXlsxDataAsset>(UEditorAssetLibrary::LoadAsset(AssetPath));
		if (Asset == nullptr)
		{
			InOutErrors.Logf(TEXT("Asset %s is not a UPMXlsxDataAsset"), *AssetPath);
			continue;
		}

		Asset->Validate(PreviousAsset, InOutErrors);
		if (InOutErrors.Num() >= MaxErrors)
		{
			return;
		}

		PreviousAsset = Asset;
	}
}

FSourceControlState FPMXlsxImporterSettingsEntry::GetXlsxFileSourceControlState(bool bSilent /* = false*/) const
{
	return USourceControlHelpers::QueryFileState(GetXlsxAbsolutePath(), bSilent);
}

FString FPMXlsxImporterSettingsEntry::GetXlsxAbsolutePath() const
{
	return XlsxFile.FilePath.IsEmpty() ? FString() : FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), XlsxFile.FilePath);
}

FString FPMXlsxImporterSettingsEntry::GetProjectRootOutputDir() const
{
	return FString::Printf(TEXT("/Game/%s"), *OutputDir.Path);
}

FString FPMXlsxImporterSettingsEntry::GetProjectRootOutputPath(const FString& AssetName) const
{
	return FString::Printf(TEXT("%s/%s"), *GetProjectRootOutputDir(), *AssetName);
}

bool FPMXlsxImporterSettingsEntry::ShouldAssetExist(const FString& AssetPath, const TArray<FPMXlsxImporterPythonBridgeDataAssetInfo>& ParsedWorksheet) const
{
	for (const FPMXlsxImporterPythonBridgeDataAssetInfo& Info : ParsedWorksheet)
	{
		if (AssetPath.EndsWith(FString::Printf(TEXT(".%s"), *Info.AssetName)))
		{
			return true;
		}
	}
	return false;
}
