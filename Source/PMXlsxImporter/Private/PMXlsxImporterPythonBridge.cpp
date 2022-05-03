#include "PMXlsxImporterPythonBridge.h"

UPMXlsxImporterPythonBridge* UPMXlsxImporterPythonBridge::Get()
{
    TArray<UClass*> PythonBridgeClasses;
    GetDerivedClasses(UPMXlsxImporterPythonBridge::StaticClass(), PythonBridgeClasses);
    int32 NumClasses = PythonBridgeClasses.Num();
    if (NumClasses > 0)
    {
        return Cast<UPMXlsxImporterPythonBridge>(PythonBridgeClasses[NumClasses - 1]->GetDefaultObject());
    }
    return nullptr;
}