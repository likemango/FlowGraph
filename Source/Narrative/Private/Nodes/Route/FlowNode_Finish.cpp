// Copyright XiaoYao

#include "Nodes/Route/NarrativeNode_Finish.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativeNode_Finish)

UNarrativeNode_Finish::UNarrativeNode_Finish(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Route");
	NodeStyle = ENarrativeNodeStyle::InOut;
#endif

	OutputPins = {};
	AllowedSignalModes = {ENarrativeSignalMode::Enabled, ENarrativeSignalMode::Disabled};
}

void UNarrativeNode_Finish::ExecuteInput(const FName& PinName)
{
	// this will call FinishNarrative()
	Finish();
}
