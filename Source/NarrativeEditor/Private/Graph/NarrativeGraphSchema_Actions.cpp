// Copyright XiaoYao

#include "Graph/NarrativeGraphSchema_Actions.h"

#include "Graph/NarrativeGraph.h"
#include "Graph/NarrativeGraphEditor.h"
#include "Graph/NarrativeGraphSchema.h"
#include "Graph/NarrativeGraphUtils.h"
#include "Graph/Nodes/NarrativeGraphNode.h"

#include "NarrativeAsset.h"
#include "Nodes/NarrativeNode.h"

#include "EdGraph/EdGraph.h"
#include "EdGraphNode_Comment.h"
#include "Editor.h"
#include "ScopedTransaction.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativeGraphSchema_Actions)

#define LOCTEXT_NAMESPACE "NarrativeGraphSchema_Actions"

/////////////////////////////////////////////////////
// Narrative Node

UEdGraphNode* FNarrativeGraphSchemaAction_NewNode::PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode /* = true*/)
{
	// prevent adding new nodes while playing
	if (GEditor->PlayWorld != nullptr)
	{
		return nullptr;
	}

	if (NodeClass)
	{
		return CreateNode(ParentGraph, FromPin, NodeClass, Location, bSelectNewNode);
	}

	return nullptr;
}

UNarrativeGraphNode* FNarrativeGraphSchemaAction_NewNode::CreateNode(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const UClass* NodeClass, const FVector2D Location, const bool bSelectNewNode /*= true*/)
{
	check(NodeClass);

	const FScopedTransaction Transaction(LOCTEXT("AddNode", "Add Node"));

	ParentGraph->Modify();
	if (FromPin)
	{
		FromPin->Modify();
	}

	UNarrativeAsset* NarrativeAsset = CastChecked<UNarrativeGraph>(ParentGraph)->GetNarrativeAsset();
	NarrativeAsset->Modify();

	// create new Narrative Graph node
	const UClass* GraphNodeClass = UNarrativeGraphSchema::GetAssignedGraphNodeClass(NodeClass);
	UNarrativeGraphNode* NewGraphNode = NewObject<UNarrativeGraphNode>(ParentGraph, GraphNodeClass, NAME_None, RF_Transactional);

	// register to the graph
	NewGraphNode->CreateNewGuid();
	ParentGraph->AddNode(NewGraphNode, false, bSelectNewNode);

	// link editor and runtime nodes together
	UNarrativeNode* NarrativeNode = NarrativeAsset->CreateNode(NodeClass, NewGraphNode);
	NewGraphNode->SetNodeTemplate(NarrativeNode);

	// create pins and connections
	NewGraphNode->AllocateDefaultPins();
	NewGraphNode->AutowireNewNode(FromPin);

	// set position
	NewGraphNode->NodePosX = Location.X;
	NewGraphNode->NodePosY = Location.Y;

	// call notifies
	NewGraphNode->PostPlacedNewNode();
	ParentGraph->NotifyGraphChanged();

	NarrativeAsset->PostEditChange();

	// select in editor UI
	if (bSelectNewNode)
	{
		const TSharedPtr<SNarrativeGraphEditor> NarrativeGraphEditor = FNarrativeGraphUtils::GetNarrativeGraphEditor(ParentGraph);
		if (NarrativeGraphEditor.IsValid())
		{
			NarrativeGraphEditor->SelectSingleNode(NewGraphNode);
		}
	}

	return NewGraphNode;
}

