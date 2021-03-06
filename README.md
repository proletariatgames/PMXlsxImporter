# Proletariat Module XLSX Importer

This plugin allows an Unreal 4 or 5 project to generate DataAssets from XLSX spreadsheets.

## INSTALLATION

1. Clone this repository into your project's Plugins directory.
2. Install [Python3](https://python.org). Any version of Python 3 is fine. This plugin uses Unreal's built-in Python plugin which runs Python 3.7. This step is necessary to download the libraries used by the plugin.
3. Install openpyxl (the python library used to read XLSX files) by running `PMXlsxImporter/Content/Python/install-openpyxl.bat` (Windows) or `install-openpyxl.sh` (Mac/Linux).
4. Build and run Unreal Editor.

## SETUP

1. Optionally clone [PMXlsxImporterDemo](https://github.com/proletariatgames/PMXlsxImporterDemo) for an example class and data.
2. Create a subclass of UPMXlsxDataAsset. See UPMXlsxImporterDemoTestData in PMXlsxImporterDemo as an example.
3. Add `Meta=(ImportFromXLSX)` to any `UPROPERTY` in your subclass that should be filled in from an XLSX file.
4. Create an XLSX file with your data. See `PMXlsxImporterDemo/Content/test.xlsx` as an example. The "Name" column will be the name of each data asset, and each other column should be the C++ name of a `UPROPERTY` marked with `Meta=(ImportFromXLSX)`.
5. In Edit->Project Settings->Asset Manager, add your subclass type as a PrimaryAssetType. You may also want to check "Should Guess Type and Name in Editor" so that the editor can determine names without you needing to manually implement `UDataAsset::GetPrimaryAssetId()` on each of your UPMXlsxDataAsset subclasses.
6. In Edit->Project Settings->XLSX Import, add an entry, then select your data asset type, XLSX file, worksheet name, and output dir. Note that output dir should be contain only data assets generated by this plugin. The plugin will attempt to delete assets that are not listed in the XLSX file under the assumption that they have been removed from the XLSX file.
7.
    - Click the "Import XLSX" button on the toolbar. This will show a dialog that allows you to import all XLSX files checked out in your version control, all XLSX files listed in XLSX Import settings, or a specific file. Look at the Output Log to see if there were any errors during the import run.  
    - Alternatively, importing XLSX files can be done via a commandlet. Add this argument to import all XLSX files without opening the editor GUI:

            -run=PMXlsxImporter

        This will import all XLSX files by default, or you can add the `-c` switch to only import XLSX files checked out in source control.

## ADVANCED FEATURES

### Several functions in UPMXlsxDataAsset can be overridden

- `ImportFromXLSXImpl` is a good place to process input from the XLSX file or to set non-`UPROPERTY` fields.
- `ValidateImpl` is a good place to check that your data is internally consistent. For example, if you have a StartDate and an EndDate, you may want to check that StartDate comes before EndDate.
- `ValidateAgainstPreviousImpl` is a good place to check that your data is consistent from one data asset to the next. For example, you may want to check that one asset's StartDate comes after the previous asset's EndDate.
- `WasModified` is used to tell if an asset needs to be checked out in source control. Assets are only checked out if they have been modified.
- `ParseValue` lets you add custom parsing for types not supported out of the box by this plugin. For example, if you have defined a USTRUCT named FMyStruct with
    ```C++
    static FMyStruct FromString(const FString& Value)
    ```
    you can do something like:
    ```C++
    if (Property->GetCPPType() == TEXT("FMyStruct"))
    {
        FMyStruct* StructResult = (FMyStruct*)Result;
        *StructResult = FMyStruct::FromString(Value);
        return true;
    }
    return Super::ParseValue(Property, Value, Result, InOutErrors); // Do this last. See comments above ParseValue.
    ```

## IF YOU FOUND THIS PLUGIN USEFUL

Please consider donating to Proletariat's annual Extra Life charity marathon in November. You can do that by visiting [Extra Life](https://www.extra-life.org/) and searching for Proletariat's team.
