// Copyright XiaoYao

#pragma once

#include "NarrativeSave.h"
#include "NarrativeTypes.h"
#include "Nodes/NarrativeNode.h"

#if WITH_EDITOR
#include "NarrativeMessageLog.h"
#endif

#include "UObject/ObjectKey.h"
#include "NarrativeAsset.generated.h"

class UNarrativeNode_CustomOutput;
class UNarrativeNode_CustomInput;
class UNarrativeNode_SubGraph;
class UNarrativeSubsystem;

class UEdGraph;
class UEdGraphNode;
class UNarrativeAsset;

#if WITH_EDITOR

/** Interface for calling the graph editor methods */
class NARRATIVE_API INarrativeGraphInterface
{
public:
	INarrativeGraphInterface() {}
	virtual ~INarrativeGraphInterface() {}

	virtual void OnInputTriggered(UEdGraphNode* GraphNode, const int32 Index) const {}
	virtual void OnOutputTriggered(UEdGraphNode* GraphNode, const int32 Index) const {}
};

DECLARE_DELEGATE(FNarrativeGraphEvent);

#endif

/**
 * Single asset containing flow nodes.
 */
UCLASS(BlueprintType, hideCategories = Object)
class NARRATIVE_API UNarrativeAsset : public UObject
{
	GENERATED_UCLASS_BODY()

public:	
	friend class UNarrativeNode;
	friend class UNarrativeNode_CustomOutput;
	friend class UNarrativeNode_SubGraph;
	friend class UNarrativeSubsystem;

	friend class FNarrativeAssetDetails;
	friend class FNarrativeNode_SubGraphDetails;
	friend class UNarrativeGraphSchema;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Narrative Asset")
	FGuid AssetGuid;

	// Set it to False, if this asset is instantiated as Root Narrative for owner that doesn't live in the world
	// This allow to SaveGame support works properly, if owner of Root Narrative would be Game Instance or its subsystem
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative Asset")
	bool bWorldBound;

//////////////////////////////////////////////////////////////////////////
// Graph

#if WITH_EDITOR
	friend class UNarrativeGraph;

	// UObject
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostDuplicate(bool bDuplicateForPIE) override;
	virtual void PostLoad() override;
	// --

	virtual EDataValidationResult ValidateAsset(FNarrativeMessageLog& MessageLog);

	// Returns whether the node class is allowed in this flow asset
	bool IsNodeClassAllowed(const UClass* NarrativeNodeClass, FText* OutOptionalFailureReason = nullptr) const;

	static FString ValidationError_NodeClassNotAllowed;
	static FString ValidationError_NullNodeInstance;

protected:
	bool CanNarrativeNodeClassBeUsedByNarrativeAsset(const UClass& NarrativeNodeClass) const;
	bool CanNarrativeAssetUseNarrativeNodeClass(const UClass& NarrativeNodeClass) const;
	bool CanNarrativeAssetReferenceNarrativeNode(const UClass& NarrativeNodeClass, FText* OutOptionalFailureReason = nullptr) const;
#endif

	// INarrativeGraphInterface
#if WITH_EDITORONLY_DATA

private:
	UPROPERTY()
	TObjectPtr<UEdGraph> NarrativeGraph;

	static TSharedPtr<INarrativeGraphInterface> NarrativeGraphInterface;
#endif

public:
#if WITH_EDITOR
	UEdGraph* GetGraph() const { return NarrativeGraph; };

	static void SetNarrativeGraphInterface(TSharedPtr<INarrativeGraphInterface> InNarrativeAssetEditor);
	static TSharedPtr<INarrativeGraphInterface> GetNarrativeGraphInterface() { return NarrativeGraphInterface; };
#endif
	// -- 

//////////////////////////////////////////////////////////////////////////
// Nodes

protected:
	TArray<TSubclassOf<UNarrativeNode>> AllowedNodeClasses;
	TArray<TSubclassOf<UNarrativeNode>> DeniedNodeClasses;

	TArray<TSubclassOf<UNarrativeNode>> AllowedInSubgraphNodeClasses;
	TArray<TSubclassOf<UNarrativeNode>> DeniedInSubgraphNodeClasses;
	
