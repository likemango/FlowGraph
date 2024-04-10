// Copyright XiaoYao

#include "Graph/Nodes/NarrativeGraphNode_Reroute.h"
#include "SGraphNodeKnot.h"

#include "Nodes/Route/NarrativeNode_Reroute.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativeGraphNode_Reroute)

UNarrativeGraphNode_Reroute::UNarrativeGraphNode_Reroute(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AssignedNodeClasses = {UNarrativeNode_Reroute::StaticClass()};
}

TSharedPtr<SGraphNode> UNarrativeGraphNode_Reroute::CreateVisualWidget()
{
	return SNew(SGraphNodeKnot, this);
}

bool UNarrativeGraphNode_Reroute::ShouldDrawNodeAsControlPointOnly(int32& OutInputPinIndex, int32& OutOutputPinIndex) const
{
	OutInputPinIndex = 0;
	OutOutputPinIndex = 1;
	return true;
}
