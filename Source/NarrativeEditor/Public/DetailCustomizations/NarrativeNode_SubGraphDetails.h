// Copyright XiaoYao

#pragma once

#include "IDetailCustomization.h"

class FNarrativeNode_SubGraphDetails final : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShareable(new FNarrativeNode_SubGraphDetails);
	}

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;
};
