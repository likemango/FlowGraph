// Copyright XiaoYao

#include "Graph/NarrativeGraphSchema.h"

#include "Graph/NarrativeGraph.h"
#include "Graph/NarrativeGraphEditor.h"
#include "Graph/NarrativeGraphEditorSettings.h"
#include "Graph/NarrativeGraphSchema_Actions.h"
#include "Graph/NarrativeGraphSettings.h"
#include "Graph/NarrativeGraphUtils.h"
#include "Graph/Nodes/NarrativeGraphNode.h"

#include "NarrativeAsset.h"
#include "Nodes/NarrativeNode.h"
#include "Nodes/NarrativeNodeBlueprint.h"
#include "Nodes/Route/NarrativeNode_CustomInput.h"
#include "Nodes/Route/NarrativeNode_Start.h"
#include "Nodes/Route/NarrativeNode_Reroute.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "EdGraph/EdGraph.h"
#include "Editor.h"
#include "ScopedTransaction.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativeGraphSchema)

#define LOCTEXT_NAMESPACE "NarrativeGraphSchema"

bool UNarrativeGraphSchema::bInitialGatherPerformed = false;
TArray<UClass*> UNarrativeGraphSchema::NativeNarrativeNodes;
TMap<FName, FAssetData> UNarrativeGraphSchema::BlueprintNarrativeNodes;
TMap<UClass*, UClass*> UNarrativeGraphSchema::GraphNodesByNarrativeNodes;

bool UNarrativeGraphSchema::bBlueprintCompilationPending;

FNarrativeGraphSchemaRefresh UNarrativeGraphSchema::OnNodeListChanged;

UNarrativeGraphSchema::UNarrativeGraphSchema(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UNarrativeGraphSchema::SubscribeToAssetChanges()
{
	const FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
	AssetRegistry.Get().OnFilesLoaded().AddStatic(&UNarrativeGraphSchema::GatherNodes);
	AssetRegistry.Get().OnAssetAdded().AddStatic(&UNarrativeGraphSchema::OnAssetAdded);
	AssetRegistry.Get().OnAssetRemoved().AddStatic(&UNarrativeGraphSchema::OnAssetRemoved);

	FCoreUObjectDelegates::ReloadCompleteDelegate.AddStatic(&UNarrativeGraphSchema::OnHotReload);

	if (GEditor)
	{
		GEditor->OnBlueprintPreCompile().AddStatic(&UNarrativeGraphSchema::OnBlueprintPreCompile);
		GEditor->OnBlueprintCompiled().AddStatic(&UNarrativeGraphSchema::OnBlueprintCompiled);
	}
}

void UNarrativeGraphSchema::GetPaletteActions(FGraphActionMenuBuilder& ActionMenuBuilder, const UNarrativeAsset* EditedNarrativeAsset, const FString& CategoryName)
{
	GetNarrativeNodeActions(ActionMenuBuilder, EditedNarrativeAsset, CategoryName);
	GetCommentAction(ActionMenuBuilder);
}

void UNarrativeGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	GetNarrativeNodeActions(ContextMenuBuilder, GetAssetClassDefaults(ContextMenuBuilder.CurrentGraph), FString());
	GetCommentAction(ContextMenuBuilder, ContextMenuBuilder.CurrentGraph);

	if (!ContextMenuBuilder.FromPin && FNarrativeGraphUtils::GetNarrativeGraphEditor(ContextMenuBuilder.CurrentGraph)->CanPasteNodes())
	{
		const TSharedPtr<FNarrativeGraphSchemaAction_Paste> NewAction(new FNarrativeGraphSchemaAction_Paste(FText::GetEmpty(), LOCTEXT("PasteHereAction", "Paste here"), FText::GetEmpty(), 0));
		ContextMenuBuilder.AddAction(NewAction);
	}
}

