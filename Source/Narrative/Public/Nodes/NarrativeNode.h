// Copyright XiaoYao

#pragma once

#include "EdGraph/EdGraphNode.h"
#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"
#include "VisualLogger/VisualLoggerDebugSnapshotInterface.h"

#include "NarrativeMessageLog.h"
#include "NarrativeTypes.h"
#include "Nodes/NarrativePin.h"
#include "NarrativeNode.generated.h"

class INarrativeOwnerInterface;
class UNarrativeAsset;
class UNarrativeSubsystem;
struct FNarrativeNodeSaveData;

#if WITH_EDITOR
DECLARE_DELEGATE(FNarrativeNodeEvent);
#endif

/**
 * A Narrative Node is UObject-based node designed to handle entire gameplay feature within single node.
 */
UCLASS(Abstract, Blueprintable, HideCategories = Object)
class NARRATIVE_API UNarrativeNode : public UObject, public IVisualLoggerDebugSnapshotInterface
{
	GENERATED_UCLASS_BODY()
	friend class SNarrativeGraphNode;
	friend class UNarrativeAsset;
	friend class UNarrativeGraphNode;
	friend class UNarrativeGraphSchema;
	friend class SNarrativeInputPinHandle;
	friend class SNarrativeOutputPinHandle;

//////////////////////////////////////////////////////////////////////////
// Node

private:
	UPROPERTY()
	UEdGraphNode* GraphNode;

#if WITH_EDITORONLY_DATA

protected:
	UPROPERTY()
	TArray<TSubclassOf<UNarrativeAsset>> AllowedAssetClasses;

	UPROPERTY()
	TArray<TSubclassOf<UNarrativeAsset>> DeniedAssetClasses;

	UPROPERTY()
	FString Category;

	UPROPERTY(EditDefaultsOnly, Category = "NarrativeNode")
	ENarrativeNodeStyle NodeStyle;

	// Set Node Style to custom to use your own color for this node
	UPROPERTY(EditDefaultsOnly, Category = "NarrativeNode", meta = (EditCondition = "NodeStyle == ENarrativeNodeStyle::Custom"))
	FLinearColor NodeColor;

	uint8 bCanDelete : 1;
	uint8 bCanDuplicate : 1;

	UPROPERTY(EditDefaultsOnly, Category = "NarrativeNode")
	bool bNodeDeprecated;

	// If this node is deprecated, it might be replaced by another node
	UPROPERTY(EditDefaultsOnly, Category = "NarrativeNode")
	TSubclassOf<UNarrativeNode> ReplacedBy;

public:
	FNarrativeNodeEvent OnReconstructionRequested;
#endif

public:
#if WITH_EDITOR
	// UObject	
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostLoad() override;
	// --

	// Opportunity to update node's data before UNarrativeGraphNode would call ReconstructNode()
	virtual void FixNode(UEdGraphNode* NewGraphNode);

	virtual EDataValidationResult ValidateNode() { return EDataValidationResult::NotValidated; }

	// used when import graph from another asset
	virtual void PostImport() {}
#endif

	UEdGraphNode* GetGraphNode() const { return GraphNode; }

#if WITH_EDITOR
	void SetGraphNode(UEdGraphNode* NewGraph);

	virtual FString GetNodeCategory() const;
	virtual FText GetNodeTitle() const;
	virtual FText GetNodeToolTip() const;

	// This method allows to have different for every node instance, i.e. Red if node represents enemy, Green if node represents a friend
	virtual bool GetDynamicTitleColor(FLinearColor& OutColor) const;

	ENarrativeNodeStyle GetNodeStyle() const { return NodeStyle; }

	// Short summary of node's content - displayed over node as NodeInfoPopup
	virtual FString GetNodeDescription() const;
#endif

protected:
	// Short summary of node's content - displayed over node as NodeInfoPopup
	UFUNCTION(BlueprintImplementableEvent, Category = "NarrativeNode", meta = (DisplayName = "Get Node Description"))
	FString K2_GetNodeDescription() const;

	// Inherits Guid after graph node
	UPROPERTY()
	FGuid NodeGuid;

public:
	UFUNCTION(BlueprintCallable, Category = "NarrativeNode")
	void SetGuid(const FGuid& NewGuid) { NodeGuid = NewGuid; }

	UFUNCTION(BlueprintPure, Category = "NarrativeNode")
	const FGuid& GetGuid() const { return NodeGuid; }

	UFUNCTION(BlueprintPure, Category = "NarrativeNode")
	UNarrativeAsset* GetNarrativeAsset() const;

