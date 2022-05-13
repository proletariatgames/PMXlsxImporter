// Copyright 2022 Proletariat, Inc.

#include "PMXlsxImporterCommandlet.h"
#include "PMXlsxImporterLog.h"
#include "PMXlsxImporterSettings.h"
#include "PMXlsxImporterContextLogger.h"

int32 UPMXlsxImporterCommandlet::Main(const FString& Params)
{
	const TCHAR* CHECKED_OUT_SWTICH = TEXT("c");

	TArray<FString> Tokens;
	TArray<FString> Switches;
	ParseCommandLine(*Params, Tokens, Switches);

	// From PythonScriptCommandlet.cpp: tick once to ensure that any start-up scripts have been run
	FTicker::GetCoreTicker().Tick(0.0f);

	const UPMXlsxImporterSettings* SettingsCDO = GetDefault<UPMXlsxImporterSettings>();
	FPMXlsxImporterContextLogger Errors;
	if (Switches.Contains(CHECKED_OUT_SWTICH))
	{
		SettingsCDO->ImportCheckedOut(Errors);
	}
	else
	{
		SettingsCDO->ImportAll(Errors);
	}

	UE_LOG(LogPMXlsxImporter, Log, TEXT("Import run completed with %i errors"), Errors.Num());
	Errors.Flush();

	return Errors.Num();
}
