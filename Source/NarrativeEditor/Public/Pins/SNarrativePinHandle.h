// Copyright XiaoYao

#pragma once

#include "SGraphPin.h"
#include "SNameComboBox.h"

class NARRATIVEEDITOR_API SNarrativePinHandle : public SGraphPin
{
public:
	SNarrativePinHandle();
	virtual ~SNarrativePinHandle() override;

	SLATE_BEGIN_ARGS(SNarrativePinHandle) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj, const UBlueprint* InBlueprint);
	virtual TSharedRef<SWidget>	GetDefaultValueWidget() override;

protected:
	virtual void RefreshNameList() {}
	void ParseDefaultValueData();
	void ComboBoxSelectionChanged(const TSharedPtr<FName> NameItem, ESelectInfo::Type SelectInfo) const;

	const UBlueprint* Blueprint;
	TArray<TSharedPtr<FName>> PinNames;
	
	TSharedPtr<SNameComboBox> ComboBox;
	TSharedPtr<FName> CurrentlySelectedName;
};
