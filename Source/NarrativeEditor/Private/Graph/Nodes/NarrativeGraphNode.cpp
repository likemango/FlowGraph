// Copyright XiaoYao

#include "Graph/Nodes/NarrativeGraphNode.h"

#include "Asset/NarrativeDebuggerSubsystem.h"
#include "NarrativeEditorCommands.h"
#include "Graph/NarrativeGraph.h"
#include "Graph/NarrativeGraphEditorSettings.h"
#include "Graph/NarrativeGraphSchema.h"
#include "Graph/NarrativeGraphSettings.h"
#include "Graph/Widgets/SNarrativeGraphNode.h"

#include "NarrativeAsset.h"
#include "Nodes/NarrativeNode.h"

#include "Developer/ToolMenus/Public/ToolMenus.h"
#include "EdGraph/EdGraphSchema.h"
#include "EdGraphSchema_K2.h"
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "Framework/Commands/GenericCommands.h"
#include "GraphEditorActions.h"
#include "HAL/FileManager.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "ScopedTransaction.h"
#include "SourceCodeNavigation.h"
#include "Textures/SlateIcon.h"
#include "ToolMenuSection.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativeGraphNode)

#define LOCTEXT_NAMESPACE "NarrativeGraphNode"

UNarrativeGraphNode::UNarrativeGraphNode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, NarrativeNode(nullptr)
	, bBlueprintCompilationPending(false)
	, bNeedsFullReconstruction(false)
{
	OrphanedPinSaveMode = ESaveOrphanPinMode::SaveAll;
}

void UNarrativeGraphNode::SetNodeTemplate(UNarrativeNode* InNarrativeNode)
{
	NarrativeNode = InNarrativeNode;
}

const UNarrativeNode* UNarrativeGraphNode::GetNodeTemplate() const
{
	return NarrativeNode;
}

UNarrativeNode* UNarrativeGraphNode::GetNarrativeNode() const
{
	if (NarrativeNode)
	{
		if (const UNarrativeAsset* InspectedInstance = NarrativeNode->GetNarrativeAsset()->GetInspectedInstance())
		{
			return InspectedInstance->GetNode(NarrativeNode->GetGuid());
		}

		return NarrativeNode;
	}

	return nullptr;
}

void UNarrativeGraphNode::PostLoad()
{
	Super::PostLoad();

	if (NarrativeNode)
	{
		NarrativeNode->FixNode(this); // fix already created nodes
		SubscribeToExternalChanges();
	}

	ReconstructNode();
}

void UNarrativeGraphNode::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);

	if (!bDuplicateForPIE)
	{
		CreateNewGuid();

		if (NarrativeNode && NarrativeNode->GetNarrativeAsset())
		{
			NarrativeNode->GetNarrativeAsset()->RegisterNode(NodeGuid, NarrativeNode);
		}
	}
}

void UNarrativeGraphNode::PostEditImport()
{
	Super::PostEditImport();

	PostCopyNode();
	SubscribeToExternalChanges();
}

void UNarrativeGraphNode::PostPlacedNewNode()
{
	Super::PostPlacedNewNode();

	SubscribeToExternalChanges();
}

void UNarrativeGraphNode::PrepareForCopying()
{
	Super::PrepareForCopying();

	if (NarrativeNode)
	{
		// Temporarily take ownership of the NarrativeNode, so that it is not deleted when cutting
		NarrativeNode->Rename(nullptr, this, REN_DontCreateRedirectors);
	}
}

void UNarrativeGraphNode::PostCopyNode()
{
	// Make sure this NarrativeNode is owned by the NarrativeAsset it's being pasted into
	if (NarrativeNode)
	{
		UNarrativeAsset* NarrativeAsset = CastChecked<UNarrativeGraph>(GetGraph())->GetNarrativeAsset();

		if (NarrativeNode->GetOuter() != NarrativeAsset)
		{
			// Ensures NarrativeNode is owned by the NarrativeAsset
			NarrativeNode->Rename(nullptr, NarrativeAsset, REN_DontCreateRedirectors);
		}

		NarrativeNode->SetGraphNode(this);
	}
}

void UNarrativeGraphNode::SubscribeToExternalChanges()
{
	if (NarrativeNode)
	{
		NarrativeNode->OnReconstructionRequested.BindUObject(this, &UNarrativeGraphNode::OnExternalChange);
	}
}

void UNarrativeGraphNode::OnExternalChange()
{
	// Do not create transaction here, since this function triggers from modifying UNarrativeNode's property, which itself already made inside of transaction.
	Modify();
	
	bNeedsFullReconstruction = true;

	ReconstructNode();
	GetGraph()->NotifyGraphChanged();
}

