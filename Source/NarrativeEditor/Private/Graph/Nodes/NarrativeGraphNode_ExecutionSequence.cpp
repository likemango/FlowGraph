// Copyright XiaoYao

#include "Graph/Nodes/NarrativeGraphNode_ExecutionSequence.h"
#include "Nodes/Route/NarrativeNode_ExecutionMultiGate.h"
#include "Nodes/Route/NarrativeNode_ExecutionSequence.h"

#include "Textures/SlateIcon.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativeGraphNode_ExecutionSequence)

UNarrativeGraphNode_ExecutionSequence::UNarrativeGraphNode_ExecutionSequence(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AssignedNodeClasses = {UNarrativeNode_ExecutionSequence::StaticClass(), UNarrativeNode_ExecutionMultiGate::StaticClass()};
}

FSlateIcon UNarrativeGraphNode_ExecutionSequence::GetIconAndTint(FLinearColor& OutColor) const
{
	static FSlateIcon Icon("NarrativeEditorStyle", "GraphEditor.Sequence_16x");
	return Icon;
}
