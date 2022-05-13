// Copyright 2022 Proletariat, Inc.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "PMXlsxImporterStyle.h"

class FPMXlsxImporterCommands : public TCommands<FPMXlsxImporterCommands>
{
public:

	FPMXlsxImporterCommands()
		: TCommands<FPMXlsxImporterCommands>(TEXT("PMXlsxImporter"), NSLOCTEXT("Contexts", "PMXlsxImporter", "PMXlsxImporter Plugin"), NAME_None, FPMXlsxImporterStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > PluginAction;
};
