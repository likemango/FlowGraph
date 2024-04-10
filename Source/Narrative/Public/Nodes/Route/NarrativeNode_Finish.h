// Copyright XiaoYao

#pragma once

#include "Nodes/NarrativeNode.h"
#include "NarrativeNode_Finish.generated.h"

/**
 * Finish execution of this Narrative Asset
 * All active nodes and sub graphs will be deactivated
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Finish"))
class NARRATIVE_API UNarrativeNode_Finish : public UNarrativeNode
{
	GENERATED_UCLASS_BODY()

protected:
	virtual bool CanFinishGraph() const override { return true; }
	virtual void ExecuteInput(const FName& PinName) override;
};
