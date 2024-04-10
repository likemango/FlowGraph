// Copyright XiaoYao

#include "NarrativeAsset.h"

#include "NarrativeSettings.h"
#include "NarrativeSubsystem.h"

#include "Nodes/NarrativeNode.h"
#include "Nodes/Route/NarrativeNode_CustomInput.h"
#include "Nodes/Route/NarrativeNode_CustomOutput.h"
#include "Nodes/Route/NarrativeNode_Start.h"
#include "Nodes/Route/NarrativeNode_SubGraph.h"

#include "Engine/World.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

#if WITH_EDITOR
#include "NarrativeMessageLog.h"
#include "NarrativeLogChannels.h"

#include "Editor.h"
#include "Editor/EditorEngine.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativeAsset)

#if WITH_EDITOR
FString UNarrativeAsset::ValidationError_NodeClassNotAllowed = TEXT("Node class {0} is not allowed in this asset.");
FString UNarrativeAsset::ValidationError_NullNodeInstance = TEXT("Node with GUID {0} is NULL");
#endif

UNarrativeAsset::UNarrativeAsset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bWorldBound(true)
#if WITH_EDITOR
	, NarrativeGraph(nullptr)
#endif
	, AllowedNodeClasses({UNarrativeNode::StaticClass()})
	, AllowedInSubgraphNodeClasses({UNarrativeNode_SubGraph::StaticClass()})
	, bStartNodePlacedAsGhostNode(false)
	, TemplateAsset(nullptr)
	, FinishPolicy(ENarrativeFinishPolicy::Keep)
{
	if (!AssetGuid.IsValid())
	{
		AssetGuid = FGuid::NewGuid();
	}

	ExpectedOwnerClass = UNarrativeSettings::Get()->GetDefaultExpectedOwnerClass();
}

#if WITH_EDITOR
void UNarrativeAsset::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	UNarrativeAsset* This = CastChecked<UNarrativeAsset>(InThis);
	Collector.AddReferencedObject(This->NarrativeGraph, This);

	Super::AddReferencedObjects(InThis, Collector);
}

void UNarrativeAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property && (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UNarrativeAsset, CustomInputs)
		|| PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UNarrativeAsset, CustomOutputs)))
	{
		OnSubGraphReconstructionRequested.ExecuteIfBound();
	}
}

void UNarrativeAsset::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);

	if (!bDuplicateForPIE)
	{
		AssetGuid = FGuid::NewGuid();
		Nodes.Empty();
	}
}

void UNarrativeAsset::PostLoad()
{
	Super::PostLoad();

	// If we removed or moved a flow node blueprint (and there is no redirector) we might loose the reference to it resulting
	// in null pointers in the Nodes FGUID->UNarrativeNode* Map. So here we iterate over all the Nodes and remove all pairs that
	// are nulled out.
	
	TSet<FGuid> NodesToRemoveGUID;

	for (auto& [Guid, Node] : GetNodes())
	{
		if (!IsValid(Node))
		{
			NodesToRemoveGUID.Emplace(Guid);
		}
	}

	for (const FGuid& Guid : NodesToRemoveGUID)
	{
		UnregisterNode(Guid);
	}
}

EDataValidationResult UNarrativeAsset::ValidateAsset(FNarrativeMessageLog& MessageLog)
{
	// validate nodes
	for (const TPair<FGuid, UNarrativeNode*>& Node : Nodes)
	{
		if (IsValid(Node.Value))
		{
			FText FailureReason;
			if (!IsNodeClassAllowed(Node.Value->GetClass(), &FailureReason))
			{
				const FString ErrorMsg = 
					FailureReason.IsEmpty() ?
						FString::Format(*ValidationError_NodeClassNotAllowed, {*Node.Value->GetClass()->GetName()}) :
						FailureReason.ToString();

				MessageLog.Error(*ErrorMsg, Node.Value);
			}
			
			Node.Value->ValidationLog.Messages.Empty();
			if (Node.Value->ValidateNode() == EDataValidationResult::Invalid)
			{
				MessageLog.Messages.Append(Node.Value->ValidationLog.Messages);
			}
		}
		else
		{
			const FString ErrorMsg = FString::Format(*ValidationError_NullNodeInstance, {*Node.Key.ToString()});
			MessageLog.Error(*ErrorMsg, this);
		}
	}

	return MessageLog.Messages.Num() > 0 ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}

