// Copyright XiaoYao

#include "DetailCustomizations/NarrativeNode_CustomInputDetails.h"
#include "DetailLayoutBuilder.h"
#include "NarrativeAsset.h"

#define LOCTEXT_NAMESPACE "NarrativeNode_CustomInputDetails"

FNarrativeNode_CustomInputDetails::FNarrativeNode_CustomInputDetails()
{
	bExcludeReferencedEvents = true;
}

void FNarrativeNode_CustomInputDetails::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	// For backward compatability, these localized texts are in NarrativeNode_CustomInputDetails, 
	//  not NarrativeNode_CustomNodeBase, so passing them in to a common function.

	static const FText CustomRowNameText = LOCTEXT("CustomRowName", "Event Name");
	static const FText EventNameText = LOCTEXT("EventName", "Event Name");

	CustomizeDetailsInternal(DetailLayout, CustomRowNameText, EventNameText);
}

IDetailCategoryBuilder& FNarrativeNode_CustomInputDetails::CreateDetailCategory(IDetailLayoutBuilder& DetailLayout) const
{
	return DetailLayout.EditCategory("CustomInput", LOCTEXT("CustomInputCategory", "Custom Event"));
}

TArray<FName> FNarrativeNode_CustomInputDetails::BuildEventNames(const UNarrativeAsset& NarrativeAsset) const
{
	return NarrativeAsset.GetCustomInputs();
}

#undef LOCTEXT_NAMESPACE
