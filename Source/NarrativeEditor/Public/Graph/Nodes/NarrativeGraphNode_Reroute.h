// Copyright XiaoYao

#pragma once

#include "Graph/Nodes/NarrativeGraphNode.h"
#include "NarrativeGraphNode_Reroute.generated.h"

UCLASS()
class NARRATIVEEDITOR_API UNarrativeGraphNode_Reroute : public UNarrativeGraphNode
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode
	virtual TSharedPtr<SGraphNode> CreateVisualWidget() override;
	virtual bool ShouldDrawNodeAsControlPointOnly(int32& OutInputPinIndex, int32& OutOutputPinIndex) const override;
	// --
};