bool UNarrativeAsset::IsNodeClassAllowed(const UClass* NarrativeNodeClass, FText* OutOptionalFailureReason) const
{
	if (!IsValid(NarrativeNodeClass))
	{
		return false;
	}

	if (!CanNarrativeNodeClassBeUsedByNarrativeAsset(*NarrativeNodeClass))
	{
		return false;
	}

	if (!CanNarrativeAssetUseNarrativeNodeClass(*NarrativeNodeClass))
	{
		return false;
	}

	// Confirm plugin reference restrictions are being respected
	if (!CanNarrativeAssetReferenceNarrativeNode(*NarrativeNodeClass, OutOptionalFailureReason))
	{
		return false;
	}

	return true;
}

bool UNarrativeAsset::CanNarrativeNodeClassBeUsedByNarrativeAsset(const UClass& NarrativeNodeClass) const
{
	UNarrativeNode* NodeDefaults = NarrativeNodeClass.GetDefaultObject<UNarrativeNode>();

	// UNarrativeNode class limits which UNarrativeAsset class can use it
	for (const UClass* DeniedAssetClass : NodeDefaults->DeniedAssetClasses)
	{
		if (DeniedAssetClass && GetClass()->IsChildOf(DeniedAssetClass))
		{
			return false;
		}
	}

	if (NodeDefaults->AllowedAssetClasses.Num() > 0)
	{
		bool bAllowedInAsset = false;
		for (const UClass* AllowedAssetClass : NodeDefaults->AllowedAssetClasses)
		{
			if (AllowedAssetClass && GetClass()->IsChildOf(AllowedAssetClass))
			{
				bAllowedInAsset = true;
				break;
			}
		}
		if (!bAllowedInAsset)
		{
			return false;
		}
	}

	return true;
}

bool UNarrativeAsset::CanNarrativeAssetUseNarrativeNodeClass(const UClass& NarrativeNodeClass) const
{
	// UNarrativeAsset class can limit which UNarrativeNode classes can be used
	for (const UClass* DeniedNodeClass : DeniedNodeClasses)
	{
		if (DeniedNodeClass && NarrativeNodeClass.IsChildOf(DeniedNodeClass))
		{
			return false;
		}
	}

	if (AllowedNodeClasses.Num() > 0)
	{
		bool bAllowedInAsset = false;
		for (const UClass* AllowedNodeClass : AllowedNodeClasses)
		{
			if (AllowedNodeClass && NarrativeNodeClass.IsChildOf(AllowedNodeClass))
			{
				bAllowedInAsset = true;
				break;
			}
		}
		if (!bAllowedInAsset)
		{
			return false;
		}
	}

	return true;
}

bool UNarrativeAsset::CanNarrativeAssetReferenceNarrativeNode(const UClass& NarrativeNodeClass, FText* OutOptionalFailureReason) const
{
	if (!GEditor || !IsValid(&NarrativeNodeClass))
	{
		return false;
	}

	FAssetData NarrativeNodeAssetData(&NarrativeNodeClass);

	FAssetReferenceFilterContext AssetReferenceFilterContext;
	AssetReferenceFilterContext.ReferencingAssets.Add(FAssetData(this));

	// Confirm plugin reference restrictions are being respected
	TSharedPtr<IAssetReferenceFilter> NarrativeAssetReferenceFilter = GEditor->MakeAssetReferenceFilter(AssetReferenceFilterContext);
	if (NarrativeAssetReferenceFilter.IsValid() &&
		!NarrativeAssetReferenceFilter->PassesFilter(NarrativeNodeAssetData, OutOptionalFailureReason))
	{
		return false;
	}

	return true;
}

TSharedPtr<INarrativeGraphInterface> UNarrativeAsset::NarrativeGraphInterface = nullptr;

void UNarrativeAsset::SetNarrativeGraphInterface(TSharedPtr<INarrativeGraphInterface> InNarrativeAssetEditor)
{
	check(!NarrativeGraphInterface.IsValid());
	NarrativeGraphInterface = InNarrativeAssetEditor;
}

UNarrativeNode* UNarrativeAsset::CreateNode(const UClass* NodeClass, UEdGraphNode* GraphNode)
{
	UNarrativeNode* NewNode = NewObject<UNarrativeNode>(this, NodeClass, NAME_None, RF_Transactional);
	NewNode->SetGraphNode(GraphNode);

	RegisterNode(GraphNode->NodeGuid, NewNode);
	return NewNode;
}