	// Gets the Owning Actor for this Node's RootNarrative
	// (if the immediate parent is an UActorComponent, it will get that Component's actor)
	AActor* TryGetRootNarrativeActorOwner() const;

	// Returns the INarrativeOwnerInterface for the owner object (if implemented)
	//  NOTE - will consider a UActorComponent owner's owning actor if appropriate
	INarrativeOwnerInterface* GetNarrativeOwnerInterface() const;

protected:

	// Helper functions for GetNarrativeOwnerInterface()
	INarrativeOwnerInterface* TryGetNarrativeOwnerInterfaceFromRootNarrativeOwner(UObject& RootNarrativeOwner, const UClass& ExpectedOwnerClass) const;
	INarrativeOwnerInterface* TryGetNarrativeOwnerInterfaceActor(UObject& RootNarrativeOwner, const UClass& ExpectedOwnerClass) const;

	// Gets the Owning Object for this Node's RootNarrative
	UObject* TryGetRootNarrativeObjectOwner() const;

public:	
	virtual bool CanFinishGraph() const { return false; }

protected:
	UPROPERTY(EditDefaultsOnly, Category = "NarrativeNode")
	TArray<ENarrativeSignalMode> AllowedSignalModes;

	// If enabled, signal will pass through node without calling ExecuteInput()
	// Designed to handle patching
	UPROPERTY()
	ENarrativeSignalMode SignalMode;

#if WITH_EDITOR
	FNarrativeMessageLog ValidationLog;
#endif

//////////////////////////////////////////////////////////////////////////
// All created pins (default, class-specific and added by user)

public:
	static FNarrativePin DefaultInputPin;
	static FNarrativePin DefaultOutputPin;

protected:
	// Class-specific and user-added inputs
	UPROPERTY(EditDefaultsOnly, Category = "NarrativeNode")
	TArray<FNarrativePin> InputPins;

	// Class-specific and user-added outputs
	UPROPERTY(EditDefaultsOnly, Category = "NarrativeNode")
	TArray<FNarrativePin> OutputPins;

	void AddInputPins(TArray<FNarrativePin> Pins);
	void AddOutputPins(TArray<FNarrativePin> Pins);

	// always use default range for nodes with user-created outputs i.e. Execution Sequence
	void SetNumberedInputPins(const uint8 FirstNumber = 0, const uint8 LastNumber = 1);
	void SetNumberedOutputPins(const uint8 FirstNumber = 0, const uint8 LastNumber = 1);

	uint8 CountNumberedInputs() const;
	uint8 CountNumberedOutputs() const;

	const TArray<FNarrativePin>& GetInputPins() const { return InputPins; }
	const TArray<FNarrativePin>& GetOutputPins() const { return OutputPins; }

public:
	UFUNCTION(BlueprintPure, Category = "NarrativeNode")
	TArray<FName> GetInputNames() const;

	UFUNCTION(BlueprintPure, Category = "NarrativeNode")
	TArray<FName> GetOutputNames() const;

#if WITH_EDITOR
	virtual bool SupportsContextPins() const { return false; }

	// Be careful, enabling it might cause loading gigabytes of data as nodes would load all related data (i.e. Level Sequences)
	virtual bool CanRefreshContextPinsOnLoad() const { return false; }

	virtual TArray<FNarrativePin> GetContextInputs() { return TArray<FNarrativePin>(); }
	virtual TArray<FNarrativePin> GetContextOutputs() { return TArray<FNarrativePin>(); }

	virtual bool CanUserAddInput() const;
	virtual bool CanUserAddOutput() const;

	void RemoveUserInput(const FName& PinName);
	void RemoveUserOutput(const FName& PinName);
#endif

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "NarrativeNode", meta = (DisplayName = "Can User Add Input"))
	bool K2_CanUserAddInput() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "NarrativeNode", meta = (DisplayName = "Can User Add Output"))
	bool K2_CanUserAddOutput() const;

//////////////////////////////////////////////////////////////////////////
// Connections to other nodes

protected:
	// Map outputs to the connected node and input pin
	UPROPERTY()
	TMap<FName, FConnectedPin> Connections;

