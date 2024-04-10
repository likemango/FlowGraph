// Copyright XiaoYao

#include "Nodes/Route/NarrativeNode_Reroute.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativeNode_Reroute)

UNarrativeNode_Reroute::UNarrativeNode_Reroute(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Route");
#endif

	AllowedSignalModes = {ENarrativeSignalMode::Enabled, ENarrativeSignalMode::Disabled};
}

void UNarrativeNode_Reroute::ExecuteInput(const FName& PinName)
{
	TriggerFirstOutput(true);
}
