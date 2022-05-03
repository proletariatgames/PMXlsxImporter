#include "PMXlsxDataAsset.h"
#include "Misc/DefaultValueHelper.h"
#include "PMXlsxImporterLog.h"
#include "Engine/AssetManager.h"
#include "EditorAssetLibrary.h"
#include "Exporters/Exporter.h"
#include "UnrealExporter.h"

static const TCHAR* const IMPORT_FROM_XLSX_METADATA_TAG = TEXT("ImportFromXLSX");

static const TCHAR* const TRUE_TEXT = TEXT("TRUE");
static const TCHAR* const FALSE_TEXT = TEXT("FALSE");

#ifdef WITH_EDITOR
void UPMXlsxDataAsset::ImportFromXLSX(const TMap<FString, FString>& Values, FPMXlsxImporterContextLogger& InOutErrors)
{
	auto ScopedErrorContext = InOutErrors.PushContext(FString::Printf(TEXT(": %s %s"), *GetClass()->GetName(), *GetName()));
	ImportFromXLSXImpl(Values, InOutErrors);
}

void UPMXlsxDataAsset::ImportFromXLSXImpl(const TMap<FString, FString>& Values, FPMXlsxImporterContextLogger& InOutErrors)
{
	UE_LOG(LogPMXlsxImporter, VeryVerbose, TEXT("Importing data from python to %s %s"), *GetClass()->GetName(), *GetName());
	for (auto& kvp : Values)
	{
		UE_LOG(LogPMXlsxImporter, VeryVerbose, TEXT("\t%s: %s"), *kvp.Key, *kvp.Value);
	}

	// Keep a copy around to see if anything actually gets changed.
	// It would be more accurate to pull Original from what's currently checked into source control,
	// but that would be very slow.
	UPMXlsxDataAsset* Original = DuplicateObject(this, nullptr, GetFName());

	// https://ikrima.dev/ue4guide/engine-programming/uobject-reflection/uobject-reflection/
	UE_LOG(LogPMXlsxImporter, VeryVerbose, TEXT("Iterating over properties of %s %s"), *GetClass()->GetName(), *GetName());
	for (TFieldIterator<FProperty> PropertyIterator(GetClass(), EFieldIteratorFlags::IncludeSuper); PropertyIterator; ++PropertyIterator)
	{
		FString CPPName = PropertyIterator->GetNameCPP();
		FString CPPType = PropertyIterator->GetCPPType();
		auto ScopedErrorContext = InOutErrors.PushContext(FString::Printf(TEXT(".%s %s"), *CPPName, *CPPType));

		bool bHasImportMetadata = PropertyIterator->HasMetaData(IMPORT_FROM_XLSX_METADATA_TAG);
		if (!bHasImportMetadata)
		{
			UE_LOG(LogPMXlsxImporter, VeryVerbose,
				TEXT("\tSkipping %s %s because it does not have metadata tag %s"),
				*CPPType, *CPPName, IMPORT_FROM_XLSX_METADATA_TAG
			);
			continue;
		}

		const FString* Value = Values.Find(CPPName);
		if (Value == nullptr)
		{
			InOutErrors.Logf(TEXT("No value found (are you missing a column named \"%s\"?)"), *CPPName);
			continue;
		}

		ParseValue(**PropertyIterator, *Value, PropertyIterator->ContainerPtrToValuePtr<void>(this), InOutErrors);
	}

	// Telling Unreal to save a file guarantees the file becomes modified even if there aren't meaningful changes to
	// that file's data. We only want to check out and save modified assets.
	if (WasModified(Original))
	{
		if (!UEditorAssetLibrary::CheckoutLoadedAsset(this))
		{
			// CheckoutLoadedAsset will print its own errors, but we want to add one here so that we can
			// properly track if the run as a whole succeeded or not.
			InOutErrors.Logf(TEXT("Unable to checkout asset %s"), *GetName());
			return;
		}
		// No reason to mark the package as dirty. We know we need to save right now.
		if (!UEditorAssetLibrary::SaveLoadedAsset(this, /*bOnlyIfIsDirty:*/ false))
		{
			// SaveLoadedAsset will print its own errors, but we want to add one here so that we can
			// properly track if the run as a whole succeeded or not.
			InOutErrors.Logf(TEXT("Unable to save asset %s"), *GetName());
			return;
		}
	}
}

