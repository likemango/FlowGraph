// Copyright XiaoYao

#include "Nodes/Route/NarrativeNode_ExecutionSequence.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativeNode_ExecutionSequence)

UNarrativeNode_ExecutionSequence::UNarrativeNode_ExecutionSequence(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bSavePinExecutionState(true)
{
#if WITH_EDITOR
	Category = TEXT("Route");
	NodeStyle = ENarrativeNodeStyle::Logic;
#endif

	SetNumberedOutputPins(0, 1);
	AllowedSignalModes = {ENarrativeSignalMode::Enabled, ENarrativeSignalMode::Disabled};
}

void UNarrativeNode_ExecutionSequence::ExecuteInput(const FName& PinName)
{
	if (bSavePinExecutionState)
	{
		ExecuteNewConnections();
	}
	else
	{
		for (const FNarrativePin& Output : OutputPins)
		{
			TriggerOutput(Output.PinName, false);
		}

		Finish();
	}
}

void UNarrativeNode_ExecutionSequence::OnLoad_Implementation()
{
	ExecuteNewConnections();
}

void UNarrativeNode_ExecutionSequence::Cleanup()
{
	ExecutedConnections.Empty();
}

void UNarrativeNode_ExecutionSequence::ExecuteNewConnections()
{
	for (const FNarrativePin& Output : OutputPins)
	{
		const FConnectedPin& Connection = GetConnection(Output.PinName);
		if (!ExecutedConnections.Contains(Connection.NodeGuid))
		{
			ExecutedConnections.Emplace(Connection.NodeGuid);
			TriggerOutput(Output.PinName, false);
		}
	}

	Finish();
}

#if WITH_EDITOR
FString UNarrativeNode_ExecutionSequence::GetNodeDescription() const
{
	if (bSavePinExecutionState)
	{
		return TEXT("Saves pin execution state");
	}

	return Super::GetNodeDescription();
}
#endif
