// Copyright XiaoYao

#include "Nodes/World/NarrativeNode_OnNotifyFromActor.h"
#include "NarrativeComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativeNode_OnNotifyFromActor)

UNarrativeNode_OnNotifyFromActor::UNarrativeNode_OnNotifyFromActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bRetroactive(false)
{
#if WITH_EDITOR
	Category = TEXT("Notifies");
	NodeStyle = ENarrativeNodeStyle::Condition;
#endif
}

void UNarrativeNode_OnNotifyFromActor::ObserveActor(TWeakObjectPtr<AActor> Actor, TWeakObjectPtr<UNarrativeComponent> Component)
{
	if (!RegisteredActors.Contains(Actor))
	{
		RegisteredActors.Emplace(Actor, Component);
		Component->OnNotifyFromComponent.AddUObject(this, &UNarrativeNode_OnNotifyFromActor::OnNotifyFromComponent);

		if (bRetroactive && Component->GetRecentlySentNotifyTags().HasAnyExact(NotifyTags))
		{
			OnEventReceived();
		}
	}
}

void UNarrativeNode_OnNotifyFromActor::ForgetActor(TWeakObjectPtr<AActor> Actor, TWeakObjectPtr<UNarrativeComponent> Component)
{
	Component->OnNotifyFromComponent.RemoveAll(this);
}

void UNarrativeNode_OnNotifyFromActor::OnNotifyFromComponent(UNarrativeComponent* Component, const FGameplayTag& Tag)
{
	if (Component->IdentityTags.HasAnyExact(IdentityTags) && (!NotifyTags.IsValid() || NotifyTags.HasTagExact(Tag)))
	{
		OnEventReceived();
	}
}

#if WITH_EDITOR
FString UNarrativeNode_OnNotifyFromActor::GetNodeDescription() const
{
	return GetIdentityTagsDescription(IdentityTags) + LINE_TERMINATOR + GetNotifyTagsDescription(NotifyTags);
}
#endif
