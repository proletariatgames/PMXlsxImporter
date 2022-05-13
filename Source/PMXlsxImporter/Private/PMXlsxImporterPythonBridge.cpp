// Copyright 2022 Proletariat, Inc.

#include "PMXlsxImporterPythonBridge.h"
#include "PMXlsxImporterLog.h"

UPMXlsxImporterPythonBridge* UPMXlsxImporterPythonBridge::Get()
{
    TArray<UClass*> PythonBridgeClasses;
    GetDerivedClasses(UPMXlsxImporterPythonBridge::StaticClass(), PythonBridgeClasses);
    int32 NumClasses = PythonBridgeClasses.Num();
    if (NumClasses > 0)
    {
        return Cast<UPMXlsxImporterPythonBridge>(PythonBridgeClasses[NumClasses - 1]->GetDefaultObject());
    }

	UE_LOG(LogPMXlsxImporter, Error, TEXT("No python bridge implementation found. Have you installed openpyxl? See PMXlsxImporter/README.md"));
    return nullptr;
}