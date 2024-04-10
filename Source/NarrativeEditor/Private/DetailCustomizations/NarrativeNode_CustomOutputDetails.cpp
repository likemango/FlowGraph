// Copyright XiaoYao

#include "DetailCustomizations/NarrativeNode_CustomOutputDetails.h"
#include "DetailLayoutBuilder.h"
#include "NarrativeAsset.h"

#define LOCTEXT_NAMESPACE "NarrativeNode_CustomOutputDetails"

FNarrativeNode_CustomOutputDetails::FNarrativeNode_CustomOutputDetails()
{
	check(bExcludeReferencedEvents == false);
}

void FNarrativeNode_CustomOutputDetails::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	// For backward compatability, these localized texts are in NarrativeNode_CustomOutputDetails, 
	//  not NarrativeNode_CustomNodeBase, so passing them in to a common function.

	static const FText CustomRowNameText = LOCTEXT("CustomRowName", "Event Name");
	static const FText EventNameText = LOCTEXT("EventName", "Event Name");

	CustomizeDetailsInternal(DetailLayout, CustomRowNameText, EventNameText);
}

IDetailCategoryBuilder& FNarrativeNode_CustomOutputDetails::CreateDetailCategory(IDetailLayoutBuilder& DetailLayout) const
{
	return DetailLayout.EditCategory("CustomOutput", LOCTEXT("CustomEventsCategory", "Custom Output"));
}

TArray<FName> FNarrativeNode_CustomOutputDetails::BuildEventNames(const UNarrativeAsset& NarrativeAsset) const
{
	return NarrativeAsset.GetCustomOutputs();
}

#undef LOCTEXT_NAMESPACE
