// Copyright XiaoYao

#include "Nodes/Operators/NarrativeNode_LogicalAND.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativeNode_LogicalAND)

UNarrativeNode_LogicalAND::UNarrativeNode_LogicalAND(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Operators");
	NodeStyle = ENarrativeNodeStyle::Logic;
#endif

	SetNumberedInputPins(0, 1);
}

void UNarrativeNode_LogicalAND::ExecuteInput(const FName& PinName)
{
	ExecutedInputNames.Add(PinName);

	if (ExecutedInputNames.Num() == InputPins.Num())
	{
		TriggerFirstOutput(true);
	}
}

void UNarrativeNode_LogicalAND::Cleanup()
{
	ExecutedInputNames.Empty();
}
