// Copyright XiaoYao

#pragma once

#include "Nodes/NarrativeNode.h"
#include "NarrativeNode_CustomEventBase.generated.h"

/**
 * Base class for nodes used to receive/send events between graphs
 */
UCLASS(Abstract, NotBlueprintable)
class NARRATIVE_API UNarrativeNode_CustomEventBase : public UNarrativeNode
{
	GENERATED_UCLASS_BODY()

protected:
	UPROPERTY()
	FName EventName;

public:
	void SetEventName(const FName& InEventName);
	const FName& GetEventName() const { return EventName; }

#if WITH_EDITOR
public:
	virtual FString GetNodeDescription() const override;
	virtual EDataValidationResult ValidateNode() override;
#endif
};
