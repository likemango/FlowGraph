// Copyright XiaoYao

#pragma once

#include "EdGraph/EdGraphSchema.h"

#include "Nodes/NarrativeGraphNode.h"
#include "Nodes/NarrativeNode.h"
#include "NarrativeGraphSchema_Actions.generated.h"

/** Action to add a node to the graph */
USTRUCT()
struct NARRATIVEEDITOR_API FNarrativeGraphSchemaAction_NewNode : public FEdGraphSchemaAction
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	class UClass* NodeClass;

	static FName StaticGetTypeId()
	{
		static FName Type("FNarrativeGraphSchemaAction_NewNode");
		return Type;
	}

	virtual FName GetTypeId() const override { return StaticGetTypeId(); }

	FNarrativeGraphSchemaAction_NewNode()
		: FEdGraphSchemaAction()
		, NodeClass(nullptr)
	{
	}

	FNarrativeGraphSchemaAction_NewNode(UClass* Node)
		: FEdGraphSchemaAction()
		, NodeClass(Node)
	{
	}

	FNarrativeGraphSchemaAction_NewNode(const UNarrativeNode* Node)
		: FEdGraphSchemaAction(FText::FromString(Node->GetNodeCategory()), Node->GetNodeTitle(), Node->GetNodeToolTip(), 0, FText::FromString(Node->GetClass()->GetMetaData("Keywords")))
		, NodeClass(Node->GetClass())
	{
	}

	// FEdGraphSchemaAction
	virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
	// --

	static UNarrativeGraphNode* CreateNode(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const UClass* NodeClass, const FVector2D Location, const bool bSelectNewNode = true);
	static UNarrativeGraphNode* RecreateNode(UEdGraph* ParentGraph, UEdGraphNode* OldInstance, UNarrativeNode* NarrativeNode);
	static UNarrativeGraphNode* ImportNode(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const UClass* NodeClass, const FGuid& NodeGuid, const FVector2D Location);
};

/** Action to paste clipboard contents into the graph */
USTRUCT()
struct NARRATIVEEDITOR_API FNarrativeGraphSchemaAction_Paste : public FEdGraphSchemaAction
{
	GENERATED_USTRUCT_BODY()

	FNarrativeGraphSchemaAction_Paste()
		: FEdGraphSchemaAction()
	{
	}

	FNarrativeGraphSchemaAction_Paste(FText InNodeCategory, FText InMenuDesc, FText InToolTip, const int32 InGrouping)
		: FEdGraphSchemaAction(MoveTemp(InNodeCategory), MoveTemp(InMenuDesc), MoveTemp(InToolTip), InGrouping)
	{
	}

	// FEdGraphSchemaAction
	virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
	// --
};

/** Action to create new comment */
USTRUCT()
struct NARRATIVEEDITOR_API FNarrativeGraphSchemaAction_NewComment : public FEdGraphSchemaAction
{
	GENERATED_USTRUCT_BODY()

	// Simple type info
	static FName StaticGetTypeId()
	{
		static FName Type("FNarrativeGraphSchemaAction_NewComment");
		return Type;
	}

	virtual FName GetTypeId() const override { return StaticGetTypeId(); }

	FNarrativeGraphSchemaAction_NewComment()
		: FEdGraphSchemaAction()
	{
	}

	FNarrativeGraphSchemaAction_NewComment(FText InNodeCategory, FText InMenuDesc, FText InToolTip, const int32 InGrouping)
		: FEdGraphSchemaAction(MoveTemp(InNodeCategory), MoveTemp(InMenuDesc), MoveTemp(InToolTip), InGrouping)
	{
	}

	// FEdGraphSchemaAction
	virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
	// --
};