void UNarrativeGraphNode::OnGraphRefresh()
{
	RefreshContextPins(true);
}

bool UNarrativeGraphNode::CanCreateUnderSpecifiedSchema(const UEdGraphSchema* Schema) const
{
	return Schema->IsA(UNarrativeGraphSchema::StaticClass());
}

void UNarrativeGraphNode::AutowireNewNode(UEdGraphPin* FromPin)
{
	if (FromPin != nullptr)
	{
		const UNarrativeGraphSchema* Schema = CastChecked<UNarrativeGraphSchema>(GetSchema());

		TSet<UEdGraphNode*> NodeList;

		// auto-connect from dragged pin to first compatible pin on the new node
		for (UEdGraphPin* Pin : Pins)
		{
			check(Pin);
			FPinConnectionResponse Response = Schema->CanCreateConnection(FromPin, Pin);
			if (CONNECT_RESPONSE_MAKE == Response.Response)
			{
				if (Schema->TryCreateConnection(FromPin, Pin))
				{
					NodeList.Add(FromPin->GetOwningNode());
					NodeList.Add(this);
				}
				break;
			}
			else if (CONNECT_RESPONSE_BREAK_OTHERS_A == Response.Response)
			{
				InsertNewNode(FromPin, Pin, NodeList);
				break;
			}
		}

		// Send all nodes that received a new pin connection a notification
		for (auto It = NodeList.CreateConstIterator(); It; ++It)
		{
			UEdGraphNode* Node = (*It);
			Node->NodeConnectionListChanged();
		}
	}
}

void UNarrativeGraphNode::InsertNewNode(UEdGraphPin* FromPin, UEdGraphPin* NewLinkPin, TSet<UEdGraphNode*>& OutNodeList)
{
	const UNarrativeGraphSchema* Schema = CastChecked<UNarrativeGraphSchema>(GetSchema());

	// The pin we are creating from already has a connection that needs to be broken. We want to "insert" the new node in between, so that the output of the new node is hooked up too
	UEdGraphPin* OldLinkedPin = FromPin->LinkedTo[0];
	check(OldLinkedPin);

	FromPin->BreakAllPinLinks();

	// Hook up the old linked pin to the first valid output pin on the new node
	for (int32 PinIndex = 0; PinIndex < Pins.Num(); PinIndex++)
	{
		UEdGraphPin* OutputExecPin = Pins[PinIndex];
		check(OutputExecPin);
		if (CONNECT_RESPONSE_MAKE == Schema->CanCreateConnection(OldLinkedPin, OutputExecPin).Response)
		{
			if (Schema->TryCreateConnection(OldLinkedPin, OutputExecPin))
			{
				OutNodeList.Add(OldLinkedPin->GetOwningNode());
				OutNodeList.Add(this);
			}
			break;
		}
	}

	if (Schema->TryCreateConnection(FromPin, NewLinkPin))
	{
		OutNodeList.Add(FromPin->GetOwningNode());
		OutNodeList.Add(this);
	}
}

void UNarrativeGraphNode::ReconstructNode()
{
	// Store old pins
	TArray<UEdGraphPin*> OldPins(Pins);

	// Reset pin arrays
	Pins.Reset();
	InputPins.Reset();
	OutputPins.Reset();

	// Recreate pins
	if (SupportsContextPins() && (NarrativeNode->CanRefreshContextPinsOnLoad() || bNeedsFullReconstruction))
	{
		RefreshContextPins(false);
	}
	AllocateDefaultPins();
	RewireOldPinsToNewPins(OldPins);

	// Destroy old pins
	for (UEdGraphPin* OldPin : OldPins)
	{
		OldPin->Modify();
		OldPin->BreakAllPinLinks();
		DestroyPin(OldPin);
	}

	bNeedsFullReconstruction = false;
}

void UNarrativeGraphNode::AllocateDefaultPins()
{
	check(Pins.Num() == 0);

	if (NarrativeNode)
	{
		for (const FNarrativePin& InputPin : NarrativeNode->InputPins)
		{
			CreateInputPin(InputPin);
		}

		for (const FNarrativePin& OutputPin : NarrativeNode->OutputPins)
		{
			CreateOutputPin(OutputPin);
		}
	}
}

