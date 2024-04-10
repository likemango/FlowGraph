// Copyright XiaoYao

#include "Nodes/Route/NarrativeNode_Timer.h"

#include "Engine/World.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativeNode_Timer)

UNarrativeNode_Timer::UNarrativeNode_Timer(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, CompletionTime(1.0f)
	, StepTime(0.0f)
	, SumOfSteps(0.0f)
	, RemainingCompletionTime(0.0f)
	, RemainingStepTime(0.0f)
{
#if WITH_EDITOR
	Category = TEXT("Route");
	NodeStyle = ENarrativeNodeStyle::Latent;
#endif

	InputPins.Add(FNarrativePin(TEXT("Skip")));
	InputPins.Add(FNarrativePin(TEXT("Restart")));

	OutputPins.Empty();
	OutputPins.Add(FNarrativePin(TEXT("Completed")));
	OutputPins.Add(FNarrativePin(TEXT("Step")));
	OutputPins.Add(FNarrativePin(TEXT("Skipped")));
}

void UNarrativeNode_Timer::ExecuteInput(const FName& PinName)
{
	if (PinName == TEXT("In"))
	{
		if (CompletionTimerHandle.IsValid() || StepTimerHandle.IsValid())
		{
			LogError(TEXT("Timer already active"));
			return;
		}

		SetTimer();
	}
	else if (PinName == TEXT("Skip"))
	{
		TriggerOutput(TEXT("Skipped"), true);
	}
	else if (PinName == TEXT("Restart"))
	{
		Restart();
	}
}

void UNarrativeNode_Timer::SetTimer()
{
	if (GetWorld())
	{
		if (StepTime > 0.0f)
		{
			GetWorld()->GetTimerManager().SetTimer(StepTimerHandle, this, &UNarrativeNode_Timer::OnStep, StepTime, true);
		}

		if (CompletionTime > UE_KINDA_SMALL_NUMBER)
		{
			GetWorld()->GetTimerManager().SetTimer(CompletionTimerHandle, this, &UNarrativeNode_Timer::OnCompletion, CompletionTime, false);
		}
		else
		{
			GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UNarrativeNode_Timer::OnCompletion);
		}
	}
	else
	{
		LogError(TEXT("No valid world"));
		TriggerOutput(TEXT("Completed"), true);
	}
}

void UNarrativeNode_Timer::Restart()
{
	Cleanup();

	RemainingStepTime = 0.0f;
	RemainingCompletionTime = 0.0f;

	SetTimer();
}

void UNarrativeNode_Timer::OnStep()
{
	SumOfSteps += StepTime;

	if (SumOfSteps >= CompletionTime)
	{
		TriggerOutput(TEXT("Completed"), true);
	}
	else
	{
		TriggerOutput(TEXT("Step"));
	}
}

void UNarrativeNode_Timer::OnCompletion()
{
	TriggerOutput(TEXT("Completed"), true);
}

void UNarrativeNode_Timer::Cleanup()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(CompletionTimerHandle);
	}
	CompletionTimerHandle.Invalidate();

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(StepTimerHandle);
	}
	StepTimerHandle.Invalidate();

	SumOfSteps = 0.0f;
}

void UNarrativeNode_Timer::OnSave_Implementation()
{
	if (GetWorld())
	{
		if (CompletionTimerHandle.IsValid())
		{
			RemainingCompletionTime = GetWorld()->GetTimerManager().GetTimerRemaining(CompletionTimerHandle);
		}

		if (StepTimerHandle.IsValid())
		{
			RemainingStepTime = GetWorld()->GetTimerManager().GetTimerRemaining(StepTimerHandle);
		}
	}
}

void UNarrativeNode_Timer::OnLoad_Implementation()
{
	if (RemainingStepTime > 0.0f || RemainingCompletionTime > 0.0f)
	{
		if (RemainingStepTime > 0.0f)
		{
			GetWorld()->GetTimerManager().SetTimer(StepTimerHandle, this, &UNarrativeNode_Timer::OnStep, StepTime, true, RemainingStepTime);
		}

		GetWorld()->GetTimerManager().SetTimer(CompletionTimerHandle, this, &UNarrativeNode_Timer::OnCompletion, RemainingCompletionTime, false);

		RemainingStepTime = 0.0f;
		RemainingCompletionTime = 0.0f;
	}
}

#if WITH_EDITOR
FString UNarrativeNode_Timer::GetNodeDescription() const
{
	if (CompletionTime > UE_KINDA_SMALL_NUMBER)
	{
		if (StepTime > 0.0f)
		{
			return FString::Printf(TEXT("%.*f, step by %.*f"), 2, CompletionTime, 2, StepTime);
		}

		return FString::Printf(TEXT("%.*f"), 2, CompletionTime);
	}

	return TEXT("Completes in next tick");
}

FString UNarrativeNode_Timer::GetStatusString() const
{
	if (StepTime > 0.0f)
	{
		return FString::Printf(TEXT("Progress: %.*f"), 2, SumOfSteps);
	}

	if (CompletionTimerHandle.IsValid() && GetWorld())
	{
		return FString::Printf(TEXT("Progress: %.*f"), 2, GetWorld()->GetTimerManager().GetTimerElapsed(CompletionTimerHandle));
	}

	return FString();
}
#endif