void UNarrativeGraphSchema::CreateDefaultNodesForGraph(UEdGraph& Graph) const
{
	const UNarrativeAsset* AssetClassDefaults = GetAssetClassDefaults(&Graph);
	static const FVector2D NodeOffsetIncrement = FVector2D(0, 128);
	FVector2D NodeOffset = FVector2D::ZeroVector;

	// Start node
	CreateDefaultNode(Graph, AssetClassDefaults, UNarrativeNode_Start::StaticClass(), NodeOffset, AssetClassDefaults->bStartNodePlacedAsGhostNode);

	// Add default nodes for all of the CustomInputs
	if (IsValid(AssetClassDefaults))
	{
		for (const FName& CustomInputName : AssetClassDefaults->CustomInputs)
		{
			NodeOffset += NodeOffsetIncrement;
			const UNarrativeGraphNode* NewNarrativeGraphNode = CreateDefaultNode(Graph, AssetClassDefaults, UNarrativeNode_CustomInput::StaticClass(), NodeOffset, true);

			UNarrativeNode_CustomInput* CustomInputNode = CastChecked<UNarrativeNode_CustomInput>(NewNarrativeGraphNode->GetNarrativeNode());
			CustomInputNode->SetEventName(CustomInputName);
		}
	}

	CastChecked<UNarrativeGraph>(&Graph)->GetNarrativeAsset()->HarvestNodeConnections();
}

UNarrativeGraphNode* UNarrativeGraphSchema::CreateDefaultNode(UEdGraph& Graph, const UNarrativeAsset* AssetClassDefaults, const TSubclassOf<UNarrativeNode>& NodeClass, const FVector2D& Offset, const bool bPlacedAsGhostNode)
{
	UNarrativeGraphNode* NewGraphNode = FNarrativeGraphSchemaAction_NewNode::CreateNode(&Graph, nullptr, NodeClass, Offset);
	SetNodeMetaData(NewGraphNode, FNodeMetadata::DefaultGraphNode);

	if (bPlacedAsGhostNode)
	{
		NewGraphNode->MakeAutomaticallyPlacedGhostNode();
	}

	return NewGraphNode;
}

const FPinConnectionResponse UNarrativeGraphSchema::CanCreateConnection(const UEdGraphPin* PinA, const UEdGraphPin* PinB) const
{
	const UNarrativeGraphNode* OwningNodeA = Cast<UNarrativeGraphNode>(PinA->GetOwningNodeUnchecked());
	const UNarrativeGraphNode* OwningNodeB = Cast<UNarrativeGraphNode>(PinB->GetOwningNodeUnchecked());

	if (!OwningNodeA || !OwningNodeB)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Invalid nodes"));
	}

	// Make sure the pins are not on the same node
	if (PinA->GetOwningNode() == PinB->GetOwningNode())
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Both are on the same node"));
	}

	if (PinA->bOrphanedPin || PinB->bOrphanedPin)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Cannot make new connections to orphaned pin"));
	}

	// Compare the directions
	const UEdGraphPin* InputPin = nullptr;
	const UEdGraphPin* OutputPin = nullptr;

	if (!CategorizePinsByDirection(PinA, PinB, /*out*/ InputPin, /*out*/ OutputPin))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Directions are not compatible"));
	}

	// Break existing connections on outputs only - multiple input connections are acceptable
	if (OutputPin->LinkedTo.Num() > 0)
	{
		const ECanCreateConnectionResponse ReplyBreakInputs = (OutputPin == PinA ? CONNECT_RESPONSE_BREAK_OTHERS_A : CONNECT_RESPONSE_BREAK_OTHERS_B);
		return FPinConnectionResponse(ReplyBreakInputs, TEXT("Replace existing connections"));
	}

	return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, TEXT(""));
}

bool UNarrativeGraphSchema::TryCreateConnection(UEdGraphPin* PinA, UEdGraphPin* PinB) const
{
	const bool bModified = UEdGraphSchema::TryCreateConnection(PinA, PinB);

	if (bModified)
	{
		PinA->GetOwningNode()->GetGraph()->NotifyGraphChanged();
	}

	return bModified;
}

bool UNarrativeGraphSchema::ShouldHidePinDefaultValue(UEdGraphPin* Pin) const
{
	return true;
}