void UNarrativeGraphNode::RewireOldPinsToNewPins(TArray<UEdGraphPin*>& InOldPins)
{
	TArray<UEdGraphPin*> OrphanedOldPins;
	TArray<bool> NewPinMatched; // Tracks whether a NewPin has already been matched to an OldPin
	TMap<UEdGraphPin*, UEdGraphPin*> MatchedPins; // Old to New

	const int32 NumNewPins = Pins.Num();
	NewPinMatched.AddDefaulted(NumNewPins);

	// Rewire any connection to pins that are matched by name (O(N^2) right now)
	// NOTE: we iterate backwards through the list because ReconstructSinglePin()
	//       destroys pins as we go along (clearing out parent pointers, etc.); 
	//       we need the parent pin chain intact for DoPinsMatchForReconstruction();              
	//       we want to destroy old pins from the split children (leaves) up, so 
	//       we do this since split child pins are ordered later in the list 
	//       (after their parents) 
	for (int32 OldPinIndex = InOldPins.Num() - 1; OldPinIndex >= 0; --OldPinIndex)
	{
		UEdGraphPin* OldPin = InOldPins[OldPinIndex];

		// common case is for InOldPins and Pins to match, so we start searching from the current index:
		bool bMatched = false;
		int32 NewPinIndex = (NumNewPins ? OldPinIndex % NumNewPins : 0);
		for (int32 NewPinCount = NumNewPins - 1; NewPinCount >= 0; --NewPinCount)
		{
			// if Pins grows then we may skip entries and fail to find a match or NewPinMatched will not be accurate
			check(NumNewPins == Pins.Num());
			if (!NewPinMatched[NewPinIndex])
			{
				UEdGraphPin* NewPin = Pins[NewPinIndex];

				if (NewPin->PinName == OldPin->PinName)
				{
					ReconstructSinglePin(NewPin, OldPin);

					MatchedPins.Add(OldPin, NewPin);
					bMatched = true;
					NewPinMatched[NewPinIndex] = true;
					break;
				}
			}
			NewPinIndex = (NewPinIndex + 1) % Pins.Num();
		}

		// Orphaned pins are those that existed in the OldPins array but do not in the NewPins.
		// We will save these pins and add them to the NewPins array if they are linked to other pins or have non-default value unless:
		// * The node has been flagged to not save orphaned pins
		// * The pin has been flagged not be saved if orphaned
		// * The pin is hidden
		if (UEdGraphPin::AreOrphanPinsEnabled() && !bDisableOrphanPinSaving && OrphanedPinSaveMode == ESaveOrphanPinMode::SaveAll
			&& !bMatched && !OldPin->bHidden && OldPin->ShouldSavePinIfOrphaned() && OldPin->LinkedTo.Num() > 0)
		{
			OldPin->bOrphanedPin = true;
			OldPin->bNotConnectable = true;
			OrphanedOldPins.Add(OldPin);
			InOldPins.RemoveAt(OldPinIndex, 1, false);
		}
	}

	// The orphaned pins get placed after the rest of the new pins
	for (int32 OrphanedIndex = OrphanedOldPins.Num() - 1; OrphanedIndex >= 0; --OrphanedIndex)
	{
		UEdGraphPin* OrphanedPin = OrphanedOldPins[OrphanedIndex];
		if (OrphanedPin->ParentPin == nullptr)
		{
			Pins.Add(OrphanedPin);
		}
	}
}

void UNarrativeGraphNode::ReconstructSinglePin(UEdGraphPin* NewPin, UEdGraphPin* OldPin)
{
	check(NewPin && OldPin);

	// Copy over modified persistent data
	NewPin->MovePersistentDataFromOldPin(*OldPin);

	// Update the in breakpoints as the old pin will be going the way of the dodo
	for (TPair<FEdGraphPinReference, FNarrativePinTrait>& PinBreakpoint : PinBreakpoints)
	{
		if (PinBreakpoint.Key.Get() == OldPin)
		{
			PinBreakpoint.Key = NewPin;
			break;
		}
	}
}