void UNarrativeAsset::RegisterNode(const FGuid& NewGuid, UNarrativeNode* NewNode)
{
	NewNode->SetGuid(NewGuid);
	Nodes.Emplace(NewGuid, NewNode);

	HarvestNodeConnections();
}

void UNarrativeAsset::UnregisterNode(const FGuid& NodeGuid)
{
	Nodes.Remove(NodeGuid);
	Nodes.Compact();

	HarvestNodeConnections();
	MarkPackageDirty();
}

void UNarrativeAsset::HarvestNodeConnections()
{
	TMap<FName, FConnectedPin> Connections;
	bool bGraphDirty = false;

	// last moment to remove invalid nodes
	for (auto NodeIt = Nodes.CreateIterator(); NodeIt; ++NodeIt)
	{
		const TPair<FGuid, UNarrativeNode*>& Pair = *NodeIt;
		if (Pair.Value == nullptr)
		{
			NodeIt.RemoveCurrent();
			bGraphDirty = true;
		}
	}

	for (const TPair<FGuid, UNarrativeNode*>& Pair : Nodes)
	{
		UNarrativeNode* Node = Pair.Value;
		TMap<FName, FConnectedPin> FoundConnections;

		for (const UEdGraphPin* ThisPin : Node->GetGraphNode()->Pins)
		{
			if (ThisPin->Direction == EGPD_Output && ThisPin->LinkedTo.Num() > 0)
			{
				if (const UEdGraphPin* LinkedPin = ThisPin->LinkedTo[0])
				{
					const UEdGraphNode* LinkedNode = LinkedPin->GetOwningNode();
					FoundConnections.Add(ThisPin->PinName, FConnectedPin(LinkedNode->NodeGuid, LinkedPin->PinName));
				}
			}
		}

		// This check exists to ensure that we don't mark graph dirty, if none of connections changed
		// Optimization: we need check it only until the first node would be marked dirty, as this already marks Narrative Asset package dirty
		if (bGraphDirty == false)
		{
			if (FoundConnections.Num() != Node->Connections.Num())
			{
				bGraphDirty = true;
			}
			else
			{
				for (const TPair<FName, FConnectedPin>& FoundConnection : FoundConnections)
				{
					if (const FConnectedPin* OldConnection = Node->Connections.Find(FoundConnection.Key))
					{
						if (FoundConnection.Value != *OldConnection)
						{
							bGraphDirty = true;
							break;
						}
					}
					else
					{
						bGraphDirty = true;
						break;
					}
				}
			}
		}

		if (bGraphDirty)
		{
			Node->SetFlags(RF_Transactional);
			Node->Modify();

			Node->SetConnections(FoundConnections);
			Node->PostEditChange();
		}
	}
}
#endif

UNarrativeNode* UNarrativeAsset::GetDefaultEntryNode() const
{
	UNarrativeNode* FirstStartNode = nullptr;

	for (const TPair<FGuid, UNarrativeNode*>& Node : Nodes)
	{
		if (UNarrativeNode_Start* StartNode = Cast<UNarrativeNode_Start>(Node.Value))
		{
			if (StartNode->GetConnectedNodes().Num() > 0)
			{
				return StartNode;
			}
			else if (FirstStartNode == nullptr)
			{
				FirstStartNode = StartNode;
			}
		}
	}

	// If none of the found start nodes have connections, fallback to the first start node we found
	return FirstStartNode;
}

#if WITH_EDITOR
void UNarrativeAsset::AddCustomInput(const FName& EventName)
{
	if (!CustomInputs.Contains(EventName))
	{
		CustomInputs.Add(EventName);
	}
}

void UNarrativeAsset::RemoveCustomInput(const FName& EventName)
{
	if (CustomInputs.Contains(EventName))
	{
		CustomInputs.Remove(EventName);
	}
}

void UNarrativeAsset::AddCustomOutput(const FName& EventName)
{
	if (!CustomOutputs.Contains(EventName))
	{
		CustomOutputs.Add(EventName);
	}
}

void UNarrativeAsset::RemoveCustomOutput(const FName& EventName)
{
	if (CustomOutputs.Contains(EventName))
	{
		CustomOutputs.Remove(EventName);
	}
}
#endif // WITH_EDITOR