void UPMXlsxDataAsset::Validate(const UPMXlsxDataAsset* Previous, FPMXlsxImporterContextLogger& InOutErrors) const
{
	auto ScopedErrorContext = InOutErrors.PushContext(FString::Printf(TEXT(": %s %s"), *GetClass()->GetName(), *GetName()));

	ValidateImpl(InOutErrors);
	ValidateAgainstPreviousImpl(Previous, InOutErrors);
}

void UPMXlsxDataAsset::ValidateImpl(FPMXlsxImporterContextLogger& InOutErrors) const
{
	UE_LOG(LogPMXlsxImporter, VeryVerbose, TEXT("Validating properties of %s %s"), *GetClass()->GetName(), *GetName());
	for (TFieldIterator<FProperty> PropertyIterator(GetClass(), EFieldIteratorFlags::IncludeSuper); PropertyIterator; ++PropertyIterator)
	{
		FString CPPName = PropertyIterator->GetNameCPP();
		FString CPPType = PropertyIterator->GetCPPType();
		auto ScopedErrorContext = InOutErrors.PushContext(FString::Printf(TEXT(".%s %s"), *CPPName, *CPPType));

		bool bHasImportMetadata = PropertyIterator->HasMetaData(IMPORT_FROM_XLSX_METADATA_TAG);
		if (!bHasImportMetadata)
		{
			continue;
		}

		const void* Value = PropertyIterator->ContainerPtrToValuePtr<void>(this);

		if (PropertyIterator->IsA<FStructProperty>())
		{
			if (CPPType == TEXT("FPrimaryAssetType"))
			{
				ValidatePrimaryAssetType(*(const FPrimaryAssetType*)Value, InOutErrors);
			}
			else if (CPPType == TEXT("FPrimaryAssetId"))
			{
				ValidatePrimaryAssetId(*(const FPrimaryAssetId*)Value, InOutErrors);
			}
		}
	}
}

bool UPMXlsxDataAsset::WasModified(UPMXlsxDataAsset* Original)
{
	// Export this and Original as text, then compare the text
	// See UAssetToolsImpl::DumpAssetToTempFile
	const FExportObjectInnerContext Context;
	const uint32 FLAGS = PPF_ExportsNotFullyQualified | PPF_Copy | PPF_Delimited;

	FStringOutputDevice OriginalArchive;
	UExporter::ExportToOutputDevice(&Context, Original, nullptr, OriginalArchive, TEXT("copy"), 0, FLAGS, false, Original->GetOuter());

	FStringOutputDevice UpdatedArchive;
	UExporter::ExportToOutputDevice(&Context, this, nullptr, UpdatedArchive, TEXT("copy"), 0, FLAGS, false, GetOuter());

	bool bWasModified = !OriginalArchive.Equals(UpdatedArchive, ESearchCase::CaseSensitive);
	UE_LOG(LogPMXlsxImporter, Verbose, TEXT("%s %s modified"), *GetName(), bWasModified ? TEXT("WAS") : TEXT("was NOT"));

	if (bWasModified)
	{
		UE_LOG(LogPMXlsxImporter, VeryVerbose, TEXT("Orignal: %s"), *OriginalArchive);
		UE_LOG(LogPMXlsxImporter, VeryVerbose, TEXT("Updated: %s"), *UpdatedArchive);
	}
	else
	{
		UE_LOG(LogPMXlsxImporter, VeryVerbose, TEXT("%s"), *OriginalArchive);
	}

	return bWasModified;
}