void UNarrativeGraphNode::GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const
{
	const FGenericCommands& GenericCommands = FGenericCommands::Get();
	const FGraphEditorCommandsImpl& GraphCommands = FGraphEditorCommands::Get();
	const FNarrativeGraphCommands& NarrativeGraphCommands = FNarrativeGraphCommands::Get();

	if (Context->Pin)
	{
		{
			FToolMenuSection& Section = Menu->AddSection("NarrativeGraphPinActions", LOCTEXT("PinActionsMenuHeader", "Pin Actions"));
			if (Context->Pin->LinkedTo.Num() > 0)
			{
				Section.AddMenuEntry(GraphCommands.BreakPinLinks);
			}

			if (Context->Pin->Direction == EGPD_Input && CanUserRemoveInput(Context->Pin))
			{
				Section.AddMenuEntry(NarrativeGraphCommands.RemovePin);
			}
			else if (Context->Pin->Direction == EGPD_Output && CanUserRemoveOutput(Context->Pin))
			{
				Section.AddMenuEntry(NarrativeGraphCommands.RemovePin);
			}
		}

		{
			FToolMenuSection& Section = Menu->AddSection("NarrativeGraphPinBreakpoints", LOCTEXT("PinBreakpointsMenuHeader", "Pin Breakpoints"));
			Section.AddMenuEntry(NarrativeGraphCommands.AddPinBreakpoint);
			Section.AddMenuEntry(NarrativeGraphCommands.RemovePinBreakpoint);
			Section.AddMenuEntry(NarrativeGraphCommands.EnablePinBreakpoint);
			Section.AddMenuEntry(NarrativeGraphCommands.DisablePinBreakpoint);
			Section.AddMenuEntry(NarrativeGraphCommands.TogglePinBreakpoint);
		}

		{
			FToolMenuSection& Section = Menu->AddSection("NarrativeGraphPinExecutionOverride", LOCTEXT("PinExecutionOverrideMenuHeader", "Execution Override"));
			Section.AddMenuEntry(NarrativeGraphCommands.ForcePinActivation);
		}
	}
	else if (Context->Node)
	{
		{
			FToolMenuSection& Section = Menu->AddSection("NarrativeGraphNodeActions", LOCTEXT("NodeActionsMenuHeader", "Node Actions"));
			Section.AddMenuEntry(GenericCommands.Delete);
			Section.AddMenuEntry(GenericCommands.Cut);
			Section.AddMenuEntry(GenericCommands.Copy);
			Section.AddMenuEntry(GenericCommands.Duplicate);

			Section.AddMenuEntry(GraphCommands.BreakNodeLinks);

			if (SupportsContextPins())
			{
				Section.AddMenuEntry(NarrativeGraphCommands.RefreshContextPins);
			}

			if (CanUserAddInput())
			{
				Section.AddMenuEntry(NarrativeGraphCommands.AddInput);
			}
			if (CanUserAddOutput())
			{
				Section.AddMenuEntry(NarrativeGraphCommands.AddOutput);
			}
		}

		{
			FToolMenuSection& Section = Menu->AddSection("NarrativeGraphNodeBreakpoints", LOCTEXT("NodeBreakpointsMenuHeader", "Node Breakpoints"));
			Section.AddMenuEntry(GraphCommands.AddBreakpoint);
			Section.AddMenuEntry(GraphCommands.RemoveBreakpoint);
			Section.AddMenuEntry(GraphCommands.EnableBreakpoint);
			Section.AddMenuEntry(GraphCommands.DisableBreakpoint);
			Section.AddMenuEntry(GraphCommands.ToggleBreakpoint);
		}

		{
			FToolMenuSection& Section = Menu->AddSection("NarrativeGraphNodeExecutionOverride", LOCTEXT("NodeExecutionOverrideMenuHeader", "Execution Override"));
			if (CanSetSignalMode(ENarrativeSignalMode::Enabled))
			{
				Section.AddMenuEntry(NarrativeGraphCommands.EnableNode);
			}
			if (CanSetSignalMode(ENarrativeSignalMode::Disabled))
			{
				Section.AddMenuEntry(NarrativeGraphCommands.DisableNode);
			}
			if (CanSetSignalMode(ENarrativeSignalMode::PassThrough))
			{
				Section.AddMenuEntry(NarrativeGraphCommands.SetPassThrough);
			}
		}

		{
			FToolMenuSection& Section = Menu->AddSection("NarrativeGraphNodeJumps", LOCTEXT("NodeJumpsMenuHeader", "Jumps"));
			if (CanFocusViewport())
			{
				Section.AddMenuEntry(NarrativeGraphCommands.FocusViewport);
			}
			if (CanJumpToDefinition())
			{
				Section.AddMenuEntry(NarrativeGraphCommands.JumpToNodeDefinition);
			}
		}

		{
			FToolMenuSection& Section = Menu->AddSection("NarrativeGraphNodeOrganisation", LOCTEXT("NodeOrganisation", "Organisation"));
			Section.AddSubMenu("Alignment", LOCTEXT("AlignmentHeader", "Alignment"), FText(), FNewToolMenuDelegate::CreateLambda([](UToolMenu* SubMenu)
			{
				FToolMenuSection& SubMenuSection = SubMenu->AddSection("EdGraphSchemaAlignment", LOCTEXT("AlignHeader", "Align"));
				SubMenuSection.AddMenuEntry(FGraphEditorCommands::Get().AlignNodesTop);
				SubMenuSection.AddMenuEntry(FGraphEditorCommands::Get().AlignNodesMiddle);
				SubMenuSection.AddMenuEntry(FGraphEditorCommands::Get().AlignNodesBottom);
				SubMenuSection.AddMenuEntry(FGraphEditorCommands::Get().AlignNodesLeft);
				SubMenuSection.AddMenuEntry(FGraphEditorCommands::Get().AlignNodesCenter);
				SubMenuSection.AddMenuEntry(FGraphEditorCommands::Get().AlignNodesRight);
				SubMenuSection.AddMenuEntry(FGraphEditorCommands::Get().StraightenConnections);
			}));
		}
	}
}

