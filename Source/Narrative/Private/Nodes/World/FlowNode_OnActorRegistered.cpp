// Copyright XiaoYao

#include "Nodes/World/NarrativeNode_OnActorRegistered.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativeNode_OnActorRegistered)

UNarrativeNode_OnActorRegistered::UNarrativeNode_OnActorRegistered(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UNarrativeNode_OnActorRegistered::ObserveActor(TWeakObjectPtr<AActor> Actor, TWeakObjectPtr<UNarrativeComponent> Component)
{
	if (!RegisteredActors.Contains(Actor))
	{
		RegisteredActors.Emplace(Actor, Component);
		OnEventReceived();
	}
}
