// Copyright XiaoYao

#include "Nodes/Route/NarrativeNode_Start.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativeNode_Start)

UNarrativeNode_Start::UNarrativeNode_Start(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Route");
	NodeStyle = ENarrativeNodeStyle::InOut;
	bCanDelete = bCanDuplicate = false;
#endif

	InputPins = {};
	AllowedSignalModes = {ENarrativeSignalMode::Enabled, ENarrativeSignalMode::Disabled};
}

void UNarrativeNode_Start::ExecuteInput(const FName& PinName)
{
	TriggerFirstOutput(true);
}
