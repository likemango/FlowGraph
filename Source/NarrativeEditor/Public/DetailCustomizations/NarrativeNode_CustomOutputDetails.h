// Copyright XiaoYao

#pragma once

#include "NarrativeNode_CustomEventBaseDetails.h"
#include "Templates/SharedPointer.h"

class FNarrativeNode_CustomOutputDetails final : public FNarrativeNode_CustomEventBaseDetails
{
public:
	FNarrativeNode_CustomOutputDetails();

	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShareable(new FNarrativeNode_CustomOutputDetails());
	}

	// IDetailCustomization
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;
	// --

protected:
	virtual IDetailCategoryBuilder& CreateDetailCategory(IDetailLayoutBuilder& DetailLayout) const override;
	virtual TArray<FName> BuildEventNames(const UNarrativeAsset& NarrativeAsset) const override;
};