bool UNarrativeGraphNode::CanUserDeleteNode() const
{
	return NarrativeNode ? NarrativeNode->bCanDelete : Super::CanUserDeleteNode();
}

bool UNarrativeGraphNode::CanDuplicateNode() const
{
	return NarrativeNode ? NarrativeNode->bCanDuplicate : Super::CanDuplicateNode();
}

TSharedPtr<SGraphNode> UNarrativeGraphNode::CreateVisualWidget()
{
	return SNew(SNarrativeGraphNode, this);
}

FText UNarrativeGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (NarrativeNode)
	{
		if (UNarrativeGraphEditorSettings::Get()->bShowNodeClass)
		{
			FString CleanAssetName;
			if (NarrativeNode->GetClass()->ClassGeneratedBy)
			{
				NarrativeNode->GetClass()->GetPathName(nullptr, CleanAssetName);
				const int32 SubStringIdx = CleanAssetName.Find(".", ESearchCase::IgnoreCase, ESearchDir::FromEnd);
				CleanAssetName.LeftInline(SubStringIdx);
			}
			else
			{
				CleanAssetName = NarrativeNode->GetClass()->GetName();
			}

			FFormatNamedArguments Args;
			Args.Add(TEXT("NodeTitle"), NarrativeNode->GetNodeTitle());
			Args.Add(TEXT("AssetName"), FText::FromString(CleanAssetName));
			return FText::Format(INVTEXT("{NodeTitle}\n{AssetName}"), Args);
		}

		return NarrativeNode->GetNodeTitle();
	}

	return Super::GetNodeTitle(TitleType);
}

FLinearColor UNarrativeGraphNode::GetNodeTitleColor() const
{
	if (NarrativeNode)
	{
		FLinearColor DynamicColor;
		if (NarrativeNode->GetDynamicTitleColor(DynamicColor))
		{
			return DynamicColor;
		}

		UNarrativeGraphSettings* GraphSettings = UNarrativeGraphSettings::Get();
		if (const FLinearColor* NodeSpecificColor = GraphSettings->NodeSpecificColors.Find(NarrativeNode->GetClass()))
		{
			return *NodeSpecificColor;
		}
		if (const FLinearColor* StyleColor = GraphSettings->NodeTitleColors.Find(NarrativeNode->GetNodeStyle()))
		{
			return *StyleColor;
		}
	}

	return Super::GetNodeTitleColor();
}

FSlateIcon UNarrativeGraphNode::GetIconAndTint(FLinearColor& OutColor) const
{
	return FSlateIcon();
}

FText UNarrativeGraphNode::GetTooltipText() const
{
	FText Tooltip;
	if (NarrativeNode)
	{
		Tooltip = NarrativeNode->GetClass()->GetToolTipText();
	}
	if (Tooltip.IsEmpty())
	{
		Tooltip = GetNodeTitle(ENodeTitleType::ListView);
	}
	return Tooltip;
}

FString UNarrativeGraphNode::GetNodeDescription() const
{
	if (NarrativeNode && (GEditor->PlayWorld == nullptr || UNarrativeGraphEditorSettings::Get()->bShowNodeDescriptionWhilePlaying))
	{
		return NarrativeNode->GetNodeDescription();
	}

	return FString();
}

UNarrativeNode* UNarrativeGraphNode::GetInspectedNodeInstance() const
{
	return NarrativeNode ? NarrativeNode->GetInspectedInstance() : nullptr;
}

ENarrativeNodeState UNarrativeGraphNode::GetActivationState() const
{
	if (NarrativeNode)
	{
		if (const UNarrativeNode* NodeInstance = NarrativeNode->GetInspectedInstance())
		{
			return NodeInstance->GetActivationState();
		}
	}

	return ENarrativeNodeState::NeverActivated;
}

FString UNarrativeGraphNode::GetStatusString() const
{
	if (NarrativeNode)
	{
		if (const UNarrativeNode* NodeInstance = NarrativeNode->GetInspectedInstance())
		{
			return NodeInstance->GetStatusString();
		}
	}

	return FString();
}