	bool bStartNodePlacedAsGhostNode;

private:
	UPROPERTY()
	TMap<FGuid, UNarrativeNode*> Nodes;

#if WITH_EDITORONLY_DATA
protected:
	/**
	 * Custom Inputs define custom entry points in graph, it's similar to blueprint Custom Events
	 * Sub Graph node using this Narrative Asset will generate context Input Pin for every valid Event name on this list
	 */
	UPROPERTY(EditAnywhere, Category = "Sub Graph")
	TArray<FName> CustomInputs;

	/**
	 * Custom Outputs define custom graph outputs, this allow to send signals to the parent graph while executing this graph
	 * Sub Graph node using this Narrative Asset will generate context Output Pin for every valid Event name on this list
	 */
	UPROPERTY(EditAnywhere, Category = "Sub Graph")
	TArray<FName> CustomOutputs;
#endif // WITH_EDITORONLY_DATA

public:
#if WITH_EDITOR
	FNarrativeGraphEvent OnSubGraphReconstructionRequested;

	UNarrativeNode* CreateNode(const UClass* NodeClass, UEdGraphNode* GraphNode);

	void RegisterNode(const FGuid& NewGuid, UNarrativeNode* NewNode);
	void UnregisterNode(const FGuid& NodeGuid);

	// Processes all nodes and creates map of all pin connections
	void HarvestNodeConnections();
#endif

	const TMap<FGuid, UNarrativeNode*>& GetNodes() const { return Nodes; }
	UNarrativeNode* GetNode(const FGuid& Guid) const { return Nodes.FindRef(Guid); }

	template <class T>
	T* GetNode(const FGuid& Guid) const
	{
		static_assert(TPointerIsConvertibleFromTo<T, const UNarrativeNode>::Value, "'T' template parameter to GetNode must be derived from UNarrativeNode");

		if (UNarrativeNode* Node = Nodes.FindRef(Guid))
		{
			return Cast<T>(Node);
		}

		return nullptr;
	}

	UFUNCTION(BlueprintPure, Category = "NarrativeAsset")
	virtual UNarrativeNode* GetDefaultEntryNode() const;

	UFUNCTION(BlueprintPure, Category = "NarrativeAsset", meta = (DeterminesOutputType = "NarrativeNodeClass"))
	TArray<UNarrativeNode*> GetNodesInExecutionOrder(UNarrativeNode* FirstIteratedNode, const TSubclassOf<UNarrativeNode> NarrativeNodeClass);

	template <class T>
	void GetNodesInExecutionOrder(UNarrativeNode* FirstIteratedNode, TArray<T*>& OutNodes)
	{
		static_assert(TPointerIsConvertibleFromTo<T, const UNarrativeNode>::Value, "'T' template parameter to GetNodesInExecutionOrder must be derived from UNarrativeNode");

		if (FirstIteratedNode)
		{
			TSet<TObjectKey<UNarrativeNode>> IteratedNodes;
			GetNodesInExecutionOrder_Recursive(FirstIteratedNode, IteratedNodes, OutNodes);
		}
	}

protected:
	template <class T>
	void GetNodesInExecutionOrder_Recursive(UNarrativeNode* Node, TSet<TObjectKey<UNarrativeNode>>& IteratedNodes, TArray<T*>& OutNodes)
	{
		IteratedNodes.Add(Node);

		if (T* NodeOfRequiredType = Cast<T>(Node))
		{
			OutNodes.Emplace(NodeOfRequiredType);
		}

		for (UNarrativeNode* ConnectedNode : Node->GetConnectedNodes())
		{
			if (ConnectedNode && !IteratedNodes.Contains(ConnectedNode))
			{
				GetNodesInExecutionOrder_Recursive(ConnectedNode, IteratedNodes, OutNodes);
			}
		}
	}

public:	
	UNarrativeNode_CustomInput* TryFindCustomInputNodeByEventName(const FName& EventName) const;
	UNarrativeNode_CustomOutput* TryFindCustomOutputNodeByEventName(const FName& EventName) const;