UNarrativeGraphNode* FNarrativeGraphSchemaAction_NewNode::RecreateNode(UEdGraph* ParentGraph, UEdGraphNode* OldInstance, UNarrativeNode* NarrativeNode)
{
	check(NarrativeNode);

	ParentGraph->Modify();

	UNarrativeAsset* NarrativeAsset = CastChecked<UNarrativeGraph>(ParentGraph)->GetNarrativeAsset();
	NarrativeAsset->Modify();

	// create new Narrative Graph node
	const UClass* GraphNodeClass = UNarrativeGraphSchema::GetAssignedGraphNodeClass(NarrativeNode->GetClass());
	UNarrativeGraphNode* NewGraphNode = NewObject<UNarrativeGraphNode>(ParentGraph, GraphNodeClass, NAME_None, RF_Transactional);

	// register to the graph
	NewGraphNode->NodeGuid = NarrativeNode->GetGuid();
	ParentGraph->AddNode(NewGraphNode, false, false);

	// link editor and runtime nodes together
	NarrativeNode->SetGraphNode(NewGraphNode);
	NewGraphNode->SetNodeTemplate(NarrativeNode);

	// move links from the old node
	NewGraphNode->AllocateDefaultPins();
	if (OldInstance)
	{
		for (UEdGraphPin* OldPin : OldInstance->Pins)
		{
			if (OldPin->LinkedTo.Num() == 0)
			{
				continue;
			}

			for (UEdGraphPin* NewPin : NewGraphNode->Pins)
			{
				if (NewPin->Direction == OldPin->Direction && NewPin->PinName == OldPin->PinName)
				{
					TArray<UEdGraphPin*> Connections = OldPin->LinkedTo;
					for (UEdGraphPin* ConnectedPin : Connections)
					{
						ConnectedPin->BreakLinkTo(OldPin);
						ConnectedPin->MakeLinkTo(NewPin);
					}
				}
			}
		}
	}

	// keep old position
	NewGraphNode->NodePosX = OldInstance ? OldInstance->NodePosX : 0;
	NewGraphNode->NodePosY = OldInstance ? OldInstance->NodePosY : 0;

	// remove leftover
	if (OldInstance)
	{
		OldInstance->DestroyNode();
	}

	// call notifies
	NewGraphNode->PostPlacedNewNode();
	ParentGraph->NotifyGraphChanged();

	return NewGraphNode;
}

UNarrativeGraphNode* FNarrativeGraphSchemaAction_NewNode::ImportNode(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const UClass* NodeClass, const FGuid& NodeGuid, const FVector2D Location)
{
	check(NodeClass);

	ParentGraph->Modify();
	if (FromPin)
	{
		FromPin->Modify();
	}

	UNarrativeAsset* NarrativeAsset = CastChecked<UNarrativeGraph>(ParentGraph)->GetNarrativeAsset();
	NarrativeAsset->Modify();

	// create new Narrative Graph node
	const UClass* GraphNodeClass = UNarrativeGraphSchema::GetAssignedGraphNodeClass(NodeClass);
	UNarrativeGraphNode* NewGraphNode = NewObject<UNarrativeGraphNode>(ParentGraph, GraphNodeClass, NAME_None, RF_Transactional);

	// register to the graph
	NewGraphNode->NodeGuid = NodeGuid;
	ParentGraph->AddNode(NewGraphNode, false, false);

	// link editor and runtime nodes together
	UNarrativeNode* NarrativeNode = NarrativeAsset->CreateNode(NodeClass, NewGraphNode);
	NewGraphNode->SetNodeTemplate(NarrativeNode);

	// create pins and connections
	NewGraphNode->AllocateDefaultPins();
	NewGraphNode->AutowireNewNode(FromPin);

	// set position
	NewGraphNode->NodePosX = Location.X;
	NewGraphNode->NodePosY = Location.Y;

	// call notifies
	NewGraphNode->PostPlacedNewNode();
	ParentGraph->NotifyGraphChanged();

	return NewGraphNode;
}

UEdGraphNode* FNarrativeGraphSchemaAction_Paste::PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, const bool bSelectNewNode/* = true*/)
{
	// prevent adding new nodes while playing
	if (GEditor->PlayWorld == nullptr)
	{
		FNarrativeGraphUtils::GetNarrativeGraphEditor(ParentGraph)->PasteNodesHere(Location);
	}

	return nullptr;
}

/////////////////////////////////////////////////////
// Comment Node

UEdGraphNode* FNarrativeGraphSchemaAction_NewComment::PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, const bool bSelectNewNode/* = true*/)
{
	// prevent adding new nodes while playing
	if (GEditor->PlayWorld != nullptr)
	{
		return nullptr;
	}

	UEdGraphNode_Comment* CommentTemplate = NewObject<UEdGraphNode_Comment>();
	FVector2D SpawnLocation = Location;

	const TSharedPtr<SNarrativeGraphEditor> NarrativeGraphEditor = FNarrativeGraphUtils::GetNarrativeGraphEditor(ParentGraph);
	if (NarrativeGraphEditor.IsValid())
	{
		FSlateRect Bounds;
		if (NarrativeGraphEditor->GetBoundsForSelectedNodes(Bounds, 50.0f))
		{
			CommentTemplate->SetBounds(Bounds);
			SpawnLocation.X = CommentTemplate->NodePosX;
			SpawnLocation.Y = CommentTemplate->NodePosY;
		}
	}

	return FEdGraphSchemaAction_NewNode::SpawnNodeFromTemplate<UEdGraphNode_Comment>(ParentGraph, CommentTemplate, SpawnLocation);
}

#undef LOCTEXT_NAMESPACE