UNarrativeNode_CustomInput* UNarrativeAsset::TryFindCustomInputNodeByEventName(const FName& EventName) const
{
	for (UNarrativeNode_CustomInput* InputNode : CustomInputNodes)
	{
		if (IsValid(InputNode) && InputNode->GetEventName() == EventName)
		{
			return InputNode;
		}
	}

	return nullptr;
}

UNarrativeNode_CustomOutput* UNarrativeAsset::TryFindCustomOutputNodeByEventName(const FName& EventName) const
{
	for (const TPair<FGuid, UNarrativeNode*>& Node : Nodes)
	{
		if (UNarrativeNode_CustomOutput* CustomOutput = Cast<UNarrativeNode_CustomOutput>(Node.Value))
		{
			if (CustomOutput->GetEventName() == EventName)
			{
				return CustomOutput;
			}
		}
	}

	return nullptr;
}

TArray<FName> UNarrativeAsset::GatherCustomInputNodeEventNames() const
{
	// Runtime-safe gathering of the CustomInputs (which is editor-only data)
	//  from the actual flow nodes
	TArray<FName> Results;

	for (const TPair<FGuid, UNarrativeNode*>& Node : Nodes)
	{
		if (UNarrativeNode_CustomInput* CustomInput = Cast<UNarrativeNode_CustomInput>(Node.Value))
		{
			Results.Add(CustomInput->GetEventName());
		}
	}

	return Results;
}

TArray<FName> UNarrativeAsset::GatherCustomOutputNodeEventNames() const
{
	// Runtime-safe gathering of the CustomOutputs (which is editor-only data)
	//  from the actual flow nodes
	TArray<FName> Results;

	for (const TPair<FGuid, UNarrativeNode*>& Node : Nodes)
	{
		if (UNarrativeNode_CustomOutput* CustomOutput = Cast<UNarrativeNode_CustomOutput>(Node.Value))
		{
			Results.Add(CustomOutput->GetEventName());
		}
	}

	return Results;
}

TArray<UNarrativeNode*> UNarrativeAsset::GetNodesInExecutionOrder(UNarrativeNode* FirstIteratedNode, const TSubclassOf<UNarrativeNode> NarrativeNodeClass)
{
	TArray<UNarrativeNode*> FoundNodes;
	GetNodesInExecutionOrder<UNarrativeNode>(FirstIteratedNode, FoundNodes);

	// filter out nodes by class
	for (int32 i = FoundNodes.Num() - 1; i >= 0; i--)
	{
		if (!FoundNodes[i]->GetClass()->IsChildOf(NarrativeNodeClass))
		{
			FoundNodes.RemoveAt(i);
		}
	}
	FoundNodes.Shrink();
	
	return FoundNodes;
}

void UNarrativeAsset::AddInstance(UNarrativeAsset* Instance)
{
	ActiveInstances.Add(Instance);
}

int32 UNarrativeAsset::RemoveInstance(UNarrativeAsset* Instance)
{
#if WITH_EDITOR
	if (InspectedInstance.IsValid() && InspectedInstance.Get() == Instance)
	{
		SetInspectedInstance(NAME_None);
	}
#endif

	ActiveInstances.Remove(Instance);
	return ActiveInstances.Num();
}

void UNarrativeAsset::ClearInstances()
{
#if WITH_EDITOR
	if (InspectedInstance.IsValid())
	{
		SetInspectedInstance(NAME_None);
	}
#endif

	for (int32 i = ActiveInstances.Num() - 1; i >= 0; i--)
	{
		if (ActiveInstances.IsValidIndex(i) && ActiveInstances[i])
		{
			ActiveInstances[i]->FinishNarrative(ENarrativeFinishPolicy::Keep);
		}
	}

	ActiveInstances.Empty();
}

#if WITH_EDITOR
void UNarrativeAsset::GetInstanceDisplayNames(TArray<TSharedPtr<FName>>& OutDisplayNames) const
{
	for (const UNarrativeAsset* Instance : ActiveInstances)
	{
		OutDisplayNames.Emplace(MakeShareable(new FName(Instance->GetDisplayName())));
	}
}