FLinearColor UNarrativeGraphSchema::GetPinTypeColor(const FEdGraphPinType& PinType) const
{
	return FLinearColor::White;
}

FText UNarrativeGraphSchema::GetPinDisplayName(const UEdGraphPin* Pin) const
{
	FText ResultPinName;
	check(Pin != nullptr);
	if (Pin->PinFriendlyName.IsEmpty())
	{
		// We don't want to display "None" for no name
		if (Pin->PinName.IsNone())
		{
			return FText::GetEmpty();
		}
		if (GetDefault<UNarrativeGraphEditorSettings>()->bEnforceFriendlyPinNames) // this option is only difference between this override and UEdGraphSchema::GetPinDisplayName
		{
			ResultPinName = FText::FromString(FName::NameToDisplayString(Pin->PinName.ToString(), true));
		}
		else
		{
			ResultPinName = FText::FromName(Pin->PinName);
		}
	}
	else
	{
		ResultPinName = Pin->PinFriendlyName;

		bool bShouldUseLocalizedNodeAndPinNames = false;
		GConfig->GetBool(TEXT("Internationalization"), TEXT("ShouldUseLocalizedNodeAndPinNames"), bShouldUseLocalizedNodeAndPinNames, GEditorSettingsIni);
		if (!bShouldUseLocalizedNodeAndPinNames)
		{
			ResultPinName = FText::FromString(ResultPinName.BuildSourceString());
		}
	}
	return ResultPinName;
}

void UNarrativeGraphSchema::BreakNodeLinks(UEdGraphNode& TargetNode) const
{
	Super::BreakNodeLinks(TargetNode);

	TargetNode.GetGraph()->NotifyGraphChanged();
}

void UNarrativeGraphSchema::BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotification) const
{
	const FScopedTransaction Transaction(LOCTEXT("GraphEd_BreakPinLinks", "Break Pin Links"));

	Super::BreakPinLinks(TargetPin, bSendsNodeNotification);

	if (TargetPin.bOrphanedPin)
	{
		// this calls NotifyGraphChanged()
		Cast<UNarrativeGraphNode>(TargetPin.GetOwningNode())->RemoveOrphanedPin(&TargetPin);
	}
	else if (bSendsNodeNotification)
	{
		TargetPin.GetOwningNode()->GetGraph()->NotifyGraphChanged();
	}
}

int32 UNarrativeGraphSchema::GetNodeSelectionCount(const UEdGraph* Graph) const
{
	return FNarrativeGraphUtils::GetNarrativeGraphEditor(Graph)->GetNumberOfSelectedNodes();
}

TSharedPtr<FEdGraphSchemaAction> UNarrativeGraphSchema::GetCreateCommentAction() const
{
	return TSharedPtr<FEdGraphSchemaAction>(static_cast<FEdGraphSchemaAction*>(new FNarrativeGraphSchemaAction_NewComment));
}

void UNarrativeGraphSchema::OnPinConnectionDoubleCicked(UEdGraphPin* PinA, UEdGraphPin* PinB, const FVector2D& GraphPosition) const
{
	const FScopedTransaction Transaction(LOCTEXT("CreateNarrativeRerouteNodeOnWire", "Create Narrative Reroute Node"));

	const FVector2D NodeSpacerSize(42.0f, 24.0f);
	const FVector2D KnotTopLeft = GraphPosition - (NodeSpacerSize * 0.5f);

	UEdGraph* ParentGraph = PinA->GetOwningNode()->GetGraph();
	UNarrativeGraphNode* NewReroute = FNarrativeGraphSchemaAction_NewNode::CreateNode(ParentGraph, nullptr, UNarrativeNode_Reroute::StaticClass(), KnotTopLeft, false);

	PinA->BreakLinkTo(PinB);
	PinA->MakeLinkTo((PinA->Direction == EGPD_Output) ? NewReroute->InputPins[0] : NewReroute->OutputPins[0]);
	PinB->MakeLinkTo((PinB->Direction == EGPD_Output) ? NewReroute->InputPins[0] : NewReroute->OutputPins[0]);
}

