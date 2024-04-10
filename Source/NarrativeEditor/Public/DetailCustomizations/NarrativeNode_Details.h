// Copyright XiaoYao

#pragma once

#include "IDetailCustomization.h"

class FNarrativeNode_Details final : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShareable(new FNarrativeNode_Details());
	}

	// IDetailCustomization
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;
	// --
};
