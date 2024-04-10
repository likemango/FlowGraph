// Copyright XiaoYao

#pragma once

#include "IDetailCustomization.h"

class FNarrativeNode_PlayLevelSequenceDetails final : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShareable(new FNarrativeNode_PlayLevelSequenceDetails);
	}

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
};