TArray<TSharedPtr<FString>> UNarrativeGraphSchema::GetNarrativeNodeCategories()
{
	if (!bInitialGatherPerformed)
	{
		GatherNodes();
	}

	TSet<FString> UnsortedCategories;
	for (const UClass* NarrativeNodeClass : NativeNarrativeNodes)
	{
		if (const UNarrativeNode* DefaultObject = NarrativeNodeClass->GetDefaultObject<UNarrativeNode>())
		{
			UnsortedCategories.Emplace(DefaultObject->GetNodeCategory());
		}
	}

	for (const TPair<FName, FAssetData>& AssetData : BlueprintNarrativeNodes)
	{
		if (const UBlueprint* Blueprint = GetPlaceableNodeBlueprint(AssetData.Value))
		{
			UnsortedCategories.Emplace(Blueprint->BlueprintCategory);
		}
	}

	TArray<FString> SortedCategories = UnsortedCategories.Array();
	SortedCategories.Sort();

	// create list of categories
	TArray<TSharedPtr<FString>> Result;
	for (const FString& Category : SortedCategories)
	{
		if (!Category.IsEmpty())
		{
			Result.Emplace(MakeShareable(new FString(Category)));
		}
	}

	return Result;
}

UClass* UNarrativeGraphSchema::GetAssignedGraphNodeClass(const UClass* NarrativeNodeClass)
{
	TArray<UClass*> FoundParentClasses;
	UClass* ReturnClass = nullptr;

	// Collect all possible parents and their corresponding GraphNodeClasses
	for (const TPair<UClass*, UClass*>& GraphNodeByNarrativeNode : GraphNodesByNarrativeNodes)
	{
		if (NarrativeNodeClass == GraphNodeByNarrativeNode.Key)
		{
			return GraphNodeByNarrativeNode.Value;
		}

		if (NarrativeNodeClass->IsChildOf(GraphNodeByNarrativeNode.Key))
		{
			FoundParentClasses.Add(GraphNodeByNarrativeNode.Key);
		}
	}

	// Of only one parent found set the return to its GraphNodeClass
	if (FoundParentClasses.Num() == 1)
	{
		ReturnClass = GraphNodesByNarrativeNodes.FindRef(FoundParentClasses[0]);
	}
	// If multiple parents found, find the closest one and set the return to its GraphNodeClass
	else if (FoundParentClasses.Num() > 1)
	{
		TPair<int32, UClass*> ClosestParentMatch = {1000, nullptr};
		for (const auto& ParentClass : FoundParentClasses)
		{
			int32 StepsTillExactMatch = 0;
			const UClass* LocalParentClass = NarrativeNodeClass;

			while (IsValid(LocalParentClass) && LocalParentClass != ParentClass && LocalParentClass != UNarrativeNode::StaticClass())
			{
				StepsTillExactMatch++;
				LocalParentClass = LocalParentClass->GetSuperClass();
			}

			if (StepsTillExactMatch != 0 && StepsTillExactMatch < ClosestParentMatch.Key)
			{
				ClosestParentMatch = {StepsTillExactMatch, ParentClass};
			}
		}

		ReturnClass = GraphNodesByNarrativeNodes.FindRef(ClosestParentMatch.Value);
	}

	return IsValid(ReturnClass) ? ReturnClass : UNarrativeGraphNode::StaticClass();
}

void UNarrativeGraphSchema::ApplyNodeFilter(const UNarrativeAsset* EditedNarrativeAsset, const UClass* NarrativeNodeClass, TArray<UNarrativeNode*>& FilteredNodes)
{
	if (NarrativeNodeClass == nullptr)
	{
		return;
	}

	if (EditedNarrativeAsset == nullptr)
	{
		return;
	}

	if (!EditedNarrativeAsset->IsNodeClassAllowed(NarrativeNodeClass))
	{
		return;
	}
	
	UNarrativeNode* NodeDefaults = NarrativeNodeClass->GetDefaultObject<UNarrativeNode>();
	FilteredNodes.Emplace(NodeDefaults);
}

