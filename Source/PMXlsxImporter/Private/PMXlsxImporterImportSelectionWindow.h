#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "PMXlsxImporterImportSelectionWindow.generated.h"

class UCheckBox;
class UComboBoxString;
class UButton;

UCLASS()
class PMXLSXIMPORTER_API UPMXlsxImporterImportSelectionWindow : public UEditorUtilityWidget
{
	GENERATED_BODY()
public:

	UPROPERTY(meta = (BindWidget))
	UCheckBox* CheckedOutOption;

	UPROPERTY(meta = (BindWidget))
	UCheckBox* AllFilesOption;

	UPROPERTY(meta = (BindWidget))
	UCheckBox* OneWorksheetOption;

	UPROPERTY(meta = (BindWidget))
	UComboBoxString* WorksheetSelector;

	UPROPERTY(meta = (BindWidget))
	UButton* ImportButton;

	static void Open();

protected:
	void NativeConstruct() override;

private:
	UFUNCTION()
	void OnCheckedOutOptionClicked(bool bSelected);

	UFUNCTION()
	void OnAllFilesOptionClicked(bool bSelected);

	UFUNCTION()
	void OnOneWorksheetOptionClicked(bool bSelected);

	UFUNCTION()
	void OnWorksheetSelectorOpened();

	UFUNCTION()
	void OnImportButtonClicked();

	void OnOptionClicked(UCheckBox* Option);

	void PopulateWorksheetSelector();
};

