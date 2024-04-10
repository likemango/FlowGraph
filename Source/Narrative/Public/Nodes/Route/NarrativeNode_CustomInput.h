// Copyright XiaoYao

#pragma once

#include "NarrativeNode_CustomEventBase.h"
#include "NarrativeNode_CustomInput.generated.h"

/**
 * Triggers output upon activation of Input (matching this EventName) on the SubGraph node containing this graph
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Custom Input"))
class NARRATIVE_API UNarrativeNode_CustomInput : public UNarrativeNode_CustomEventBase
{
	GENERATED_UCLASS_BODY()

	friend class UNarrativeAsset;

protected:
	virtual void ExecuteInput(const FName& PinName) override;

#if WITH_EDITOR
public:
	virtual FText GetNodeTitle() const override;
#endif
};
