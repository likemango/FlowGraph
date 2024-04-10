// Copyright XiaoYao

#include "Nodes/World/NarrativeNode_NotifyActor.h"
#include "NarrativeComponent.h"
#include "NarrativeSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativeNode_NotifyActor)

UNarrativeNode_NotifyActor::UNarrativeNode_NotifyActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, MatchType(EGameplayContainerMatchType::All)
	, bExactMatch(true)
	, NetMode(ENarrativeNetMode::Authority)
{
#if WITH_EDITOR
	Category = TEXT("Notifies");
#endif
}

void UNarrativeNode_NotifyActor::ExecuteInput(const FName& PinName)
{
	if (const UNarrativeSubsystem* NarrativeSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UNarrativeSubsystem>())
	{
		for (const TWeakObjectPtr<UNarrativeComponent>& Component : NarrativeSubsystem->GetComponents<UNarrativeComponent>(IdentityTags, MatchType, bExactMatch))
		{
			Component->NotifyFromGraph(NotifyTags, NetMode);
		}
	}

	TriggerFirstOutput(true);
}

#if WITH_EDITOR
FString UNarrativeNode_NotifyActor::GetNodeDescription() const
{
	return GetIdentityTagsDescription(IdentityTags) + LINE_TERMINATOR + GetNotifyTagsDescription(NotifyTags);
}

EDataValidationResult UNarrativeNode_NotifyActor::ValidateNode()
{
	if (IdentityTags.IsEmpty())
	{
		ValidationLog.Error<UNarrativeNode>(*UNarrativeNode::MissingIdentityTag, this);
		return EDataValidationResult::Invalid;
	}

	return EDataValidationResult::Valid;
}
#endif
