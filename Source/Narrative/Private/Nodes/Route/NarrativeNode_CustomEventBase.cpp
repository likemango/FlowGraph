// Copyright XiaoYao

#include "Nodes/Route/NarrativeNode_CustomEventBase.h"
#include "NarrativeSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativeNode_CustomEventBase)

UNarrativeNode_CustomEventBase::UNarrativeNode_CustomEventBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Route");
	NodeStyle = ENarrativeNodeStyle::InOut;
#endif

	AllowedSignalModes = {ENarrativeSignalMode::Enabled, ENarrativeSignalMode::Disabled};
}

void UNarrativeNode_CustomEventBase::SetEventName(const FName& InEventName)
{
	if (EventName != InEventName)
	{
		EventName = InEventName;

#if WITH_EDITOR
		// Must reconstruct the visual representation if anything that is included in AdaptiveNodeTitles changes
		OnReconstructionRequested.ExecuteIfBound();
#endif // WITH_EDITOR
	}
}

#if WITH_EDITOR

FString UNarrativeNode_CustomEventBase::GetNodeDescription() const
{
	if (UNarrativeSettings::Get()->bUseAdaptiveNodeTitles)
	{
		return Super::GetNodeDescription();
	}

	return EventName.ToString();
}

EDataValidationResult UNarrativeNode_CustomEventBase::ValidateNode()
{
	if (EventName.IsNone())
	{
		ValidationLog.Error<UNarrativeNode>(TEXT("Event Name is empty!"), this);
		return EDataValidationResult::Invalid;
	}

	return EDataValidationResult::Valid;
}
#endif
