// Copyright XiaoYao

#pragma once

#include "Nodes/NarrativeNode.h"
#include "NarrativeNode_Start.generated.h"

/**
 * Execution of the graph always starts from this node
 */
UCLASS(NotBlueprintable, NotPlaceable, meta = (DisplayName = "Start"))
class NARRATIVE_API UNarrativeNode_Start : public UNarrativeNode
{
	GENERATED_UCLASS_BODY()

	friend class UNarrativeAsset;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
