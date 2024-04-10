// Copyright XiaoYao

// NOTE (gtaylor) This class is planned for submission to Epic to include in baseline UE.
// If/when that happens, we will want to remove this version and update to the latest one in the PropertyModule

#include "UnrealExtensions/INarrativeExtendedPropertyTypeCustomization.h"

#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "Widgets/Text/STextBlock.h"

void INarrativeExtendedPropertyTypeCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> InStructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	StructPropertyHandle = InStructPropertyHandle;

	// Connect our property callback to any of the children properties changing
	uint32 NumChildren;
	StructPropertyHandle->GetNumChildren(NumChildren);

	for (uint32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex)
	{
		const TSharedRef<IPropertyHandle> ChildHandle = StructPropertyHandle->GetChildHandle(ChildIndex).ToSharedRef();

		ChildHandle->SetOnPropertyValueChanged(
			FSimpleDelegate::CreateSP(this, &INarrativeExtendedPropertyTypeCustomization::OnAnyChildPropertyChanged));
	}

	CreateHeaderRowWidget(HeaderRow, StructCustomizationUtils);
}

void INarrativeExtendedPropertyTypeCustomization::CustomizeChildrenDefaultImpl(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	// A 'default' implementation of CustomizeChildren

	uint32 NumChildren = 0;
	PropertyHandle->GetNumChildren(NumChildren);

	for (uint32 ChildNum = 0; ChildNum < NumChildren; ++ChildNum)
	{
		StructBuilder.AddProperty(PropertyHandle->GetChildHandle(ChildNum).ToSharedRef());
	}
}

void INarrativeExtendedPropertyTypeCustomization::CreateHeaderRowWidget(FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	// Build the Slate widget for the header row
	HeaderRow
		.NameContent()
		[
			SAssignNew(HeaderTextBlock, STextBlock)
			.Text(BuildHeaderText())
		];
}

void INarrativeExtendedPropertyTypeCustomization::OnAnyChildPropertyChanged() const
{
	RefreshHeader();
}

void INarrativeExtendedPropertyTypeCustomization::RefreshHeader() const
{
	if (HeaderTextBlock.IsValid() && StructPropertyHandle.IsValid())
	{
		HeaderTextBlock->SetText(BuildHeaderText());
	}
}

FText INarrativeExtendedPropertyTypeCustomization::BuildHeaderText() const
{
	if (StructPropertyHandle.IsValid())
	{
		return StructPropertyHandle->GetPropertyDisplayName();
	}
	else
	{
		return FText();
	}
}
