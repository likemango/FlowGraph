// Copyright XiaoYao

#include "Graph/Nodes/NarrativeGraphNode_Finish.h"
#include "Graph/Widgets/SNarrativeGraphNode_Finish.h"

#include "Nodes/Route/NarrativeNode_Finish.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativeGraphNode_Finish)

UNarrativeGraphNode_Finish::UNarrativeGraphNode_Finish(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AssignedNodeClasses = {UNarrativeNode_Finish::StaticClass()};
}

TSharedPtr<SGraphNode> UNarrativeGraphNode_Finish::CreateVisualWidget()
{
	return SNew(SNarrativeGraphNode_Finish, this);
}
