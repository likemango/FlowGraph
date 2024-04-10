// Copyright XiaoYao

#pragma once

#include "EdGraphUtilities.h"
#include "SNarrativePinHandle.h"

class NARRATIVEEDITOR_API SNarrativeOutputPinHandle : public SNarrativePinHandle
{
protected:
	virtual void RefreshNameList() override;
};

class FNarrativeOutputPinHandleFactory final : public FGraphPanelPinFactory
{
public:
	virtual TSharedPtr<class SGraphPin> CreatePin(class UEdGraphPin* InPin) const override;
};