void UNarrativeAsset::SetInspectedInstance(const FName& NewInspectedInstanceName)
{
	if (NewInspectedInstanceName.IsNone())
	{
		InspectedInstance = nullptr;
	}
	else
	{
		for (UNarrativeAsset* ActiveInstance : ActiveInstances)
		{
			if (ActiveInstance && ActiveInstance->GetDisplayName() == NewInspectedInstanceName)
			{
				if (!InspectedInstance.IsValid() || InspectedInstance != ActiveInstance)
				{
					InspectedInstance = ActiveInstance;
				}
				break;
			}
		}
	}

	BroadcastDebuggerRefresh();
}

void UNarrativeAsset::BroadcastDebuggerRefresh() const
{
	RefreshDebuggerEvent.Broadcast();
}

void UNarrativeAsset::BroadcastRuntimeMessageAdded(const TSharedRef<FTokenizedMessage>& Message)
{
	RuntimeMessageEvent.Broadcast(this, Message);
}
#endif // WITH_EDITOR

void UNarrativeAsset::InitializeInstance(const TWeakObjectPtr<UObject> InOwner, UNarrativeAsset* InTemplateAsset)
{
	Owner = InOwner;
	TemplateAsset = InTemplateAsset;

	for (TPair<FGuid, UNarrativeNode*>& Node : Nodes)
	{
		UNarrativeNode* NewNodeInstance = NewObject<UNarrativeNode>(this, Node.Value->GetClass(), NAME_None, RF_Transient, Node.Value, false, nullptr);
		Node.Value = NewNodeInstance;

		if (UNarrativeNode_CustomInput* CustomInput = Cast<UNarrativeNode_CustomInput>(NewNodeInstance))
		{
			if (!CustomInput->EventName.IsNone())
			{
				CustomInputNodes.Emplace(CustomInput);
			}
		}

		NewNodeInstance->InitializeInstance();
	}
}

void UNarrativeAsset::DeinitializeInstance()
{
	for (const TPair<FGuid, UNarrativeNode*>& Node : Nodes)
	{
		if (IsValid(Node.Value))
		{
			Node.Value->DeinitializeInstance();
		}
	}

	if (TemplateAsset)
	{
		const int32 ActiveInstancesLeft = TemplateAsset->RemoveInstance(this);
		if (ActiveInstancesLeft == 0 && GetNarrativeSubsystem())
		{
			GetNarrativeSubsystem()->RemoveInstancedTemplate(TemplateAsset);
		}
	}
}

void UNarrativeAsset::PreStartNarrative()
{
	ResetNodes();

#if WITH_EDITOR
	if (TemplateAsset->ActiveInstances.Num() == 1)
	{
		// this instance is the only active one, set it directly as Inspected Instance
		TemplateAsset->SetInspectedInstance(GetDisplayName());
	}
	else
	{
		// request to refresh list to show newly created instance
		TemplateAsset->BroadcastDebuggerRefresh();
	}
#endif
}

void UNarrativeAsset::StartNarrative()
{
	PreStartNarrative();

	if (UNarrativeNode* ConnectedEntryNode = GetDefaultEntryNode())
	{
		RecordedNodes.Add(ConnectedEntryNode);
		ConnectedEntryNode->TriggerFirstOutput(true);
	}
}

void UNarrativeAsset::FinishNarrative(const ENarrativeFinishPolicy InFinishPolicy, const bool bRemoveInstance /*= true*/)
{
	FinishPolicy = InFinishPolicy;

	// end execution of this asset and all of its nodes
	for (UNarrativeNode* Node : ActiveNodes)
	{
		Node->Deactivate();
	}
	ActiveNodes.Empty();

	// flush preloaded content
	for (UNarrativeNode* PreloadedNode : PreloadedNodes)
	{
		PreloadedNode->TriggerFlush();
	}
	PreloadedNodes.Empty();

	// provides option to finish game-specific logic prior to removing asset instance 
	if (bRemoveInstance)
	{
		DeinitializeInstance();
	}
}

bool UNarrativeAsset::HasStartedNarrative() const
{
	return RecordedNodes.Num() > 0;
}

AActor* UNarrativeAsset::TryFindActorOwner() const
{
	const UActorComponent* OwnerAsComponent = Cast<UActorComponent>(GetOwner());
	if (IsValid(OwnerAsComponent))
	{
		return Cast<AActor>(OwnerAsComponent->GetOwner());
	}

	return nullptr;
}

