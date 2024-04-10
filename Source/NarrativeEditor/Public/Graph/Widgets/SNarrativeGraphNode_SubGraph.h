// Copyright XiaoYao

#pragma once

#include "Graph/Widgets/SNarrativeGraphNode.h"

class NARRATIVEEDITOR_API SNarrativeGraphNode_SubGraph : public SNarrativeGraphNode
{
protected:
	// SGraphNode
	virtual TSharedPtr<SToolTip> GetComplexTooltip() override;
	// --
};
