// Copyright XiaoYao

#include "Nodes/World/NarrativeNode_OnActorUnregistered.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativeNode_OnActorUnregistered)

UNarrativeNode_OnActorUnregistered::UNarrativeNode_OnActorUnregistered(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UNarrativeNode_OnActorUnregistered::ObserveActor(TWeakObjectPtr<AActor> Actor, TWeakObjectPtr<UNarrativeComponent> Component)
{
	if (!RegisteredActors.Contains(Actor))
	{
		RegisteredActors.Emplace(Actor, Component);
	}
}

void UNarrativeNode_OnActorUnregistered::ForgetActor(TWeakObjectPtr<AActor> Actor, TWeakObjectPtr<UNarrativeComponent> Component)
{
	if (ActivationState == ENarrativeNodeState::Active)
	{
		OnEventReceived();
	}
}
