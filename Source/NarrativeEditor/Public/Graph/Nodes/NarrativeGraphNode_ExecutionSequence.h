// Copyright XiaoYao

#pragma once

#include "Graph/Nodes/NarrativeGraphNode.h"
#include "NarrativeGraphNode_ExecutionSequence.generated.h"

UCLASS()
class NARRATIVEEDITOR_API UNarrativeGraphNode_ExecutionSequence : public UNarrativeGraphNode
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	// --
};
