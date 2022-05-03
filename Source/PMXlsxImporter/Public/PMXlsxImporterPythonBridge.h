#pragma once

#include "CoreMinimal.h"
#include "PMXlsxImporterPythonBridge.generated.h"

USTRUCT(Blueprintable, BlueprintType)
struct FPMXlsxImporterPythonBridgeDataAssetInfo
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite)
	FString AssetName;

	// Map of header to value for that data
	UPROPERTY(BlueprintReadWrite)
	TMap<FString, FString> Data;
};

UCLASS(Blueprintable)
class UPMXlsxImporterPythonBridge : public UObject
{
	GENERATED_BODY()

public:

	// Returns the Python subclass of UPMXlsxImporterPythonBridge which contains all of the
	// UFUNCTION(BlueprintImplementableEvent)s that have been implemented in Python
	// See https://forums.unrealengine.com/t/running-a-python-script-with-c/114117/3
	static UPMXlsxImporterPythonBridge* Get();

	UFUNCTION(BlueprintImplementableEvent, Category = Python)
	TArray<FString> ReadWorksheetNames(const FString& AbsoluteFilePath);

	UFUNCTION(BlueprintImplementableEvent, Category = Python)
	TArray<FPMXlsxImporterPythonBridgeDataAssetInfo> ReadWorksheet(const FString& AbsoluteFilePath, const FString& WorksheetName);
};