public:
	void SetConnections(const TMap<FName, FConnectedPin>& InConnections) { Connections = InConnections; }
	FConnectedPin GetConnection(const FName OutputName) const { return Connections.FindRef(OutputName); }

	UFUNCTION(BlueprintPure, Category= "NarrativeNode")
	TSet<UNarrativeNode*> GetConnectedNodes() const;
	
	FName GetPinConnectedToNode(const FGuid& OtherNodeGuid);

	UFUNCTION(BlueprintPure, Category= "NarrativeNode")
	bool IsInputConnected(const FName& PinName) const;

	UFUNCTION(BlueprintPure, Category= "NarrativeNode")
	bool IsOutputConnected(const FName& PinName) const;

	static void RecursiveFindNodesByClass(UNarrativeNode* Node, const TSubclassOf<UNarrativeNode> Class, uint8 Depth, TArray<UNarrativeNode*>& OutNodes);

//////////////////////////////////////////////////////////////////////////
// Debugger
protected:
	static FString MissingIdentityTag;
	static FString MissingNotifyTag;
	static FString MissingClass;
	static FString NoActorsFound;

//////////////////////////////////////////////////////////////////////////
// Executing node instance

public:
	bool bPreloaded;

protected:
	UPROPERTY(SaveGame)
	ENarrativeNodeState ActivationState;

public:
	ENarrativeNodeState GetActivationState() const { return ActivationState; }

#if !UE_BUILD_SHIPPING

private:
	TMap<FName, TArray<FPinRecord>> InputRecords;
	TMap<FName, TArray<FPinRecord>> OutputRecords;
#endif

public:
	UFUNCTION(BlueprintPure, Category = "NarrativeNode")
	UNarrativeSubsystem* GetNarrativeSubsystem() const;

	virtual UWorld* GetWorld() const override;

protected:
	// Method called just after creating the node instance, while initializing the Narrative Asset instance
	// This happens before executing graph, only called during gameplay
	virtual void InitializeInstance();

	// Event called just after creating the node instance, while initializing the Narrative Asset instance
	// This happens before executing graph, only called during gameplay
	UFUNCTION(BlueprintImplementableEvent, Category = "NarrativeNode", meta = (DisplayName = "Init Instance"))
	void K2_InitializeInstance();

public:
	void TriggerPreload();
	void TriggerFlush();

protected:
	virtual void PreloadContent();
	virtual void FlushContent();

	UFUNCTION(BlueprintImplementableEvent, Category = "NarrativeNode", meta = (DisplayName = "Preload Content"))
	void K2_PreloadContent();

	UFUNCTION(BlueprintImplementableEvent, Category = "NarrativeNode", meta = (DisplayName = "Flush Content"))
	void K2_FlushContent();

	virtual void OnActivate();

	UFUNCTION(BlueprintImplementableEvent, Category = "NarrativeNode", meta = (DisplayName = "On Activate"))
	void K2_OnActivate();

	// Trigger execution of input pin
	void TriggerInput(const FName& PinName, const ENarrativePinActivationType ActivationType = ENarrativePinActivationType::Default);

	// Method reacting on triggering Input pin
	virtual void ExecuteInput(const FName& PinName);

	// Event reacting on triggering Input pin
	UFUNCTION(BlueprintImplementableEvent, Category = "NarrativeNode", meta = (DisplayName = "Execute Input"))
	void K2_ExecuteInput(const FName& PinName);

	// Simply trigger the first Output Pin, convenient to use if node has only one output
	UFUNCTION(BlueprintCallable, Category = "NarrativeNode")
	void TriggerFirstOutput(const bool bFinish);

	UFUNCTION(BlueprintCallable, Category = "NarrativeNode", meta = (HidePin = "bForcedActivation"))
	void TriggerOutput(const FName& PinName, const bool bFinish = false, const ENarrativePinActivationType ActivationType = ENarrativePinActivationType::Default);

	void TriggerOutput(const FString& PinName, const bool bFinish = false);
	void TriggerOutput(const FText& PinName, const bool bFinish = false);
	void TriggerOutput(const TCHAR* PinName, const bool bFinish = false);

	UFUNCTION(BlueprintCallable, Category = "NarrativeNode", meta = (HidePin = "ActivationType"))
	void TriggerOutputPin(const FNarrativeOutputPinHandle Pin, const bool bFinish = false, const ENarrativePinActivationType ActivationType = ENarrativePinActivationType::Default);

public:
	// Finish execution of node, it will call Cleanup
	UFUNCTION(BlueprintCallable, Category = "NarrativeNode")
	void Finish();