void UNarrativeGraphSchema::GetNarrativeNodeActions(FGraphActionMenuBuilder& ActionMenuBuilder, const UNarrativeAsset* EditedNarrativeAsset, const FString& CategoryName)
{
	if (!bInitialGatherPerformed)
	{
		GatherNodes();
	}

	// Narrative Asset type might limit which nodes are placeable 
	TArray<UNarrativeNode*> FilteredNodes;
	{
		FilteredNodes.Reserve(NativeNarrativeNodes.Num() + BlueprintNarrativeNodes.Num());

		for (const UClass* NarrativeNodeClass : NativeNarrativeNodes)
		{
			ApplyNodeFilter(EditedNarrativeAsset, NarrativeNodeClass, FilteredNodes);
		}

		for (const TPair<FName, FAssetData>& AssetData : BlueprintNarrativeNodes)
		{
			if (const UBlueprint* Blueprint = GetPlaceableNodeBlueprint(AssetData.Value))
			{
				ApplyNodeFilter(EditedNarrativeAsset, Blueprint->GeneratedClass, FilteredNodes);
			}
		}

		FilteredNodes.Shrink();
	}

	for (const UNarrativeNode* NarrativeNode : FilteredNodes)
	{
		if ((CategoryName.IsEmpty() || CategoryName.Equals(NarrativeNode->GetNodeCategory())) && !UNarrativeGraphSettings::Get()->NodesHiddenFromPalette.Contains(NarrativeNode->GetClass()))
		{
			TSharedPtr<FNarrativeGraphSchemaAction_NewNode> NewNodeAction(new FNarrativeGraphSchemaAction_NewNode(NarrativeNode));
			ActionMenuBuilder.AddAction(NewNodeAction);
		}
	}
}

void UNarrativeGraphSchema::GetCommentAction(FGraphActionMenuBuilder& ActionMenuBuilder, const UEdGraph* CurrentGraph /*= nullptr*/)
{
	if (!ActionMenuBuilder.FromPin)
	{
		const bool bIsManyNodesSelected = CurrentGraph ? (FNarrativeGraphUtils::GetNarrativeGraphEditor(CurrentGraph)->GetNumberOfSelectedNodes() > 0) : false;
		const FText MenuDescription = bIsManyNodesSelected ? LOCTEXT("CreateCommentAction", "Create Comment from Selection") : LOCTEXT("AddCommentAction", "Add Comment...");
		const FText ToolTip = LOCTEXT("CreateCommentToolTip", "Creates a comment.");

		const TSharedPtr<FNarrativeGraphSchemaAction_NewComment> NewAction(new FNarrativeGraphSchemaAction_NewComment(FText::GetEmpty(), MenuDescription, ToolTip, 0));
		ActionMenuBuilder.AddAction(NewAction);
	}
}

bool UNarrativeGraphSchema::IsNarrativeNodePlaceable(const UClass* Class)
{
	if (Class->HasAnyClassFlags(CLASS_Abstract | CLASS_NotPlaceable | CLASS_Deprecated))
	{
		return false;
	}

	if (const UNarrativeNode* DefaultObject = Class->GetDefaultObject<UNarrativeNode>())
	{
		return !DefaultObject->bNodeDeprecated;
	}

	return true;
}

void UNarrativeGraphSchema::OnBlueprintPreCompile(UBlueprint* Blueprint)
{
	if (Blueprint && Blueprint->GeneratedClass && Blueprint->GeneratedClass->IsChildOf(UNarrativeNode::StaticClass()))
	{
		bBlueprintCompilationPending = true;
	}
}

void UNarrativeGraphSchema::OnBlueprintCompiled()
{
	if (bBlueprintCompilationPending)
	{
		GatherNodes();
	}

	bBlueprintCompilationPending = false;
}

void UNarrativeGraphSchema::OnHotReload(EReloadCompleteReason ReloadCompleteReason)
{
	GatherNodes();
}