bool UPMXlsxDataAsset::ParseValue(FProperty& Property, const FString& Value, void* Result, FPMXlsxImporterContextLogger& InOutErrors)
{
	if (Property.IsA<FBoolProperty>())
	{
		return ParseBool(Value, *(bool*)Result, InOutErrors);
	}
	else if (Property.IsA<FInt8Property>())
	{
		return ParseInt<int8>(Value, *(int8*)Result, InOutErrors);
	}
	else if (Property.IsA<FInt16Property>())
	{
		return ParseInt<int16>(Value, *(int16*)Result, InOutErrors);
	}
	else if (Property.IsA<FIntProperty>())
	{
		return ParseInt<int32>(Value, *(int32*)Result, InOutErrors);
	}
	else if (Property.IsA<FInt64Property>())
	{
		return ParseInt<int64>(Value, *(int64*)Result, InOutErrors);
	}
	else if (Property.IsA<FByteProperty>())
	{
		return ParseInt<uint8>(Value, *(uint8*)Result, InOutErrors);
	}
	else if (Property.IsA<FUInt16Property>())
	{
		return ParseInt<uint16>(Value, *(uint16*)Result, InOutErrors);
	}
	else if (Property.IsA<FUInt32Property>())
	{
		return ParseInt<uint32>(Value, *(uint32*)Result, InOutErrors);
	}
	// FUint64Property is not supported - see ParseInt

	else if (Property.IsA<FEnumProperty>())
	{
		FEnumProperty* EnumProp = CastField<FEnumProperty>(&Property);
		FNumericProperty* UnderlyingProp = EnumProp->GetUnderlyingProperty();
		UEnum* Enum = EnumProp->GetEnum();
		if (UnderlyingProp->IsA<FInt8Property>())
		{
			return ParseEnum<int8>(Value, *Enum, *(int8*)Result, InOutErrors);
		}
		else if (UnderlyingProp->IsA<FInt16Property>())
		{
			return ParseEnum<int16>(Value, *Enum, *(int16*)Result, InOutErrors);
		}
		else if (UnderlyingProp->IsA<FIntProperty>())
		{
			return ParseEnum<int32>(Value, *Enum, *(int32*)Result, InOutErrors);
		}
		else if (UnderlyingProp->IsA<FInt64Property>())
		{
			return ParseEnum<int64>(Value, *Enum, *(int64*)Result, InOutErrors);
		}
		else if (UnderlyingProp->IsA<FByteProperty>())
		{
			return ParseEnum<uint8>(Value, *Enum, *(uint8*)Result, InOutErrors);
		}
		else if (UnderlyingProp->IsA<FUInt16Property>())
		{
			return ParseEnum<uint16>(Value, *Enum, *(uint16*)Result, InOutErrors);
		}
		else if (UnderlyingProp->IsA<FUInt32Property>())
		{
			return ParseEnum<uint32>(Value, *Enum, *(uint32*)Result, InOutErrors);
		}
		// uint64 is not supported - see ParseEnum
	}
	else if (Property.IsA<FTextProperty>())
	{
		return ParseText(Property.GetNameCPP(), Value, *(FText*)Result, InOutErrors);
	}
	else if (Property.IsA<FStructProperty>())
	{
		const FString CPPType = Property.GetCPPType();

		if (CPPType == TEXT("FDateTime"))
		{
			return ParseDateTime(Value, *(FDateTime*)Result, InOutErrors);
		}
	}
	else if (Property.IsA<FArrayProperty>())
	{
		FArrayProperty* ArrayProp = CastField<FArrayProperty>(&Property);
		return ParseArray(*ArrayProp, Value, Result, InOutErrors);
	}

	// Property is not explictly supported by this class, but maybe Unreal supports it natively
	if (Property.ImportText(*Value, Result, PPF_None, this, &InOutErrors))
	{
		return true;
	}

	InOutErrors.Log(TEXT("Type not supported"));
	return false;
}

bool UPMXlsxDataAsset::ParseBool(const FString& Value, bool& OutResult, FPMXlsxImporterContextLogger& InOutErrors)
{
	if (Value.Equals(TRUE_TEXT, ESearchCase::IgnoreCase))
	{
		OutResult = true;
		return true;
	}
	else if (Value.Equals(FALSE_TEXT, ESearchCase::IgnoreCase))
	{
		OutResult = false;
		return true; // successfully parsed a false value
	}

	int32 IntResult = 0;
	if (FDefaultValueHelper::ParseInt(Value, IntResult))
	{
		OutResult = (bool)IntResult;
		return true;
	}

	AddUnableToParseError(Value, InOutErrors);
	return false;
}

