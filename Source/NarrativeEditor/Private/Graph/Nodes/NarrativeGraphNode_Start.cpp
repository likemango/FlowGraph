// Copyright XiaoYao

#include "Graph/Nodes/NarrativeGraphNode_Start.h"
#include "Graph/Widgets/SNarrativeGraphNode_Start.h"

#include "Nodes/Route/NarrativeNode_Start.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativeGraphNode_Start)

UNarrativeGraphNode_Start::UNarrativeGraphNode_Start(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AssignedNodeClasses = {UNarrativeNode_Start::StaticClass()};
}

TSharedPtr<SGraphNode> UNarrativeGraphNode_Start::CreateVisualWidget()
{
	return SNew(SNarrativeGraphNode_Start, this);
}