	TArray<FName> GatherCustomInputNodeEventNames() const;
	TArray<FName> GatherCustomOutputNodeEventNames() const;

#if WITH_EDITOR
	const TArray<FName>& GetCustomInputs() const { return CustomInputs; }
	const TArray<FName>& GetCustomOutputs() const { return CustomOutputs; }

protected:
	void AddCustomInput(const FName& EventName);
	void RemoveCustomInput(const FName& EventName);

	void AddCustomOutput(const FName& EventName);
	void RemoveCustomOutput(const FName& EventName);
#endif // WITH_EDITOR
	
//////////////////////////////////////////////////////////////////////////
// Instances of the template asset

private:
	// Original object holds references to instances
	UPROPERTY(Transient)
	TArray<UNarrativeAsset*> ActiveInstances;

#if WITH_EDITORONLY_DATA
	TWeakObjectPtr<UNarrativeAsset> InspectedInstance;

	// Message log for storing runtime errors/notes/warnings that will only last until the next game run
	// Log lives in the asset template, so it can be inspected after ending the PIE
	TSharedPtr<class FNarrativeMessageLog> RuntimeLog;
#endif

public:
	void AddInstance(UNarrativeAsset* Instance);
	int32 RemoveInstance(UNarrativeAsset* Instance);

	void ClearInstances();
	int32 GetInstancesNum() const { return ActiveInstances.Num(); }

#if WITH_EDITOR
	void GetInstanceDisplayNames(TArray<TSharedPtr<FName>>& OutDisplayNames) const;

	void SetInspectedInstance(const FName& NewInspectedInstanceName);
	UNarrativeAsset* GetInspectedInstance() const { return InspectedInstance.IsValid() ? InspectedInstance.Get() : nullptr; }

	DECLARE_EVENT(UNarrativeAsset, FRefreshDebuggerEvent);

	FRefreshDebuggerEvent& OnDebuggerRefresh() { return RefreshDebuggerEvent; }
	FRefreshDebuggerEvent RefreshDebuggerEvent;

	DECLARE_EVENT_TwoParams(UNarrativeAsset, FRuntimeMessageEvent, UNarrativeAsset*, const TSharedRef<FTokenizedMessage>&);

	FRuntimeMessageEvent& OnRuntimeMessageAdded() { return RuntimeMessageEvent; }
	FRuntimeMessageEvent RuntimeMessageEvent;

private:
	void BroadcastDebuggerRefresh() const;
	void BroadcastRuntimeMessageAdded(const TSharedRef<FTokenizedMessage>& Message);
#endif

//////////////////////////////////////////////////////////////////////////
// Executing asset instance

protected:
	UPROPERTY()
	UNarrativeAsset* TemplateAsset;

	// Object that spawned Root Narrative instance, i.e. World Settings or Player Controller
	// This pointer is passed to child instances: Narrative Asset instances created by the SubGraph nodes
	TWeakObjectPtr<UObject> Owner;

	// SubGraph node that created this Narrative Asset instance
	TWeakObjectPtr<UNarrativeNode_SubGraph> NodeOwningThisAssetInstance;

	// Narrative Asset instances created by SubGraph nodes placed in the current graph
	TMap<TWeakObjectPtr<UNarrativeNode_SubGraph>, TWeakObjectPtr<UNarrativeAsset>> ActiveSubGraphs;

	// Optional entry points to the graph, similar to blueprint Custom Events
	UPROPERTY()
	TSet<UNarrativeNode_CustomInput*> CustomInputNodes;

	UPROPERTY()
	TSet<UNarrativeNode*> PreloadedNodes;

	// Nodes that have any work left, not marked as Finished yet
	UPROPERTY()
	TArray<UNarrativeNode*> ActiveNodes;

	// All nodes active in the past, done their work
	UPROPERTY()
	TArray<UNarrativeNode*> RecordedNodes;

	ENarrativeFinishPolicy FinishPolicy;

public:
	virtual void InitializeInstance(const TWeakObjectPtr<UObject> InOwner, UNarrativeAsset* InTemplateAsset);
	virtual void DeinitializeInstance();

	UNarrativeAsset* GetTemplateAsset() const { return TemplateAsset; }

