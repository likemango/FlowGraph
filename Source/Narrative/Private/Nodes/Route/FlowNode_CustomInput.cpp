// Copyright XiaoYao

#include "Nodes/Route/NarrativeNode_CustomInput.h"
#include "NarrativeSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativeNode_CustomInput)

#define LOCTEXT_NAMESPACE "NarrativeNode_CustomInput"

UNarrativeNode_CustomInput::UNarrativeNode_CustomInput(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InputPins.Empty();
}

void UNarrativeNode_CustomInput::ExecuteInput(const FName& PinName)
{
	TriggerFirstOutput(true);
}

#if WITH_EDITOR
FText UNarrativeNode_CustomInput::GetNodeTitle() const
{
	if (!EventName.IsNone() && UNarrativeSettings::Get()->bUseAdaptiveNodeTitles)
	{
		return FText::Format(LOCTEXT("CustomInputTitle", "{0} Input"), {FText::FromString(EventName.ToString())});
	}

	return Super::GetNodeTitle();
}
#endif

#undef LOCTEXT_NAMESPACE
