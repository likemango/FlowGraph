// Copyright XiaoYao

#pragma once

#include "Nodes/World/NarrativeNode_ComponentObserver.h"
#include "NarrativeNode_OnActorRegistered.generated.h"

/**
 * Triggers output when Narrative Component with matching Identity Tag appears in the world
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "On Actor Registered", Keywords = "bind"))
class NARRATIVE_API UNarrativeNode_OnActorRegistered : public UNarrativeNode_ComponentObserver
{
	GENERATED_UCLASS_BODY()

protected:
	virtual void ObserveActor(TWeakObjectPtr<AActor> Actor, TWeakObjectPtr<UNarrativeComponent> Component) override;
};
