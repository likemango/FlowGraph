// Copyright XiaoYao

#include "Nodes/World/NarrativeNode_ComponentObserver.h"
#include "NarrativeSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativeNode_ComponentObserver)

UNarrativeNode_ComponentObserver::UNarrativeNode_ComponentObserver(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, IdentityMatchType(ENarrativeTagContainerMatchType::HasAnyExact)
	, SuccessLimit(1)
	, SuccessCount(0)
{
#if WITH_EDITOR
	NodeStyle = ENarrativeNodeStyle::Condition;
	Category = TEXT("World");
#endif

	InputPins = {FNarrativePin(TEXT("Start")), FNarrativePin(TEXT("Stop"))};
	OutputPins = {FNarrativePin(TEXT("Success")), FNarrativePin(TEXT("Completed")), FNarrativePin(TEXT("Stopped"))};
}

void UNarrativeNode_ComponentObserver::ExecuteInput(const FName& PinName)
{
	if (IdentityTags.IsValid())
	{
		if (PinName == TEXT("Start"))
		{
			StartObserving();
		}
		else if (PinName == TEXT("Stop"))
		{
			TriggerOutput(TEXT("Stopped"), true);
		}
	}
	else
	{
		LogError(MissingIdentityTag);
	}
}

void UNarrativeNode_ComponentObserver::OnLoad_Implementation()
{
	if (IdentityTags.IsValid())
	{
		StartObserving();
	}
}

void UNarrativeNode_ComponentObserver::StartObserving()
{
	if (UNarrativeSubsystem* NarrativeSubsystem = GetNarrativeSubsystem())
	{
		// translate Narrative name into engine types
		const EGameplayContainerMatchType ContainerMatchType = (IdentityMatchType == ENarrativeTagContainerMatchType::HasAny || IdentityMatchType == ENarrativeTagContainerMatchType::HasAnyExact) ? EGameplayContainerMatchType::Any : EGameplayContainerMatchType::All;
		const bool bExactMatch = (IdentityMatchType == ENarrativeTagContainerMatchType::HasAnyExact || IdentityMatchType == ENarrativeTagContainerMatchType::HasAllExact);

		// collect already registered components
		for (const TWeakObjectPtr<UNarrativeComponent>& FoundComponent : NarrativeSubsystem->GetComponents<UNarrativeComponent>(IdentityTags, ContainerMatchType, bExactMatch))
		{
			ObserveActor(FoundComponent->GetOwner(), FoundComponent);
			
			// node might finish work immediately as the effect of ObserveActor()
			// we should terminate iteration in this case
			if (GetActivationState() != ENarrativeNodeState::Active)
			{
				return;
			}
		}
		
		NarrativeSubsystem->OnComponentRegistered.AddUniqueDynamic(this, &UNarrativeNode_ComponentObserver::OnComponentRegistered);
		NarrativeSubsystem->OnComponentTagAdded.AddUniqueDynamic(this, &UNarrativeNode_ComponentObserver::OnComponentTagAdded);
		NarrativeSubsystem->OnComponentTagRemoved.AddUniqueDynamic(this, &UNarrativeNode_ComponentObserver::OnComponentTagRemoved);
		NarrativeSubsystem->OnComponentUnregistered.AddUniqueDynamic(this, &UNarrativeNode_ComponentObserver::OnComponentUnregistered);
	}
}

void UNarrativeNode_ComponentObserver::StopObserving()
{
	if (UNarrativeSubsystem* NarrativeSubsystem = GetNarrativeSubsystem())
	{
		NarrativeSubsystem->OnComponentRegistered.RemoveAll(this);
		NarrativeSubsystem->OnComponentUnregistered.RemoveAll(this);
		NarrativeSubsystem->OnComponentTagAdded.RemoveAll(this);
		NarrativeSubsystem->OnComponentTagRemoved.RemoveAll(this);
	}
}

void UNarrativeNode_ComponentObserver::OnComponentRegistered(UNarrativeComponent* Component)
{
	if (!RegisteredActors.Contains(Component->GetOwner()) && NarrativeTypes::HasMatchingTags(Component->IdentityTags, IdentityTags, IdentityMatchType) == true)
	{
		ObserveActor(Component->GetOwner(), Component);
	}
}

void UNarrativeNode_ComponentObserver::OnComponentTagAdded(UNarrativeComponent* Component, const FGameplayTagContainer& AddedTags)
{
	if (!RegisteredActors.Contains(Component->GetOwner()) && NarrativeTypes::HasMatchingTags(Component->IdentityTags, IdentityTags, IdentityMatchType) == true)
	{
		ObserveActor(Component->GetOwner(), Component);
	}
}

void UNarrativeNode_ComponentObserver::OnComponentTagRemoved(UNarrativeComponent* Component, const FGameplayTagContainer& RemovedTags)
{
	if (RegisteredActors.Contains(Component->GetOwner()) && NarrativeTypes::HasMatchingTags(Component->IdentityTags, IdentityTags, IdentityMatchType) == false)
	{
		RegisteredActors.Remove(Component->GetOwner());
		ForgetActor(Component->GetOwner(), Component);
	}
}

void UNarrativeNode_ComponentObserver::OnComponentUnregistered(UNarrativeComponent* Component)
{
	if (RegisteredActors.Contains(Component->GetOwner()))
	{
		RegisteredActors.Remove(Component->GetOwner());
		ForgetActor(Component->GetOwner(), Component);
	}
}

void UNarrativeNode_ComponentObserver::OnEventReceived()
{
	TriggerFirstOutput(false);

	SuccessCount++;
	if (SuccessLimit > 0 && SuccessCount == SuccessLimit)
	{
		TriggerOutput(TEXT("Completed"), true);
	}
}

void UNarrativeNode_ComponentObserver::Cleanup()
{
	StopObserving();

	if (RegisteredActors.Num() > 0)
	{
		for (const TPair<TWeakObjectPtr<AActor>, TWeakObjectPtr<UNarrativeComponent>>& RegisteredActor : RegisteredActors)
		{
			ForgetActor(RegisteredActor.Key, RegisteredActor.Value);
		}
	}
	RegisteredActors.Empty();

	SuccessCount = 0;
}

#if WITH_EDITOR
FString UNarrativeNode_ComponentObserver::GetNodeDescription() const
{
	return GetIdentityTagsDescription(IdentityTags);
}

EDataValidationResult UNarrativeNode_ComponentObserver::ValidateNode()
{
	if (IdentityTags.IsEmpty())
	{
		ValidationLog.Error<UNarrativeNode>(*UNarrativeNode::MissingIdentityTag, this);
		return EDataValidationResult::Invalid;
	}

	return EDataValidationResult::Valid;
}

FString UNarrativeNode_ComponentObserver::GetStatusString() const
{
	if (ActivationState == ENarrativeNodeState::Active && RegisteredActors.Num() == 0)
	{
		return NoActorsFound;
	}

	return FString();
}
#endif
