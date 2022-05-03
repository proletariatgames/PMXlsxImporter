#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "PMXlsxImporterCommandlet.generated.h"

// Imports all XLSX files currently configured in project settings.
// Run using -run=PMXlsxImporter
// Options: -c (only import XLSX files that are locally checked out in source control)
UCLASS()
class UPMXlsxImporterCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	virtual int32 Main(const FString& Params) override;
};
