// Copyright XiaoYao

#pragma once

#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "Templates/SubclassOf.h"

#include "NarrativeTypes.h"
#include "Nodes/NarrativePin.h"
#include "NarrativeGraphNode.generated.h"

class UEdGraphSchema;
class UNarrativeNode;

DECLARE_DELEGATE(FNarrativeGraphNodeEvent);

/**
 * Graph representation of the Narrative Node
 */
UCLASS()
class NARRATIVEEDITOR_API UNarrativeGraphNode : public UEdGraphNode
{
	GENERATED_UCLASS_BODY()

//////////////////////////////////////////////////////////////////////////
// Narrative node

private:
	UPROPERTY(Instanced)
	UNarrativeNode* NarrativeNode;

	bool bBlueprintCompilationPending;
	bool bNeedsFullReconstruction;
	static bool bNarrativeAssetsLoaded;

public:
	// It would be intuitive to assign a custom Graph Node class in Narrative Node class
	// However, we shouldn't assign class from editor module to runtime module class
	UPROPERTY()
	TArray<TSubclassOf<UNarrativeNode>> AssignedNodeClasses;
	
	void SetNodeTemplate(UNarrativeNode* InNarrativeNode);
	const UNarrativeNode* GetNodeTemplate() const;

	UNarrativeNode* GetNarrativeNode() const;

	// UObject
	virtual void PostLoad() override;
	virtual void PostDuplicate(bool bDuplicateForPIE) override;
	virtual void PostEditImport() override;
	// --

	// UEdGraphNode
	virtual void PostPlacedNewNode() override;
	virtual void PrepareForCopying() override;
    // --
	
	void PostCopyNode();

private:
	void SubscribeToExternalChanges();
	void OnExternalChange();

public:
	virtual void OnGraphRefresh();

//////////////////////////////////////////////////////////////////////////
// Graph node

public:
	UPROPERTY()
	FNarrativePinTrait NodeBreakpoint;

	// UEdGraphNode
	virtual bool CanCreateUnderSpecifiedSchema(const UEdGraphSchema* Schema) const override;
	virtual void AutowireNewNode(UEdGraphPin* FromPin) override;
	// --

	/**
	 * Handles inserting the node between the FromPin and what the FromPin was original connected to
	 *
	 * @param FromPin			The pin this node is being spawned from
	 * @param NewLinkPin		The new pin the FromPin will connect to
	 * @param OutNodeList		Any nodes that are modified will get added to this list for notification purposes
	 */
	void InsertNewNode(UEdGraphPin* FromPin, UEdGraphPin* NewLinkPin, TSet<UEdGraphNode*>& OutNodeList);

	// UEdGraphNode
	virtual void ReconstructNode() override;
	virtual void AllocateDefaultPins() override;
	// --

	// variants of K2Node methods
	void RewireOldPinsToNewPins(TArray<UEdGraphPin*>& InOldPins);
	void ReconstructSinglePin(UEdGraphPin* NewPin, UEdGraphPin* OldPin);
	// --

	// UEdGraphNode
	virtual void GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;
	virtual bool CanUserDeleteNode() const override;
	virtual bool CanDuplicateNode() const override;
	virtual TSharedPtr<SGraphNode> CreateVisualWidget() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	virtual bool ShowPaletteIconOnNode() const override { return true; }
	virtual FText GetTooltipText() const override;
	// --

//////////////////////////////////////////////////////////////////////////
// Utils

public:
	// Short summary of node's content
	FString GetNodeDescription() const;

	// Get flow node for the inspected asset instance
	UNarrativeNode* GetInspectedNodeInstance() const;

	// Used for highlighting active nodes of the inspected asset instance
	ENarrativeNodeState GetActivationState() const;

	// Information displayed while node is active
	FString GetStatusString() const;
	FLinearColor GetStatusBackgroundColor() const;

	// Check this to display information while node is preloaded
	bool IsContentPreloaded() const;

	bool CanFocusViewport() const;

	// UEdGraphNode
	virtual bool CanJumpToDefinition() const override;
	virtual void JumpToDefinition() const override;
	// --

//////////////////////////////////////////////////////////////////////////
// Pins

public:
	TArray<UEdGraphPin*> InputPins;
	TArray<UEdGraphPin*> OutputPins;

	UPROPERTY()
	TMap<FEdGraphPinReference, FNarrativePinTrait> PinBreakpoints;

	void CreateInputPin(const FNarrativePin& NarrativePin, const int32 Index = INDEX_NONE);
	void CreateOutputPin(const FNarrativePin& NarrativePin, const int32 Index = INDEX_NONE);

	void RemoveOrphanedPin(UEdGraphPin* Pin);

	bool SupportsContextPins() const;

	bool CanUserAddInput() const;
	bool CanUserAddOutput() const;

	bool CanUserRemoveInput(const UEdGraphPin* Pin) const;
	bool CanUserRemoveOutput(const UEdGraphPin* Pin) const;

	void AddUserInput();
	void AddUserOutput();

	// Add pin only on this instance of node, under default pins
	void AddInstancePin(const EEdGraphPinDirection Direction, const uint8 NumberedPinsAmount);

	// Call node and graph updates manually, if using bBatchRemoval
	void RemoveInstancePin(UEdGraphPin* Pin);

	// Create pins from the context asset, i.e. Sequencer events
	void RefreshContextPins(const bool bReconstructNode);

	// UEdGraphNode
	virtual void GetPinHoverText(const UEdGraphPin& Pin, FString& HoverTextOut) const override;
	// --

//////////////////////////////////////////////////////////////////////////
// Breakpoints

public:
	void OnInputTriggered(const int32 Index);
	void OnOutputTriggered(const int32 Index);

private:
	void TryPausingSession(bool bPauseSession);

	void OnResumePIE(const bool bIsSimulating);
	void OnEndPIE(const bool bIsSimulating);
	void ResetBreakpoints();

//////////////////////////////////////////////////////////////////////////
// Execution Override

public:
	FNarrativeGraphNodeEvent OnSignalModeChanged;
	
	// Pin activation forced by user during PIE
	virtual void ForcePinActivation(const FEdGraphPinReference PinReference) const;

	// Pass-through forced by designer, set per node instance
	virtual void SetSignalMode(const ENarrativeSignalMode Mode);

	virtual ENarrativeSignalMode GetSignalMode() const;
	virtual bool CanSetSignalMode(const ENarrativeSignalMode Mode) const;
};
