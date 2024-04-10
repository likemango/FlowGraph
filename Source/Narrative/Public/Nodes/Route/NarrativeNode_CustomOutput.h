// Copyright XiaoYao

#pragma once

#include "NarrativeNode_CustomEventBase.h"
#include "NarrativeNode_CustomOutput.generated.h"

/**
 * Triggers output on SubGraph node containing this graph
 * Triggered output name matches EventName selected on this node
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Custom Output"))
class NARRATIVE_API UNarrativeNode_CustomOutput final : public UNarrativeNode_CustomEventBase
{
	GENERATED_UCLASS_BODY()

protected:
	virtual void ExecuteInput(const FName& PinName) override;

#if WITH_EDITOR
	virtual FText GetNodeTitle() const override;
#endif
};