	// Object that spawned Root Narrative instance, i.e. World Settings or Player Controller
	// This pointer is passed to child instances: Narrative Asset instances created by the SubGraph nodes
	UFUNCTION(BlueprintPure, Category = "Narrative")
	UObject* GetOwner() const { return Owner.Get(); }

	template <class T>
	TWeakObjectPtr<T> GetOwner() const
	{
		return Owner.IsValid() ? Cast<T>(Owner) : nullptr;
	}

	// Returns the Owner as an Actor, or if Owner is a Component, return its Owner as an Actor
	UFUNCTION(BlueprintPure, Category = "Narrative")
	AActor* TryFindActorOwner() const;

	// Opportunity to preload content of project-specific nodes
	virtual void PreloadNodes() {}

	virtual void PreStartNarrative();
	virtual void StartNarrative();

	virtual void FinishNarrative(const ENarrativeFinishPolicy InFinishPolicy, const bool bRemoveInstance = true);

	bool HasStartedNarrative() const;
	void TriggerCustomInput(const FName& EventName);

	// Get Narrative Asset instance created by the given SubGraph node
	TWeakObjectPtr<UNarrativeAsset> GetNarrativeInstance(UNarrativeNode_SubGraph* SubGraphNode) const;

protected:
	void TriggerCustomInput_FromSubGraph(UNarrativeNode_SubGraph* Node, const FName& EventName) const;
	void TriggerCustomOutput(const FName& EventName);

	void TriggerInput(const FGuid& NodeGuid, const FName& PinName);

	void FinishNode(UNarrativeNode* Node);
	void ResetNodes();

public:
	UNarrativeSubsystem* GetNarrativeSubsystem() const;
	FName GetDisplayName() const;

	UNarrativeNode_SubGraph* GetNodeOwningThisAssetInstance() const;
	UNarrativeAsset* GetParentInstance() const;

	// Are there any active nodes?
	UFUNCTION(BlueprintPure, Category = "Narrative")
	bool IsActive() const { return ActiveNodes.Num() > 0; }

	// Returns nodes that have any work left, not marked as Finished yet
	UFUNCTION(BlueprintPure, Category = "Narrative")
	const TArray<UNarrativeNode*>& GetActiveNodes() const { return ActiveNodes; }

	// Returns nodes active in the past, done their work
	UFUNCTION(BlueprintPure, Category = "Narrative")
	const TArray<UNarrativeNode*>& GetRecordedNodes() const { return RecordedNodes; }

//////////////////////////////////////////////////////////////////////////
// Expected Owner Class support (for use with CallOwnerFunction nodes)

public:
	UClass* GetExpectedOwnerClass() const { return ExpectedOwnerClass; }

protected:
	// Expects to be owned (at runtime) by an object with this class (or one of its subclasses)
	// NOTE - If the class is an AActor, and the flow asset is owned by a component,
	//        it will consider the component's owner for the AActor
	UPROPERTY(EditAnywhere, Category = "Narrative", meta = (MustImplement = "/Script/Narrative.NarrativeOwnerInterface"))
	TSubclassOf<UObject> ExpectedOwnerClass;

//////////////////////////////////////////////////////////////////////////
// SaveGame support

public:
	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	FNarrativeAssetSaveData SaveInstance(TArray<FNarrativeAssetSaveData>& SavedNarrativeInstances);

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	void LoadInstance(const FNarrativeAssetSaveData& AssetRecord);

protected:
	virtual void OnActivationStateLoaded(UNarrativeNode* Node);

	UFUNCTION(BlueprintNativeEvent, Category = "SaveGame")
	void OnSave();

	UFUNCTION(BlueprintNativeEvent, Category = "SaveGame")
	void OnLoad();

public:
	UFUNCTION(BlueprintNativeEvent, Category = "SaveGame")
	bool IsBoundToWorld();

//////////////////////////////////////////////////////////////////////////
// Utils

#if WITH_EDITOR
public:
	void LogError(const FString& MessageToLog, UNarrativeNode* Node);
	void LogWarning(const FString& MessageToLog, UNarrativeNode* Node);
	void LogNote(const FString& MessageToLog, UNarrativeNode* Node);
#endif
};
