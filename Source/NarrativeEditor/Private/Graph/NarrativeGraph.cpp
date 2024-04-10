// Copyright XiaoYao

#include "Graph/NarrativeGraph.h"
#include "Graph/NarrativeGraphSchema.h"
#include "Graph/NarrativeGraphSchema_Actions.h"
#include "Graph/Nodes/NarrativeGraphNode.h"

#include "Nodes/NarrativeNode.h"

#include "Editor.h"
#include "Kismet2/BlueprintEditorUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativeGraph)

void FNarrativeGraphInterface::OnInputTriggered(UEdGraphNode* GraphNode, const int32 Index) const
{
	CastChecked<UNarrativeGraphNode>(GraphNode)->OnInputTriggered(Index);
}

void FNarrativeGraphInterface::OnOutputTriggered(UEdGraphNode* GraphNode, const int32 Index) const
{
	CastChecked<UNarrativeGraphNode>(GraphNode)->OnOutputTriggered(Index);
}

UNarrativeGraph::UNarrativeGraph(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	if (!UNarrativeAsset::GetNarrativeGraphInterface().IsValid())
	{
		UNarrativeAsset::SetNarrativeGraphInterface(MakeShared<FNarrativeGraphInterface>());
	}
}

UEdGraph* UNarrativeGraph::CreateGraph(UNarrativeAsset* InNarrativeAsset)
{
	return CreateGraph(InNarrativeAsset, UNarrativeGraphSchema::StaticClass());
}

UEdGraph* UNarrativeGraph::CreateGraph(UNarrativeAsset* InNarrativeAsset, TSubclassOf<UNarrativeGraphSchema> NarrativeSchema)
{
	check(NarrativeSchema);
	UEdGraph* NewGraph = CastChecked<UNarrativeGraph>(FBlueprintEditorUtils::CreateNewGraph(InNarrativeAsset, NAME_None, StaticClass(), NarrativeSchema));
	NewGraph->bAllowDeletion = false;

	InNarrativeAsset->NarrativeGraph = NewGraph;
	NewGraph->GetSchema()->CreateDefaultNodesForGraph(*NewGraph);

	return NewGraph;
}

void UNarrativeGraph::RefreshGraph()
{
	// don't run fixup in PIE
	if (GEditor && !GEditor->PlayWorld)
	{
		// check if all Graph Nodes have expected, up-to-date type
		CastChecked<UNarrativeGraphSchema>(GetSchema())->GatherNativeNodes();
		for (const TPair<FGuid, UNarrativeNode*>& Node : GetNarrativeAsset()->GetNodes())
		{
			if (UNarrativeNode* NarrativeNode = Node.Value)
			{
				const UClass* ExpectGraphNodeClass = UNarrativeGraphSchema::GetAssignedGraphNodeClass(NarrativeNode->GetClass());
				if (NarrativeNode->GetGraphNode() && NarrativeNode->GetGraphNode()->GetClass() != ExpectGraphNodeClass)
				{
					// Create a new Narrative Graph Node of proper type
					FNarrativeGraphSchemaAction_NewNode::RecreateNode(this, NarrativeNode->GetGraphNode(), NarrativeNode);
				}
			}
		}

		// refresh nodes
		TArray<UNarrativeGraphNode*> NarrativeGraphNodes;
		GetNodesOfClass<UNarrativeGraphNode>(NarrativeGraphNodes);
		for (UNarrativeGraphNode* GraphNode : NarrativeGraphNodes)
		{
			GraphNode->OnGraphRefresh();
		}
	}
}

void UNarrativeGraph::NotifyGraphChanged()
{
	GetNarrativeAsset()->HarvestNodeConnections();

	Super::NotifyGraphChanged();
}

UNarrativeAsset* UNarrativeGraph::GetNarrativeAsset() const
{
	return GetTypedOuter<UNarrativeAsset>();
}