TWeakObjectPtr<UNarrativeAsset> UNarrativeAsset::GetNarrativeInstance(UNarrativeNode_SubGraph* SubGraphNode) const
{
	return ActiveSubGraphs.FindRef(SubGraphNode);
}

void UNarrativeAsset::TriggerCustomInput_FromSubGraph(UNarrativeNode_SubGraph* Node, const FName& EventName) const
{
	const TWeakObjectPtr<UNarrativeAsset> NarrativeInstance = ActiveSubGraphs.FindRef(Node);
	if (NarrativeInstance.IsValid())
	{
		NarrativeInstance->TriggerCustomInput(EventName);
	}
}

void UNarrativeAsset::TriggerCustomInput(const FName& EventName)
{
	for (UNarrativeNode_CustomInput* CustomInput : CustomInputNodes)
	{
		if (CustomInput->EventName == EventName)
		{
			RecordedNodes.Add(CustomInput);
			CustomInput->ExecuteInput(EventName);
		}
	}
}

void UNarrativeAsset::TriggerCustomOutput(const FName& EventName)
{
	if (NodeOwningThisAssetInstance.IsValid()) // it's a SubGraph
	{
		NodeOwningThisAssetInstance->TriggerOutput(EventName);
	}
	else // it's a Root Narrative, so the intention here might be to call event on the Narrative Component
	{
		if (UNarrativeComponent* NarrativeComponent = Cast<UNarrativeComponent>(GetOwner()))
		{
			NarrativeComponent->OnTriggerRootNarrativeOutputEventDispatcher(this, EventName);
		}
	}
}

void UNarrativeAsset::TriggerInput(const FGuid& NodeGuid, const FName& PinName)
{
	if (UNarrativeNode* Node = Nodes.FindRef(NodeGuid))
	{
		if (!ActiveNodes.Contains(Node))
		{
			ActiveNodes.Add(Node);
			RecordedNodes.Add(Node);
		}

		Node->TriggerInput(PinName);
	}
}

void UNarrativeAsset::FinishNode(UNarrativeNode* Node)
{
	if (ActiveNodes.Contains(Node))
	{
		ActiveNodes.Remove(Node);

		// if graph reached Finish and this asset instance was created by SubGraph node
		if (Node->CanFinishGraph())
		{
			if (NodeOwningThisAssetInstance.IsValid())
			{
				NodeOwningThisAssetInstance.Get()->TriggerFirstOutput(true);
			}
			else
			{
				FinishNarrative(ENarrativeFinishPolicy::Keep);
			}
		}
	}
}

void UNarrativeAsset::ResetNodes()
{
	for (UNarrativeNode* Node : RecordedNodes)
	{
		Node->ResetRecords();
	}

	RecordedNodes.Empty();
}

UNarrativeSubsystem* UNarrativeAsset::GetNarrativeSubsystem() const
{
	return Cast<UNarrativeSubsystem>(GetOuter());
}

FName UNarrativeAsset::GetDisplayName() const
{
	return GetFName();
}

UNarrativeNode_SubGraph* UNarrativeAsset::GetNodeOwningThisAssetInstance() const
{
	return NodeOwningThisAssetInstance.Get();
}

UNarrativeAsset* UNarrativeAsset::GetParentInstance() const
{
	return NodeOwningThisAssetInstance.IsValid() ? NodeOwningThisAssetInstance.Get()->GetNarrativeAsset() : nullptr;
}