protected:
	void Deactivate();

	// Method called after node finished the work
	virtual void Cleanup();

	// Event called after node finished the work
	UFUNCTION(BlueprintImplementableEvent, Category = "NarrativeNode", meta = (DisplayName = "Cleanup"))
	void K2_Cleanup();

	// Method called from UNarrativeAsset::DeinitializeInstance()
	virtual void DeinitializeInstance();

	// Event called from UNarrativeAsset::DeinitializeInstance()
	UFUNCTION(BlueprintImplementableEvent, Category = "NarrativeNode", meta = (DisplayName = "DeinitializeInstance"))
	void K2_DeinitializeInstance();

public:
	// Define what happens when node is terminated from the outside
	virtual void ForceFinishNode();

protected:
	// Define what happens when node is terminated from the outside
	UFUNCTION(BlueprintImplementableEvent, Category = "NarrativeNode", meta = (DisplayName = "Force Finish Node"))
	void K2_ForceFinishNode();

private:
	void ResetRecords();

//////////////////////////////////////////////////////////////////////////
// SaveGame support

public:
	UFUNCTION(BlueprintCallable, Category = "NarrativeNode")
	void SaveInstance(FNarrativeNodeSaveData& NodeRecord);

	UFUNCTION(BlueprintCallable, Category = "NarrativeNode")
	void LoadInstance(const FNarrativeNodeSaveData& NodeRecord);

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "NarrativeNode")
	void OnSave();

	UFUNCTION(BlueprintNativeEvent, Category = "NarrativeNode")
	void OnLoad();

	UFUNCTION(BlueprintNativeEvent, Category = "NarrativeNode")
	void OnPassThrough();
	
//////////////////////////////////////////////////////////////////////////
// Utils

#if WITH_EDITOR
public:
	UNarrativeNode* GetInspectedInstance() const;

	TMap<uint8, FPinRecord> GetWireRecords() const;
	TArray<FPinRecord> GetPinRecords(const FName& PinName, const EEdGraphPinDirection PinDirection) const;

	// Information displayed while node is working - displayed over node as NodeInfoPopup
	virtual FString GetStatusString() const;
	virtual bool GetStatusBackgroundColor(FLinearColor& OutColor) const;

	virtual FString GetAssetPath();
	virtual UObject* GetAssetToEdit();
	virtual AActor* GetActorToFocus();
#endif

protected:
	// Information displayed while node is working - displayed over node as NodeInfoPopup
	UFUNCTION(BlueprintImplementableEvent, Category = "NarrativeNode", meta = (DisplayName = "Get Status String"))
	FString K2_GetStatusString() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "NarrativeNode", meta = (DisplayName = "Get Status Background Color"))
	bool K2_GetStatusBackgroundColor(FLinearColor& OutColor) const;

	UFUNCTION(BlueprintImplementableEvent, Category = "NarrativeNode", meta = (DisplayName = "Get Asset Path"))
	FString K2_GetAssetPath();

	UFUNCTION(BlueprintImplementableEvent, Category = "NarrativeNode", meta = (DisplayName = "Get Asset To Edit"))
	UObject* K2_GetAssetToEdit();

	UFUNCTION(BlueprintImplementableEvent, Category = "NarrativeNode", meta = (DisplayName = "Get Actor To Focus"))
	AActor* K2_GetActorToFocus();

public:
	UFUNCTION(BlueprintPure, Category = "NarrativeNode")
	static FString GetIdentityTagDescription(const FGameplayTag& Tag);

	UFUNCTION(BlueprintPure, Category = "NarrativeNode")
	static FString GetIdentityTagsDescription(const FGameplayTagContainer& Tags);

	UFUNCTION(BlueprintPure, Category = "NarrativeNode")
	static FString GetNotifyTagsDescription(const FGameplayTagContainer& Tags);

	UFUNCTION(BlueprintPure, Category = "NarrativeNode")
	static FString GetClassDescription(const TSubclassOf<UObject> Class);

	UFUNCTION(BlueprintPure, Category = "NarrativeNode")
	static FString GetProgressAsString(float Value);

public:
	UFUNCTION(BlueprintCallable, Category = "NarrativeNode", meta = (DevelopmentOnly))
	void LogError(FString Message, const ENarrativeOnScreenMessageType OnScreenMessageType = ENarrativeOnScreenMessageType::Permanent);

	UFUNCTION(BlueprintCallable, Category = "NarrativeNode", meta = (DevelopmentOnly))
	void LogWarning(FString Message);

	UFUNCTION(BlueprintCallable, Category = "NarrativeNode", meta = (DevelopmentOnly))
	void LogNote(FString Message);

#if !UE_BUILD_SHIPPING
private:
	bool BuildMessage(FString& Message) const;
#endif
};
