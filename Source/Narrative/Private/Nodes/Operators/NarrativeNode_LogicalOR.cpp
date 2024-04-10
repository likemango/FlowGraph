// Copyright XiaoYao

#include "Nodes/Operators/NarrativeNode_LogicalOR.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativeNode_LogicalOR)

UNarrativeNode_LogicalOR::UNarrativeNode_LogicalOR(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bEnabled(true)
	, ExecutionLimit(1)
	, ExecutionCount(0)
{
#if WITH_EDITOR
	Category = TEXT("Operators");
	NodeStyle = ENarrativeNodeStyle::Logic;
#endif

	SetNumberedInputPins(0, 1);
	InputPins.Add(FNarrativePin(TEXT("Enable"), TEXT("Enabling resets Execution Count")));
	InputPins.Add(FNarrativePin(TEXT("Disable"), TEXT("Disabling resets Execution Count")));
}

void UNarrativeNode_LogicalOR::ExecuteInput(const FName& PinName)
{
	if (PinName == TEXT("Enable"))
	{
		if (!bEnabled)
		{
			ResetCounter();
			bEnabled = true;
		}
		return;
	}

	if (PinName == TEXT("Disable"))
	{
		if (bEnabled)
		{
			bEnabled = false;
			Finish();
		}
		return;
	}

	if (bEnabled && PinName.ToString().IsNumeric())
	{
		ExecutionCount++;
		if (ExecutionLimit > 0 && ExecutionCount == ExecutionLimit)
		{
			bEnabled = false;
		}

		TriggerFirstOutput(true);
	}
}

void UNarrativeNode_LogicalOR::Cleanup()
{
	ResetCounter();
}

void UNarrativeNode_LogicalOR::ResetCounter()
{
	ExecutionCount = 0;
}
