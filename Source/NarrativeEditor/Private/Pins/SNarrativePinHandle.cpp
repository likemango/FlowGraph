// Copyright XiaoYao

#include "Pins/SNarrativePinHandle.h"

#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "SNarrativePinHandle"

SNarrativePinHandle::SNarrativePinHandle()
	: Blueprint(nullptr)
{
}

SNarrativePinHandle::~SNarrativePinHandle()
{
	Blueprint = nullptr;
}

void SNarrativePinHandle::Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj, const UBlueprint* InBlueprint)
{
	Blueprint = InBlueprint;
	RefreshNameList();
	SGraphPin::Construct(SGraphPin::FArguments(), InGraphPinObj);
}

TSharedRef<SWidget> SNarrativePinHandle::GetDefaultValueWidget()
{
	ParseDefaultValueData();

	// Create widget
	return SAssignNew(ComboBox, SNameComboBox)
		.ContentPadding(FMargin(6.0f, 2.0f))
		.OptionsSource(&PinNames)
		.InitiallySelectedItem(CurrentlySelectedName)
		.OnSelectionChanged(this, &SNarrativePinHandle::ComboBoxSelectionChanged)
		.Visibility(this, &SGraphPin::GetDefaultValueVisibility);
}

void SNarrativePinHandle::ParseDefaultValueData()
{
	FString DefaultValue = GraphPinObj->GetDefaultAsString();
	if (DefaultValue.StartsWith(TEXT("(PinName=")) && DefaultValue.EndsWith(TEXT(")")))
	{
		DefaultValue.Split(TEXT("PinName=\""), nullptr, &DefaultValue);
		DefaultValue.Split(TEXT("\""), &DefaultValue, nullptr);
	}

	// Preserve previous selection
	if (PinNames.Num() > 0)
	{
		for (TSharedPtr<FName> PinName : PinNames)
		{
			if (*PinName.Get() == *DefaultValue)
			{
				CurrentlySelectedName = PinName;
				break;
			}
		}
	}
}

void SNarrativePinHandle::ComboBoxSelectionChanged(const TSharedPtr<FName> NameItem, ESelectInfo::Type SelectInfo) const
{
	const FName Name = NameItem.IsValid() ? *NameItem : NAME_None;
	if (const UEdGraphSchema* Schema = (GraphPinObj ? GraphPinObj->GetSchema() : nullptr))
	{
		const FString ValueString = TEXT("(PinName=\"") + Name.ToString() + TEXT("\")");

		if (GraphPinObj->GetDefaultAsString() != ValueString)
		{
			const FScopedTransaction Transaction(LOCTEXT("ChangePinValue", "Change Pin Value"));
			GraphPinObj->Modify();

			Schema->TrySetDefaultValue(*GraphPinObj, ValueString);
		}
	}
}

#undef LOCTEXT_NAMESPACE