FLinearColor UNarrativeGraphNode::GetStatusBackgroundColor() const
{
	if (NarrativeNode)
	{
		if (const UNarrativeNode* NodeInstance = NarrativeNode->GetInspectedInstance())
		{
			FLinearColor ObtainedColor;
			if (NodeInstance->GetStatusBackgroundColor(ObtainedColor))
			{
				return ObtainedColor;
			}
		}
	}

	return UNarrativeGraphSettings::Get()->NodeStatusBackground;
}

bool UNarrativeGraphNode::IsContentPreloaded() const
{
	if (NarrativeNode)
	{
		if (const UNarrativeNode* NodeInstance = NarrativeNode->GetInspectedInstance())
		{
			return NodeInstance->bPreloaded;
		}
	}

	return false;
}

bool UNarrativeGraphNode::CanFocusViewport() const
{
	return NarrativeNode ? (GEditor->bIsSimulatingInEditor && NarrativeNode->GetActorToFocus()) : false;
}

bool UNarrativeGraphNode::CanJumpToDefinition() const
{
	return NarrativeNode != nullptr;
}

void UNarrativeGraphNode::JumpToDefinition() const
{
	if (NarrativeNode)
	{
		if (NarrativeNode->GetClass()->IsNative())
		{
			if (FSourceCodeNavigation::CanNavigateToClass(NarrativeNode->GetClass()))
			{
				const bool bSucceeded = FSourceCodeNavigation::NavigateToClass(NarrativeNode->GetClass());
				if (bSucceeded)
				{
					return;
				}
			}

			// Failing that, fall back to the older method which will still get the file open assuming it exists
			FString NativeParentClassHeaderPath;
			const bool bFileFound = FSourceCodeNavigation::FindClassHeaderPath(NarrativeNode->GetClass(), NativeParentClassHeaderPath) && (IFileManager::Get().FileSize(*NativeParentClassHeaderPath) != INDEX_NONE);
			if (bFileFound)
			{
				const FString AbsNativeParentClassHeaderPath = FPaths::ConvertRelativePathToFull(NativeParentClassHeaderPath);
				FSourceCodeNavigation::OpenSourceFile(AbsNativeParentClassHeaderPath);
			}
		}
		else
		{
			FKismetEditorUtilities::BringKismetToFocusAttentionOnObject(NarrativeNode->GetClass());
		}
	}
}

void UNarrativeGraphNode::CreateInputPin(const FNarrativePin& NarrativePin, const int32 Index /*= INDEX_NONE*/)
{
	if (NarrativePin.PinName.IsNone())
	{
		return;
	}

	const FEdGraphPinType PinType = FEdGraphPinType(UEdGraphSchema_K2::PC_Exec, FName(NAME_None), nullptr, EPinContainerType::None, false, FEdGraphTerminalType());
	UEdGraphPin* NewPin = CreatePin(EGPD_Input, PinType, NarrativePin.PinName, Index);
	check(NewPin);

	if (!NarrativePin.PinFriendlyName.IsEmpty())
	{
		NewPin->bAllowFriendlyName = true;
		NewPin->PinFriendlyName = NarrativePin.PinFriendlyName;
	}

	NewPin->PinToolTip = NarrativePin.PinToolTip;

	InputPins.Emplace(NewPin);
}

void UNarrativeGraphNode::CreateOutputPin(const FNarrativePin& NarrativePin, const int32 Index /*= INDEX_NONE*/)
{
	if (NarrativePin.PinName.IsNone())
	{
		return;
	}

	const FEdGraphPinType PinType = FEdGraphPinType(UEdGraphSchema_K2::PC_Exec, FName(NAME_None), nullptr, EPinContainerType::None, false, FEdGraphTerminalType());
	UEdGraphPin* NewPin = CreatePin(EGPD_Output, PinType, NarrativePin.PinName, Index);
	check(NewPin);

	if (!NarrativePin.PinFriendlyName.IsEmpty())
	{
		NewPin->bAllowFriendlyName = true;
		NewPin->PinFriendlyName = NarrativePin.PinFriendlyName;
	}

	NewPin->PinToolTip = NarrativePin.PinToolTip;

	OutputPins.Emplace(NewPin);
}

void UNarrativeGraphNode::RemoveOrphanedPin(UEdGraphPin* Pin)
{
	const FScopedTransaction Transaction(LOCTEXT("RemoveOrphanedPin", "Remove Orphaned Pin"));
	Modify();

	PinBreakpoints.Remove(Pin);

	Pin->MarkAsGarbage();
	Pins.Remove(Pin);

	ReconstructNode();
	GetGraph()->NotifyGraphChanged();
}

bool UNarrativeGraphNode::SupportsContextPins() const
{
	return NarrativeNode && NarrativeNode->SupportsContextPins();
}