void UNarrativeGraphSchema::GatherNativeNodes()
{
	const bool bHotReloadNativeNodes = UNarrativeGraphEditorSettings::Get()->bHotReloadNativeNodes;
	// collect C++ nodes once per editor session
	if (NativeNarrativeNodes.Num() > 0 && !bHotReloadNativeNodes)
	{
		return;
	}

	NativeNarrativeNodes.Reset();
	GraphNodesByNarrativeNodes.Reset();

	TArray<UClass*> NarrativeNodes;
	GetDerivedClasses(UNarrativeNode::StaticClass(), NarrativeNodes);
	for (UClass* Class : NarrativeNodes)
	{
		if (Class->ClassGeneratedBy == nullptr && IsNarrativeNodePlaceable(Class))
		{
			NativeNarrativeNodes.Emplace(Class);
		}
	}

	TArray<UClass*> GraphNodes;
	GetDerivedClasses(UNarrativeGraphNode::StaticClass(), GraphNodes);
	for (UClass* GraphNodeClass : GraphNodes)
	{
		const UNarrativeGraphNode* GraphNodeCDO = GraphNodeClass->GetDefaultObject<UNarrativeGraphNode>();
		for (UClass* AssignedClass : GraphNodeCDO->AssignedNodeClasses)
		{
			if (AssignedClass->IsChildOf(UNarrativeNode::StaticClass()))
			{
				GraphNodesByNarrativeNodes.Emplace(AssignedClass, GraphNodeClass);
			}
		}
	}
}

void UNarrativeGraphSchema::GatherNodes()
{
	// prevent asset crunching during PIE
	if (GEditor && GEditor->PlayWorld)
	{
		return;
	}

	bInitialGatherPerformed = true;

	GatherNativeNodes();

	// retrieve all blueprint nodes
	FARFilter Filter;
	Filter.ClassPaths.Add(UNarrativeNodeBlueprint::StaticClass()->GetClassPathName());
	Filter.bRecursiveClasses = true;

	TArray<FAssetData> FoundAssets;
	const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
	AssetRegistryModule.Get().GetAssets(Filter, FoundAssets);
	for (const FAssetData& AssetData : FoundAssets)
	{
		AddAsset(AssetData, true);
	}

	OnNodeListChanged.Broadcast();
}

void UNarrativeGraphSchema::OnAssetAdded(const FAssetData& AssetData)
{
	AddAsset(AssetData, false);
}

void UNarrativeGraphSchema::AddAsset(const FAssetData& AssetData, const bool bBatch)
{
	if (!BlueprintNarrativeNodes.Contains(AssetData.PackageName))
	{
		const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
		if (AssetRegistryModule.Get().IsLoadingAssets())
		{
			return;
		}

		if (AssetData.GetClass()->IsChildOf(UNarrativeNodeBlueprint::StaticClass()))
		{
			BlueprintNarrativeNodes.Emplace(AssetData.PackageName, AssetData);

			if (!bBatch)
			{
				OnNodeListChanged.Broadcast();
			}
		}
	}
}

void UNarrativeGraphSchema::OnAssetRemoved(const FAssetData& AssetData)
{
	if (BlueprintNarrativeNodes.Contains(AssetData.PackageName))
	{
		BlueprintNarrativeNodes.Remove(AssetData.PackageName);
		BlueprintNarrativeNodes.Shrink();

		OnNodeListChanged.Broadcast();
	}
}

UBlueprint* UNarrativeGraphSchema::GetPlaceableNodeBlueprint(const FAssetData& AssetData)
{
	UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.GetAsset());
	if (Blueprint && IsNarrativeNodePlaceable(Blueprint->GeneratedClass))
	{
		return Blueprint;
	}

	return nullptr;
}

const UNarrativeAsset* UNarrativeGraphSchema::GetAssetClassDefaults(const UEdGraph* Graph)
{
	const UClass* AssetClass = UNarrativeAsset::StaticClass();

	if (Graph)
	{
		if (const UNarrativeAsset* NarrativeAsset = Graph->GetTypedOuter<UNarrativeAsset>())
		{
			AssetClass = NarrativeAsset->GetClass();
		}
	}

	return AssetClass->GetDefaultObject<UNarrativeAsset>();
}

#undef LOCTEXT_NAMESPACE
