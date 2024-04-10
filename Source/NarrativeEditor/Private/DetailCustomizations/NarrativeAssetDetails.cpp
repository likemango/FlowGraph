// Copyright XiaoYao

#include "DetailCustomizations/NarrativeAssetDetails.h"
#include "NarrativeAsset.h"
#include "Nodes/Route/NarrativeNode_SubGraph.h"

#include "DetailLayoutBuilder.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyEditing.h"
#include "Widgets/Input/SEditableTextBox.h"

#define LOCTEXT_NAMESPACE "NarrativeAssetDetails"

void FNarrativeAssetDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	IDetailCategoryBuilder& NarrativeAssetCategory = DetailBuilder.EditCategory("SubGraph", LOCTEXT("SubGraphCategory", "Sub Graph"));

	TArray<TSharedPtr<IPropertyHandle>> ArrayPropertyHandles;
	ArrayPropertyHandles.Add(DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UNarrativeAsset, CustomInputs)));
	ArrayPropertyHandles.Add(DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UNarrativeAsset, CustomOutputs)));
	for (const TSharedPtr<IPropertyHandle>& PropertyHandle : ArrayPropertyHandles)
	{
		if (PropertyHandle.IsValid() && PropertyHandle->AsArray().IsValid())
		{
			const TSharedRef<FDetailArrayBuilder> ArrayBuilder = MakeShareable(new FDetailArrayBuilder(PropertyHandle.ToSharedRef()));
			ArrayBuilder->OnGenerateArrayElementWidget(FOnGenerateArrayElementWidget::CreateSP(this, &FNarrativeAssetDetails::GenerateCustomPinArray));

			NarrativeAssetCategory.AddCustomBuilder(ArrayBuilder);
		}
	}
}

void FNarrativeAssetDetails::GenerateCustomPinArray(TSharedRef<IPropertyHandle> PropertyHandle, int32 ArrayIndex, IDetailChildrenBuilder& ChildrenBuilder)
{
	IDetailPropertyRow& PropertyRow = ChildrenBuilder.AddProperty(PropertyHandle);
	PropertyRow.ShowPropertyButtons(true);
	PropertyRow.ShouldAutoExpand(true);

	PropertyRow.CustomWidget(false)
		.ValueContent()
		[
			SNew(SEditableTextBox)
				.Text(this, &FNarrativeAssetDetails::GetCustomPinText, PropertyHandle)
				.OnTextCommitted_Static(&FNarrativeAssetDetails::OnCustomPinTextCommitted, PropertyHandle)
				.OnVerifyTextChanged_Static(&FNarrativeAssetDetails::VerifyNewCustomPinText)
		];
}

FText FNarrativeAssetDetails::GetCustomPinText(TSharedRef<IPropertyHandle> PropertyHandle) const
{
	FText PropertyValue;
	const FPropertyAccess::Result GetValueResult = PropertyHandle->GetValueAsDisplayText(PropertyValue);
	ensure(GetValueResult == FPropertyAccess::Success);
	return PropertyValue;
}

void FNarrativeAssetDetails::OnCustomPinTextCommitted(const FText& InText, ETextCommit::Type InCommitType, TSharedRef<IPropertyHandle> PropertyHandle)
{
	const FPropertyAccess::Result SetValueResult = PropertyHandle->SetValueFromFormattedString(InText.ToString());
	ensure(SetValueResult == FPropertyAccess::Success);
}

bool FNarrativeAssetDetails::VerifyNewCustomPinText(const FText& InNewText, FText& OutErrorMessage)
{
	const FName NewString = *InNewText.ToString();

	if (NewString == UNarrativeNode_SubGraph::StartPin.PinName || NewString == UNarrativeNode_SubGraph::FinishPin.PinName)
	{
		OutErrorMessage = LOCTEXT("VerifyTextFailed", "This is a standard pin name of Sub Graph node!");
		return false;
	}

	return true;
}

#undef LOCTEXT_NAMESPACE
