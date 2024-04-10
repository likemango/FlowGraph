// Copyright XiaoYao

#include "NarrativeOwnerFunctionParams.h"
#include "Nodes/NarrativeNode.h"
#include "Nodes/World/NarrativeNode_CallOwnerFunction.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativeOwnerFunctionParams)

UNarrativeOwnerFunctionParams::UNarrativeOwnerFunctionParams()
{
#if WITH_EDITOR
	InputNames.Add(UNarrativeNode::DefaultInputPin.PinName);
	OutputNames.Add(UNarrativeNode::DefaultOutputPin.PinName);
#endif
}

void UNarrativeOwnerFunctionParams::PreExecute(UNarrativeNode_CallOwnerFunction& InSourceNode, const FName& InputPinName)
{
	SourceNode = &InSourceNode;
	ExecutedInputPinName = InputPinName;

	BP_PreExecute();
}

void UNarrativeOwnerFunctionParams::PostExecute()
{
	BP_PostExecute();

	SourceNode = nullptr;
	ExecutedInputPinName = NAME_None;
}

bool UNarrativeOwnerFunctionParams::ShouldFinishForOutputName_Implementation(const FName& OutputName) const
{
	// Blueprint subclasses may want to declare certain outputs as "non-finishing"
	//  but by default, all outputs are 'finishing'
	return true;
}

TArray<FName> UNarrativeOwnerFunctionParams::BP_GetInputNames() const
{
	if (IsValid(SourceNode))
	{
		return SourceNode->GetInputNames();
	}

	return TArray<FName>();
}

TArray<FName> UNarrativeOwnerFunctionParams::BP_GetOutputNames() const
{
	if (IsValid(SourceNode))
	{
		return SourceNode->GetOutputNames();
	}

	return TArray<FName>();
}