bool UPMXlsxDataAsset::ParseInt64Internal(const FString& Value, int64& OutResult)
{
	if (FDefaultValueHelper::ParseInt64(Value, OutResult))
	{
		return true;
	}

	// XLSX might save an int as, for example, "2.0". That's valid, but needs to be parsed as a double then converted to an int.
	double DoubleResult;
	if (FDefaultValueHelper::ParseDouble(Value, DoubleResult) && FMath::RoundToZero(DoubleResult) == DoubleResult)
	{
		OutResult = (int64)DoubleResult;
		return true;
	}

	// Let the caller add the error
	return false;
}

bool UPMXlsxDataAsset::ParseDateTime(const FString& Value, FDateTime& OutResult, FPMXlsxImporterContextLogger& InOutErrors)
{
	if (FDateTime::ParseIso8601(*Value, OutResult))
	{
		return true;
	}

	AddUnableToParseError(Value, InOutErrors);
	return false;
}

bool UPMXlsxDataAsset::ParseText(const FString& PropName, const FString& Value, FText& OutResult, FPMXlsxImporterContextLogger& InOutErrors)
{
	OutResult = FText::FromString(Value);
	OutResult = FText::ChangeKey(GetClass()->GetName(), FString::Printf(TEXT("%s.%s"), *GetName(), *PropName), OutResult);
	return true;
}

bool UPMXlsxDataAsset::ParseArray(const FArrayProperty& Property, const FString& Value, void* OutResult, FPMXlsxImporterContextLogger& InOutErrors)
{
	// See JsonObjectConverter.cpp
	FScriptArrayHelper ArrayHelper(&Property, OutResult);

	if (Value.IsEmpty())
	{
		ArrayHelper.EmptyValues();
		return true;
	}

	TArray<FString> Values;
	// TODO support other delimiters via metadata
	int32 ArrayLength = Value.ParseIntoArray(Values, TEXT(","), /*bCullEmpty:*/ false);

	ArrayHelper.Resize(ArrayLength);

	bool bAllParsed = true;
	for (int32 Index = 0; Index < ArrayLength; ++Index)
	{
		auto ScopedErrorContext = InOutErrors.PushContext(FString::Printf(TEXT("[%i]"), Index));

		const FString TrimmedValue = Values[Index].TrimStartAndEnd();
		bAllParsed &= ParseValue(*Property.Inner, TrimmedValue, ArrayHelper.GetRawPtr(Index), InOutErrors);
	}

	return bAllParsed;
}

void UPMXlsxDataAsset::ValidatePrimaryAssetType(const FPrimaryAssetType& AssetType, FPMXlsxImporterContextLogger& InOutErrors) const
{
	if (!AssetType.IsValid())
	{
		return; // Explicitly invalid PrimaryAssetTypes (NAME_None) are allowed
	}

	UAssetManager& AssetManager = UAssetManager::Get();
	FPrimaryAssetTypeInfo Info;
	if (!AssetManager.GetPrimaryAssetTypeInfo(AssetType, Info))
	{
		InOutErrors.Logf(TEXT("PrimaryAssetType %s does not exist"), *AssetType.ToString());
	}
}

void UPMXlsxDataAsset::ValidatePrimaryAssetId(const FPrimaryAssetId& AssetId, FPMXlsxImporterContextLogger& InOutErrors) const
{
	if (!AssetId.IsValid())
	{
		return; // Explictly invalid PrimaryAssetIds (NAME_None:NAME_None) are allowed
	}

	UAssetManager& AssetManager = UAssetManager::Get();
	FAssetData AssetData;
	if (!AssetManager.GetPrimaryAssetData(AssetId, AssetData))
	{
		InOutErrors.Logf(TEXT("PrimaryAssetId %s does not exist"), *AssetId.ToString());
	}
}

void UPMXlsxDataAsset::AddUnableToParseError(const FString& Value, FPMXlsxImporterContextLogger& InOutErrors)
{
	InOutErrors.Logf(TEXT("Unable to parse %s"), *Value);
}

#endif
