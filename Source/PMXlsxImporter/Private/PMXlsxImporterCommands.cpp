// Copyright 2022 Proletariat, Inc.

#include "PMXlsxImporterCommands.h"

#define LOCTEXT_NAMESPACE "FPMXlsxImporterModule"

void FPMXlsxImporterCommands::RegisterCommands()
{
#if ENGINE_MAJOR_VERSION == 4
	UI_COMMAND(PluginAction, "Import XLSX", "Import XLSX Files as data assets", EUserInterfaceActionType::Button, FInputGesture());
#elif ENGINE_MAJOR_VERSION == 5
	UI_COMMAND(PluginAction, "Import XLSX", "Import XLSX Files as data assets", EUserInterfaceActionType::Button, FInputChord());
#else
#	error Unknown engine version
#endif
}

#undef LOCTEXT_NAMESPACE
