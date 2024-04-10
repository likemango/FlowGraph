// Copyright XiaoYao

#pragma once

#include "Nodes/NarrativeNode.h"
#include "NarrativeNode_Reroute.generated.h"

/**
 * Reroute
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Reroute"))
class NARRATIVE_API UNarrativeNode_Reroute final : public UNarrativeNode
{
	GENERATED_UCLASS_BODY()
	
protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