bool UNarrativeGraphNode::CanUserAddInput() const
{
	return NarrativeNode && NarrativeNode->CanUserAddInput() && InputPins.Num() < 256;
}

bool UNarrativeGraphNode::CanUserAddOutput() const
{
	return NarrativeNode && NarrativeNode->CanUserAddOutput() && OutputPins.Num() < 256;
}

bool UNarrativeGraphNode::CanUserRemoveInput(const UEdGraphPin* Pin) const
{
	return NarrativeNode && !NarrativeNode->GetClass()->GetDefaultObject<UNarrativeNode>()->InputPins.Contains(Pin->PinName);
}

bool UNarrativeGraphNode::CanUserRemoveOutput(const UEdGraphPin* Pin) const
{
	return NarrativeNode && !NarrativeNode->GetClass()->GetDefaultObject<UNarrativeNode>()->OutputPins.Contains(Pin->PinName);
}

void UNarrativeGraphNode::AddUserInput()
{
	AddInstancePin(EGPD_Input, NarrativeNode->CountNumberedInputs());
}

void UNarrativeGraphNode::AddUserOutput()
{
	AddInstancePin(EGPD_Output, NarrativeNode->CountNumberedOutputs());
}

void UNarrativeGraphNode::AddInstancePin(const EEdGraphPinDirection Direction, const uint8 NumberedPinsAmount)
{
	const FScopedTransaction Transaction(LOCTEXT("AddInstancePin", "Add Instance Pin"));
	Modify();

	const FNarrativePin PinName = FNarrativePin(FString::FromInt(NumberedPinsAmount));

	if (Direction == EGPD_Input)
	{
		if (NarrativeNode->InputPins.IsValidIndex(NumberedPinsAmount))
		{
			NarrativeNode->InputPins.Insert(PinName, NumberedPinsAmount);
		}
		else
		{
			NarrativeNode->InputPins.Add(PinName);
		}

		CreateInputPin(PinName, NumberedPinsAmount);
	}
	else
	{
		if (NarrativeNode->OutputPins.IsValidIndex(NumberedPinsAmount))
		{
			NarrativeNode->OutputPins.Insert(PinName, NumberedPinsAmount);
		}
		else
		{
			NarrativeNode->OutputPins.Add(PinName);
		}

		CreateOutputPin(PinName, NarrativeNode->InputPins.Num() + NumberedPinsAmount);
	}

	GetGraph()->NotifyGraphChanged();
}

void UNarrativeGraphNode::RemoveInstancePin(UEdGraphPin* Pin)
{
	const FScopedTransaction Transaction(LOCTEXT("RemoveInstancePin", "Remove Instance Pin"));
	Modify();

	PinBreakpoints.Remove(Pin);

	if (Pin->Direction == EGPD_Input)
	{
		if (InputPins.Contains(Pin))
		{
			InputPins.Remove(Pin);
			NarrativeNode->RemoveUserInput(Pin->PinName);

			Pin->MarkAsGarbage();
			Pins.Remove(Pin);
		}
	}
	else
	{
		if (OutputPins.Contains(Pin))
		{
			OutputPins.Remove(Pin);
			NarrativeNode->RemoveUserOutput(Pin->PinName);

			Pin->MarkAsGarbage();
			Pins.Remove(Pin);
		}
	}

	ReconstructNode();
	GetGraph()->NotifyGraphChanged();
}

void UNarrativeGraphNode::RefreshContextPins(const bool bReconstructNode)
{
	if (SupportsContextPins())
	{
		const FScopedTransaction Transaction(LOCTEXT("RefreshContextPins", "Refresh Context Pins"));
		Modify();

		const UNarrativeNode* NodeDefaults = NarrativeNode->GetClass()->GetDefaultObject<UNarrativeNode>();

		// recreate inputs
		NarrativeNode->InputPins = NodeDefaults->InputPins;
		NarrativeNode->AddInputPins(NarrativeNode->GetContextInputs());

		// recreate outputs
		NarrativeNode->OutputPins = NodeDefaults->OutputPins;
		NarrativeNode->AddOutputPins(NarrativeNode->GetContextOutputs());

		if (bReconstructNode)
		{
			ReconstructNode();
			GetGraph()->NotifyGraphChanged();
		}
	}
}

