// Copyright XiaoYao

#pragma once

#include "Nodes/World/NarrativeNode_ComponentObserver.h"
#include "NarrativeNode_OnActorUnregistered.generated.h"

/**
 * Triggers output when Narrative Component with matching Identity Tag disappears from the world
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "On Actor Unregistered", Keywords = "unbind"))
class NARRATIVE_API UNarrativeNode_OnActorUnregistered : public UNarrativeNode_ComponentObserver
{
	GENERATED_UCLASS_BODY()

protected:
	virtual void ObserveActor(TWeakObjectPtr<AActor> Actor, TWeakObjectPtr<UNarrativeComponent> Component) override;
	virtual void ForgetActor(TWeakObjectPtr<AActor> Actor, TWeakObjectPtr<UNarrativeComponent> Component) override;
};
