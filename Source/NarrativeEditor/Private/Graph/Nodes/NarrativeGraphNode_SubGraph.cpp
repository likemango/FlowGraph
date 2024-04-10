// Copyright XiaoYao

#include "Graph/Nodes/NarrativeGraphNode_SubGraph.h"
#include "Graph/Widgets/SNarrativeGraphNode_SubGraph.h"

#include "Nodes/Route/NarrativeNode_SubGraph.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativeGraphNode_SubGraph)

UNarrativeGraphNode_SubGraph::UNarrativeGraphNode_SubGraph(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AssignedNodeClasses = {UNarrativeNode_SubGraph::StaticClass()};
}

TSharedPtr<SGraphNode> UNarrativeGraphNode_SubGraph::CreateVisualWidget()
{
	return SNew(SNarrativeGraphNode_SubGraph, this);
}
