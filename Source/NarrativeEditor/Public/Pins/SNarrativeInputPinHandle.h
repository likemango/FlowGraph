// Copyright XiaoYao

#pragma once

#include "EdGraphUtilities.h"
#include "SNarrativePinHandle.h"

class NARRATIVEEDITOR_API SNarrativeInputPinHandle : public SNarrativePinHandle
{
protected:
	virtual void RefreshNameList() override;
};

class FNarrativeInputPinHandleFactory final : public FGraphPanelPinFactory
{
public:
	virtual TSharedPtr<class SGraphPin> CreatePin(class UEdGraphPin* InPin) const override;
};