FNarrativeAssetSaveData UNarrativeAsset::SaveInstance(TArray<FNarrativeAssetSaveData>& SavedNarrativeInstances)
{
	FNarrativeAssetSaveData AssetRecord;
	AssetRecord.WorldName = IsBoundToWorld() ? GetWorld()->GetName() : FString();
	AssetRecord.InstanceName = GetName();

	// opportunity to collect data before serializing asset
	OnSave();

	// iterate nodes
	TArray<UNarrativeNode*> NodesInExecutionOrder;
	GetNodesInExecutionOrder<UNarrativeNode>(GetDefaultEntryNode(), NodesInExecutionOrder);
	for (UNarrativeNode* Node : NodesInExecutionOrder)
	{
		if (Node && Node->ActivationState == ENarrativeNodeState::Active)
		{
			// iterate SubGraphs
			if (UNarrativeNode_SubGraph* SubGraphNode = Cast<UNarrativeNode_SubGraph>(Node))
			{
				const TWeakObjectPtr<UNarrativeAsset> SubNarrativeInstance = GetNarrativeInstance(SubGraphNode);
				if (SubNarrativeInstance.IsValid())
				{
					const FNarrativeAssetSaveData SubAssetRecord = SubNarrativeInstance->SaveInstance(SavedNarrativeInstances);
					SubGraphNode->SavedAssetInstanceName = SubAssetRecord.InstanceName;
				}
			}

			FNarrativeNodeSaveData NodeRecord;
			Node->SaveInstance(NodeRecord);

			AssetRecord.NodeRecords.Emplace(NodeRecord);
		}
	}

	// serialize asset
	FMemoryWriter MemoryWriter(AssetRecord.AssetData, true);
	FNarrativeArchive Ar(MemoryWriter);
	Serialize(Ar);

	// write archive to SaveGame
	SavedNarrativeInstances.Emplace(AssetRecord);

	return AssetRecord;
}

void UNarrativeAsset::LoadInstance(const FNarrativeAssetSaveData& AssetRecord)
{
	FMemoryReader MemoryReader(AssetRecord.AssetData, true);
	FNarrativeArchive Ar(MemoryReader);
	Serialize(Ar);

	PreStartNarrative();

	// iterate graph "from the end", backward to execution order
	// prevents issue when the preceding node would instantly fire output to a not-yet-loaded node
	for (int32 i = AssetRecord.NodeRecords.Num() - 1; i >= 0; i--)
	{
		if (UNarrativeNode* Node = Nodes.FindRef(AssetRecord.NodeRecords[i].NodeGuid))
		{
			Node->LoadInstance(AssetRecord.NodeRecords[i]);
		}
	}

	OnLoad();
}

void UNarrativeAsset::OnActivationStateLoaded(UNarrativeNode* Node)
{
	if (Node->ActivationState != ENarrativeNodeState::NeverActivated)
	{
		RecordedNodes.Emplace(Node);
	}

	if (Node->ActivationState == ENarrativeNodeState::Active)
	{
		ActiveNodes.Emplace(Node);
	}
}

void UNarrativeAsset::OnSave_Implementation()
{
}

void UNarrativeAsset::OnLoad_Implementation()
{
}

bool UNarrativeAsset::IsBoundToWorld_Implementation()
{
	return bWorldBound;
}

#if WITH_EDITOR
void UNarrativeAsset::LogError(const FString& MessageToLog, UNarrativeNode* Node)
{
	// this is runtime log which is should be only called on runtime instances of asset
	if (TemplateAsset == nullptr)
	{
		UE_LOG(LogNarrative, Log, TEXT("Attempted to use Runtime Log on template asset %s"), *MessageToLog);
	}

	if (RuntimeLog.Get())
	{
		const TSharedRef<FTokenizedMessage> TokenizedMessage = RuntimeLog.Get()->Error(*MessageToLog, Node);
		BroadcastRuntimeMessageAdded(TokenizedMessage);
	}
}

void UNarrativeAsset::LogWarning(const FString& MessageToLog, UNarrativeNode* Node)
{
	// this is runtime log which is should be only called on runtime instances of asset
	if (TemplateAsset == nullptr)
	{
		UE_LOG(LogNarrative, Log, TEXT("Attempted to use Runtime Log on template asset %s"), *MessageToLog);
	}

	if (RuntimeLog.Get())
	{
		const TSharedRef<FTokenizedMessage> TokenizedMessage = RuntimeLog.Get()->Warning(*MessageToLog, Node);
		BroadcastRuntimeMessageAdded(TokenizedMessage);
	}
}

void UNarrativeAsset::LogNote(const FString& MessageToLog, UNarrativeNode* Node)
{
	// this is runtime log which is should be only called on runtime instances of asset
	if (TemplateAsset == nullptr)
	{
		UE_LOG(LogNarrative, Log, TEXT("Attempted to use Runtime Log on template asset %s"), *MessageToLog);
	}

	if (RuntimeLog.Get())
	{
		const TSharedRef<FTokenizedMessage> TokenizedMessage = RuntimeLog.Get()->Note(*MessageToLog, Node);
		BroadcastRuntimeMessageAdded(TokenizedMessage);
	}
}
#endif
