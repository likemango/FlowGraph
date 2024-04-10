// Copyright XiaoYao

#pragma once

#include "IDetailCustomization.h"

class FNarrativeNode_ComponentObserverDetails final : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShareable(new FNarrativeNode_ComponentObserverDetails);
	}

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
};
