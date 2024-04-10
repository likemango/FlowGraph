// Copyright XiaoYao

#pragma once

#include "Graph/Nodes/NarrativeGraphNode.h"
#include "NarrativeGraphNode_SubGraph.generated.h"

UCLASS()
class NARRATIVEEDITOR_API UNarrativeGraphNode_SubGraph : public UNarrativeGraphNode
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode
	virtual TSharedPtr<SGraphNode> CreateVisualWidget() override;
	// --
};
