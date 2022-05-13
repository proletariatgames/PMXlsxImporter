// Copyright 2022 Proletariat, Inc.

#pragma once

#include "CoreMinimal.h"
#include <type_traits>
#include "PMXlsxImporterContextLogger.h"
#include "PMXlsxDataAsset.generated.h"

UCLASS()
class PMXLSXIMPORTER_API UPMXlsxDataAsset : public UDataAsset
{
	GENERATED_BODY()

#ifdef WITH_EDITOR
public:
	// Input: A map of column headers to stringified values for each of those headers.
	// Make sure to force values to strings in python using the str(...) function
	// or Unreal won't be able to convert the map properly.
	// Record all errors by adding them to InOutErrors
	// Override this function if you want to parse non-UPROPERTY fields.
	void ImportFromXLSX(const TMap<FString, FString>& Values, FPMXlsxImporterContextLogger& InOutErrors);

	// Validate that this object has been set up correctly against both itself and the UPMXlsxDataAsset that came
	// before it in the XLSX file.
	void Validate(const UPMXlsxDataAsset* Previous, FPMXlsxImporterContextLogger& InOutErrors) const;

protected:
	virtual void ImportFromXLSXImpl(const TMap<FString, FString>& Values, FPMXlsxImporterContextLogger& InOutErrors);
	virtual void ValidateImpl(FPMXlsxImporterContextLogger& InOutErrors) const;
	virtual void ValidateAgainstPreviousImpl(const UPMXlsxDataAsset* Previous, FPMXlsxImporterContextLogger& InOutErrors) const {}

	// ImportFromXLSX makes a copy of this before parsing anything. This function checks if anything has changed
	// by exporting this and the copy to text, then comparing the text results.
	// If your subclass parses non-UPROPERTY properties, override WasModified and check those properties here.
	virtual bool WasModified(UPMXlsxDataAsset* Original);

	// Parse Value according to type info in Property, then store the parsed value in Result.
	// All Parse* functions return a bool indicating whether the string was successfully parsed.
	// If it was not successfully parsed, the function should push an error onto InOutErrors.
	// Subclasses may override this to parse class-specific types.
	// Subclasses should do their own parsing for custom types then call Super::ParseValue if necessary,
	// because UPMXlsxDataAsset::ParseValue appends an error if it fails
	virtual bool ParseValue(FProperty& Property, const FString& Value, void* Result, FPMXlsxImporterContextLogger& InOutErrors);

	// Accepts numbers ending in ".0" because sometimes XLSX files are like that
	template<typename TInt>
	bool ParseInt(const FString& Value, TInt& OutResult, FPMXlsxImporterContextLogger& InOutErrors)
	{
		// uint64 is not supported. Unreal does not have FDefaultValueHelper::ParseUInt64.
		static_assert(!std::is_same<TInt, uint64>::value, "uint64 is not supported by PMXlsxDataAsset::ParseInt.");
		static_assert(sizeof(TInt) <= sizeof(int64), "Data types larger than int64 are not supported.");

		int64 Result64 = 0;
		if (!UPMXlsxDataAsset::ParseInt64Internal(Value, Result64))
		{
			AddUnableToParseError(Value, InOutErrors);
			return false;
		}

		if (Result64 < (int64)TNumericLimits<TInt>::Min() || Result64 > (int64)TNumericLimits<TInt>::Max())
		{
			InOutErrors.Logf(TEXT("Value %li is outside storage limits"), Result64);
			return false;
		}

		OutResult = (TInt)Result64;
		return true;
	}

	// Accepts either the enum's name or the integer value of the enum as valid input.
	template<typename TInt>
	bool ParseEnum(const FString& Value, const UEnum& EnumType, TInt& OutResult, FPMXlsxImporterContextLogger& InOutErrors)
	{
		// uint64 is not supported. Unreal's enum interface returns int64s, which do not overlap with uint64s.
		static_assert(!std::is_same<TInt, uint64>::value, "uint64 is not supported by PMXlsxDataAsset::ParseEnum.");

		int64 Result64 = EnumType.GetValueByNameString(Value, EGetByNameFlags::CheckAuthoredName);
		if (Result64 != INDEX_NONE)
		{
			OutResult = (TInt)Result64;
			return true;
		}

		if (ParseInt64Internal(Value, Result64))
		{
			if (EnumType.IsValidEnumValue(Result64))
			{
				OutResult = (TInt)Result64;
				return true;
			}
			else
			{
				InOutErrors.Logf(TEXT("%i is not a valid value for %s"), Result64, *EnumType.GetName());
				return false;
			}
		}

		AddUnableToParseError(Value, InOutErrors);
		return false;
	}

	// Accepts case insensitive "TRUE", "FALSE", or an int32.
	bool ParseBool(const FString& Value, bool& OutResult, FPMXlsxImporterContextLogger& InOutErrors);
	// ISO8601 formatted datetimes
	bool ParseDateTime(const FString& Value, FDateTime& OutResult, FPMXlsxImporterContextLogger& InOutErrors);
	// Automatically sets up localization namespace and key
	bool ParseText(const FString& PropName, const FString& Value, FText& OutResult, FPMXlsxImporterContextLogger& InOutErrors);
	// Splits Value by commas and recursively parses each element
	bool ParseArray(const FArrayProperty& Property, const FString& Value, void* OutResult, FPMXlsxImporterContextLogger& InOutErrors);

	// Ensures the asset type exists or is empty.
	void ValidatePrimaryAssetType(const FPrimaryAssetType& AssetType, FPMXlsxImporterContextLogger& InOutErrors) const;
	// Ensures the asset id refers to a valid asset or empty.
	void ValidatePrimaryAssetId(const FPrimaryAssetId& AssetId, FPMXlsxImporterContextLogger& InOutErrors) const;

	// Pushes a generic "unable to parse" error on the end of InOutErrors
	void AddUnableToParseError(const FString& Value, FPMXlsxImporterContextLogger& InOutErrors);

private:
	// Parses an Int64 but does not add an error if it fails. Used by ParseInt and ParseEnum.
	bool ParseInt64Internal(const FString& Value, int64& OutResult);
#endif
};
