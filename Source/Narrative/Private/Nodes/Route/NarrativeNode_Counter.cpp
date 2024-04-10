// Copyright XiaoYao

#include "Nodes/Route/NarrativeNode_Counter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativeNode_Counter)

UNarrativeNode_Counter::UNarrativeNode_Counter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, Goal(2)
	, CurrentSum(0)
{
#if WITH_EDITOR
	Category = TEXT("Route");
	NodeStyle = ENarrativeNodeStyle::Condition;
#endif

	InputPins.Empty();
	InputPins.Add(FNarrativePin(TEXT("Increment")));
	InputPins.Add(FNarrativePin(TEXT("Decrement")));
	InputPins.Add(FNarrativePin(TEXT("Skip")));

	OutputPins.Empty();
	OutputPins.Add(FNarrativePin(TEXT("Zero")));
	OutputPins.Add(FNarrativePin(TEXT("Step")));
	OutputPins.Add(FNarrativePin(TEXT("Goal")));
	OutputPins.Add(FNarrativePin(TEXT("Skipped")));
}

void UNarrativeNode_Counter::ExecuteInput(const FName& PinName)
{
	if (PinName == TEXT("Increment"))
	{
		CurrentSum++;
		if (CurrentSum == Goal)
		{
			TriggerOutput(TEXT("Goal"), true);
		}
		else
		{
			TriggerOutput(TEXT("Step"));
		}
		return;
	}

	if (PinName == TEXT("Decrement"))
	{
		CurrentSum--;
		if (CurrentSum == 0)
		{
			TriggerOutput(TEXT("Zero"), true);
		}
		else
		{
			TriggerOutput(TEXT("Step"));
		}
		return;
	}

	if (PinName == TEXT("Skip"))
	{
		TriggerOutput(TEXT("Skipped"), true);
	}
}

void UNarrativeNode_Counter::Cleanup()
{
	CurrentSum = 0;
}

#if WITH_EDITOR
FString UNarrativeNode_Counter::GetNodeDescription() const
{
	return FString::FromInt(Goal);
}

FString UNarrativeNode_Counter::GetStatusString() const
{
	return FString::FromInt(CurrentSum);
}
#endif