void UNarrativeGraphNode::GetPinHoverText(const UEdGraphPin& Pin, FString& HoverTextOut) const
{
	// start with the default hover text (from the pin's tool-tip)
	Super::GetPinHoverText(Pin, HoverTextOut);

	// add information on pin activations
	if (GEditor->PlayWorld)
	{
		if (const UNarrativeNode* InspectedNodeInstance = GetInspectedNodeInstance())
		{
			if (!HoverTextOut.IsEmpty())
			{
				HoverTextOut.Append(LINE_TERMINATOR).Append(LINE_TERMINATOR);
			}

			const TArray<FPinRecord>& PinRecords = InspectedNodeInstance->GetPinRecords(Pin.PinName, Pin.Direction);
			if (PinRecords.Num() == 0)
			{
				HoverTextOut.Append(FPinRecord::NoActivations);
			}
			else
			{
				HoverTextOut.Append(FPinRecord::PinActivations);
				for (int32 i = 0; i < PinRecords.Num(); i++)
				{
					HoverTextOut.Append(LINE_TERMINATOR);
					HoverTextOut.Appendf(TEXT("%d) %s"), i + 1, *PinRecords[i].HumanReadableTime);

					switch (PinRecords[i].ActivationType)
					{
						case ENarrativePinActivationType::Default:
							break;
						case ENarrativePinActivationType::Forced:
							HoverTextOut.Append(FPinRecord::ForcedActivation);
							break;
						case ENarrativePinActivationType::PassThrough:
							HoverTextOut.Append(FPinRecord::PassThroughActivation);
							break;
						default: ;
					}
				}
			}
		}
	}
}

void UNarrativeGraphNode::OnInputTriggered(const int32 Index)
{
	if (InputPins.IsValidIndex(Index) && PinBreakpoints.Contains(InputPins[Index]))
	{
		PinBreakpoints[InputPins[Index]].MarkAsHit();
		TryPausingSession(true);
	}

	TryPausingSession(false);
}

void UNarrativeGraphNode::OnOutputTriggered(const int32 Index)
{
	if (OutputPins.IsValidIndex(Index) && PinBreakpoints.Contains(OutputPins[Index]))
	{
		PinBreakpoints[OutputPins[Index]].MarkAsHit();
		TryPausingSession(true);
	}

	TryPausingSession(false);
}

void UNarrativeGraphNode::TryPausingSession(bool bPauseSession)
{
	// Node breakpoints waits on any pin triggered
	if (NodeBreakpoint.IsEnabled())
	{
		NodeBreakpoint.MarkAsHit();
		bPauseSession = true;
	}

	if (bPauseSession)
	{
		FEditorDelegates::ResumePIE.AddUObject(this, &UNarrativeGraphNode::OnResumePIE);
		FEditorDelegates::EndPIE.AddUObject(this, &UNarrativeGraphNode::OnEndPIE);

		UNarrativeDebuggerSubsystem::PausePlaySession();
	}
}

void UNarrativeGraphNode::OnResumePIE(const bool bIsSimulating)
{
	ResetBreakpoints();
}

void UNarrativeGraphNode::OnEndPIE(const bool bIsSimulating)
{
	ResetBreakpoints();
}

void UNarrativeGraphNode::ResetBreakpoints()
{
	FEditorDelegates::ResumePIE.RemoveAll(this);
	FEditorDelegates::EndPIE.RemoveAll(this);

	NodeBreakpoint.ResetHit();
	for (TPair<FEdGraphPinReference, FNarrativePinTrait>& PinBreakpoint : PinBreakpoints)
	{
		PinBreakpoint.Value.ResetHit();
	}
}

void UNarrativeGraphNode::ForcePinActivation(const FEdGraphPinReference PinReference) const
{
	UNarrativeNode* InspectedNodeInstance = GetInspectedNodeInstance();
	if (InspectedNodeInstance == nullptr)
	{
		return;
	}

	if (const UEdGraphPin* FoundPin = PinReference.Get())
	{
		switch (FoundPin->Direction)
		{
			case EGPD_Input:
				InspectedNodeInstance->TriggerInput(FoundPin->PinName, ENarrativePinActivationType::Forced);
				break;
			case EGPD_Output:
				InspectedNodeInstance->TriggerOutput(FoundPin->PinName, false, ENarrativePinActivationType::Forced);
				break;
			default: ;
		}
	}
}

void UNarrativeGraphNode::SetSignalMode(const ENarrativeSignalMode Mode)
{
	if (NarrativeNode)
	{
		NarrativeNode->SignalMode = Mode;
		OnSignalModeChanged.ExecuteIfBound();
	}
}

ENarrativeSignalMode UNarrativeGraphNode::GetSignalMode() const
{
	return NarrativeNode ? NarrativeNode->SignalMode : ENarrativeSignalMode::Disabled;
}

bool UNarrativeGraphNode::CanSetSignalMode(const ENarrativeSignalMode Mode) const
{
	return NarrativeNode ? (NarrativeNode->AllowedSignalModes.Contains(Mode) && NarrativeNode->SignalMode != Mode) : false;
}

#undef LOCTEXT_NAMESPACE
