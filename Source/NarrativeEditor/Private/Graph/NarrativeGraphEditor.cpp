// Copyright XiaoYao

#include "Graph/NarrativeGraphEditor.h"

#include "NarrativeEditorCommands.h"

#include "Asset/NarrativeAssetEditor.h"
#include "Asset/NarrativeDebuggerSubsystem.h"
#include "Graph/NarrativeGraphEditorSettings.h"
#include "Graph/NarrativeGraphSchema_Actions.h"
#include "Graph/Nodes/NarrativeGraphNode.h"

#include "Nodes/Route/NarrativeNode_SubGraph.h"

#include "EdGraphUtilities.h"
#include "Framework/Commands/GenericCommands.h"
#include "GraphEditorActions.h"
#include "HAL/PlatformApplicationMisc.h"
#include "IDetailsView.h"
#include "LevelEditor.h"
#include "Modules/ModuleManager.h"
#include "ScopedTransaction.h"
#include "SNodePanel.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "NarrativeGraphEditor"

void SNarrativeGraphEditor::Construct(const FArguments& InArgs, const TSharedPtr<FNarrativeAssetEditor> InAssetEditor)
{
	NarrativeAssetEditor = InAssetEditor;
	NarrativeAsset = NarrativeAssetEditor.Pin()->GetNarrativeAsset();

	DetailsView = InArgs._DetailsView;

	BindGraphCommands();

	SGraphEditor::FArguments Arguments;
	Arguments._AdditionalCommands = CommandList;
	Arguments._Appearance = GetGraphAppearanceInfo();
	Arguments._GraphToEdit = NarrativeAsset->GetGraph();
	Arguments._GraphEvents = InArgs._GraphEvents;
	Arguments._AutoExpandActionMenu = true;
	Arguments._GraphEvents.OnSelectionChanged = FOnSelectionChanged::CreateSP(this, &SNarrativeGraphEditor::OnSelectedNodesChanged);
	Arguments._GraphEvents.OnNodeDoubleClicked = FSingleNodeEvent::CreateSP(this, &SNarrativeGraphEditor::OnNodeDoubleClicked);
	Arguments._GraphEvents.OnTextCommitted = FOnNodeTextCommitted::CreateSP(this, &SNarrativeGraphEditor::OnNodeTitleCommitted);
	Arguments._GraphEvents.OnSpawnNodeByShortcut = FOnSpawnNodeByShortcut::CreateStatic(&SNarrativeGraphEditor::OnSpawnGraphNodeByShortcut, static_cast<UEdGraph*>(NarrativeAsset->GetGraph()));

	SGraphEditor::Construct(Arguments);
}

void SNarrativeGraphEditor::BindGraphCommands()
{
	FGraphEditorCommands::Register();
	FNarrativeGraphCommands::Register();
	FNarrativeSpawnNodeCommands::Register();

	const FGenericCommands& GenericCommands = FGenericCommands::Get();
	const FGraphEditorCommandsImpl& GraphEditorCommands = FGraphEditorCommands::Get();
	const FNarrativeGraphCommands& NarrativeGraphCommands = FNarrativeGraphCommands::Get();

	CommandList = MakeShareable(new FUICommandList);

	// Graph commands
	CommandList->MapAction(GraphEditorCommands.CreateComment,
	                               FExecuteAction::CreateSP(this, &SNarrativeGraphEditor::OnCreateComment),
	                               FCanExecuteAction::CreateStatic(&SNarrativeGraphEditor::CanEdit));

	CommandList->MapAction(GraphEditorCommands.StraightenConnections,
	                               FExecuteAction::CreateSP(this, &SNarrativeGraphEditor::OnStraightenConnections));

	// Generic Node commands
	CommandList->MapAction(GenericCommands.Undo,
	                               FExecuteAction::CreateStatic(&SNarrativeGraphEditor::UndoGraphAction),
	                               FCanExecuteAction::CreateStatic(&SNarrativeGraphEditor::CanEdit));

	CommandList->MapAction(GenericCommands.Redo,
	                               FExecuteAction::CreateStatic(&SNarrativeGraphEditor::RedoGraphAction),
	                               FCanExecuteAction::CreateStatic(&SNarrativeGraphEditor::CanEdit));

	CommandList->MapAction(GenericCommands.SelectAll,
	                               FExecuteAction::CreateSP(this, &SNarrativeGraphEditor::SelectAllNodes),
	                               FCanExecuteAction::CreateSP(this, &SNarrativeGraphEditor::CanSelectAllNodes));

	CommandList->MapAction(GenericCommands.Delete,
	                               FExecuteAction::CreateSP(this, &SNarrativeGraphEditor::DeleteSelectedNodes),
	                               FCanExecuteAction::CreateSP(this, &SNarrativeGraphEditor::CanDeleteNodes));

	CommandList->MapAction(GenericCommands.Copy,
	                               FExecuteAction::CreateSP(this, &SNarrativeGraphEditor::CopySelectedNodes),
	                               FCanExecuteAction::CreateSP(this, &SNarrativeGraphEditor::CanCopyNodes));

	CommandList->MapAction(GenericCommands.Cut,
	                               FExecuteAction::CreateSP(this, &SNarrativeGraphEditor::CutSelectedNodes),
	                               FCanExecuteAction::CreateSP(this, &SNarrativeGraphEditor::CanCutNodes));

	CommandList->MapAction(GenericCommands.Paste,
	                               FExecuteAction::CreateSP(this, &SNarrativeGraphEditor::PasteNodes),
	                               FCanExecuteAction::CreateSP(this, &SNarrativeGraphEditor::CanPasteNodes));

	CommandList->MapAction(GenericCommands.Duplicate,
	                               FExecuteAction::CreateSP(this, &SNarrativeGraphEditor::DuplicateNodes),
	                               FCanExecuteAction::CreateSP(this, &SNarrativeGraphEditor::CanDuplicateNodes));

	// Pin commands
	CommandList->MapAction(NarrativeGraphCommands.RefreshContextPins,
	                               FExecuteAction::CreateSP(this, &SNarrativeGraphEditor::RefreshContextPins),
	                               FCanExecuteAction::CreateSP(this, &SNarrativeGraphEditor::CanRefreshContextPins));

	CommandList->MapAction(NarrativeGraphCommands.AddInput,
	                               FExecuteAction::CreateSP(this, &SNarrativeGraphEditor::AddInput),
	                               FCanExecuteAction::CreateSP(this, &SNarrativeGraphEditor::CanAddInput));

	CommandList->MapAction(NarrativeGraphCommands.AddOutput,
	                               FExecuteAction::CreateSP(this, &SNarrativeGraphEditor::AddOutput),
	                               FCanExecuteAction::CreateSP(this, &SNarrativeGraphEditor::CanAddOutput));

	CommandList->MapAction(NarrativeGraphCommands.RemovePin,
	                               FExecuteAction::CreateSP(this, &SNarrativeGraphEditor::RemovePin),
	                               FCanExecuteAction::CreateSP(this, &SNarrativeGraphEditor::CanRemovePin));

	// Breakpoint commands
	CommandList->MapAction(GraphEditorCommands.AddBreakpoint,
	                               FExecuteAction::CreateSP(this, &SNarrativeGraphEditor::OnAddBreakpoint),
	                               FCanExecuteAction::CreateSP(this, &SNarrativeGraphEditor::CanAddBreakpoint),
	                               FIsActionChecked(),
	                               FIsActionButtonVisible::CreateSP(this, &SNarrativeGraphEditor::CanAddBreakpoint)
	);

	CommandList->MapAction(GraphEditorCommands.RemoveBreakpoint,
	                               FExecuteAction::CreateSP(this, &SNarrativeGraphEditor::OnRemoveBreakpoint),
	                               FCanExecuteAction::CreateSP(this, &SNarrativeGraphEditor::CanRemoveBreakpoint),
	                               FIsActionChecked(),
	                               FIsActionButtonVisible::CreateSP(this, &SNarrativeGraphEditor::CanRemoveBreakpoint)
	);

	CommandList->MapAction(GraphEditorCommands.EnableBreakpoint,
	                               FExecuteAction::CreateSP(this, &SNarrativeGraphEditor::OnEnableBreakpoint),
	                               FCanExecuteAction::CreateSP(this, &SNarrativeGraphEditor::CanEnableBreakpoint),
	                               FIsActionChecked(),
	                               FIsActionButtonVisible::CreateSP(this, &SNarrativeGraphEditor::CanEnableBreakpoint)
	);

	CommandList->MapAction(GraphEditorCommands.DisableBreakpoint,
	                               FExecuteAction::CreateSP(this, &SNarrativeGraphEditor::OnDisableBreakpoint),
	                               FCanExecuteAction::CreateSP(this, &SNarrativeGraphEditor::CanDisableBreakpoint),
	                               FIsActionChecked(),
	                               FIsActionButtonVisible::CreateSP(this, &SNarrativeGraphEditor::CanDisableBreakpoint)
	);

	CommandList->MapAction(GraphEditorCommands.ToggleBreakpoint,
	                               FExecuteAction::CreateSP(this, &SNarrativeGraphEditor::OnToggleBreakpoint),
	                               FCanExecuteAction::CreateSP(this, &SNarrativeGraphEditor::CanToggleBreakpoint),
	                               FIsActionChecked(),
	                               FIsActionButtonVisible::CreateSP(this, &SNarrativeGraphEditor::CanToggleBreakpoint)
	);

	// Pin Breakpoint commands
	CommandList->MapAction(NarrativeGraphCommands.AddPinBreakpoint,
	                               FExecuteAction::CreateSP(this, &SNarrativeGraphEditor::OnAddPinBreakpoint),
	                               FCanExecuteAction::CreateSP(this, &SNarrativeGraphEditor::CanAddPinBreakpoint),
	                               FIsActionChecked(),
	                               FIsActionButtonVisible::CreateSP(this, &SNarrativeGraphEditor::CanAddPinBreakpoint)
	);

	CommandList->MapAction(NarrativeGraphCommands.RemovePinBreakpoint,
	                               FExecuteAction::CreateSP(this, &SNarrativeGraphEditor::OnRemovePinBreakpoint),
	                               FCanExecuteAction::CreateSP(this, &SNarrativeGraphEditor::CanRemovePinBreakpoint),
	                               FIsActionChecked(),
	                               FIsActionButtonVisible::CreateSP(this, &SNarrativeGraphEditor::CanRemovePinBreakpoint)
	);

	CommandList->MapAction(NarrativeGraphCommands.EnablePinBreakpoint,
	                               FExecuteAction::CreateSP(this, &SNarrativeGraphEditor::OnEnablePinBreakpoint),
	                               FCanExecuteAction::CreateSP(this, &SNarrativeGraphEditor::CanEnablePinBreakpoint),
	                               FIsActionChecked(),
	                               FIsActionButtonVisible::CreateSP(this, &SNarrativeGraphEditor::CanEnablePinBreakpoint)
	);

	CommandList->MapAction(NarrativeGraphCommands.DisablePinBreakpoint,
	                               FExecuteAction::CreateSP(this, &SNarrativeGraphEditor::OnDisablePinBreakpoint),
	                               FCanExecuteAction::CreateSP(this, &SNarrativeGraphEditor::CanDisablePinBreakpoint),
	                               FIsActionChecked(),
	                               FIsActionButtonVisible::CreateSP(this, &SNarrativeGraphEditor::CanDisablePinBreakpoint)
	);

	CommandList->MapAction(NarrativeGraphCommands.TogglePinBreakpoint,
	                               FExecuteAction::CreateSP(this, &SNarrativeGraphEditor::OnTogglePinBreakpoint),
	                               FCanExecuteAction::CreateSP(this, &SNarrativeGraphEditor::CanTogglePinBreakpoint),
	                               FIsActionChecked(),
	                               FIsActionButtonVisible::CreateSP(this, &SNarrativeGraphEditor::CanTogglePinBreakpoint)
	);

	// Execution Override commands
	CommandList->MapAction(NarrativeGraphCommands.EnableNode,
	                               FExecuteAction::CreateSP(this, &SNarrativeGraphEditor::SetSignalMode, ENarrativeSignalMode::Enabled),
	                               FCanExecuteAction::CreateSP(this, &SNarrativeGraphEditor::CanSetSignalMode, ENarrativeSignalMode::Enabled),
	                               FIsActionChecked(),
	                               FIsActionButtonVisible::CreateSP(this, &SNarrativeGraphEditor::CanSetSignalMode, ENarrativeSignalMode::Enabled)
	);

	CommandList->MapAction(NarrativeGraphCommands.DisableNode,
	                               FExecuteAction::CreateSP(this, &SNarrativeGraphEditor::SetSignalMode, ENarrativeSignalMode::Disabled),
	                               FCanExecuteAction::CreateSP(this, &SNarrativeGraphEditor::CanSetSignalMode, ENarrativeSignalMode::Disabled),
	                               FIsActionChecked(),
	                               FIsActionButtonVisible::CreateSP(this, &SNarrativeGraphEditor::CanSetSignalMode, ENarrativeSignalMode::Disabled)
	);

	CommandList->MapAction(NarrativeGraphCommands.SetPassThrough,
	                               FExecuteAction::CreateSP(this, &SNarrativeGraphEditor::SetSignalMode, ENarrativeSignalMode::PassThrough),
	                               FCanExecuteAction::CreateSP(this, &SNarrativeGraphEditor::CanSetSignalMode, ENarrativeSignalMode::PassThrough),
	                               FIsActionChecked(),
	                               FIsActionButtonVisible::CreateSP(this, &SNarrativeGraphEditor::CanSetSignalMode, ENarrativeSignalMode::PassThrough)
	);

	CommandList->MapAction(NarrativeGraphCommands.ForcePinActivation,
	                               FExecuteAction::CreateSP(this, &SNarrativeGraphEditor::OnForcePinActivation),
	                               FCanExecuteAction::CreateStatic(&SNarrativeGraphEditor::IsPIE),
	                               FIsActionChecked(),
	                               FIsActionButtonVisible::CreateStatic(&SNarrativeGraphEditor::IsPIE)
	);

	// Jump commands
	CommandList->MapAction(NarrativeGraphCommands.FocusViewport,
	                               FExecuteAction::CreateSP(this, &SNarrativeGraphEditor::FocusViewport),
	                               FCanExecuteAction::CreateSP(this, &SNarrativeGraphEditor::CanFocusViewport));

	CommandList->MapAction(NarrativeGraphCommands.JumpToNodeDefinition,
	                               FExecuteAction::CreateSP(this, &SNarrativeGraphEditor::JumpToNodeDefinition),
	                               FCanExecuteAction::CreateSP(this, &SNarrativeGraphEditor::CanJumpToNodeDefinition));

	// Organisation Commands
	CommandList->MapAction(GraphEditorCommands.AlignNodesTop,
	                               FExecuteAction::CreateSP(this, &SNarrativeGraphEditor::OnAlignTop));

	CommandList->MapAction(GraphEditorCommands.AlignNodesMiddle,
	                               FExecuteAction::CreateSP(this, &SNarrativeGraphEditor::OnAlignMiddle));

	CommandList->MapAction(GraphEditorCommands.AlignNodesBottom,
	                               FExecuteAction::CreateSP(this, &SNarrativeGraphEditor::OnAlignBottom));

	CommandList->MapAction(GraphEditorCommands.AlignNodesLeft,
	                               FExecuteAction::CreateSP(this, &SNarrativeGraphEditor::OnAlignLeft));

	CommandList->MapAction(GraphEditorCommands.AlignNodesCenter,
	                               FExecuteAction::CreateSP(this, &SNarrativeGraphEditor::OnAlignCenter));

	CommandList->MapAction(GraphEditorCommands.AlignNodesRight,
	                               FExecuteAction::CreateSP(this, &SNarrativeGraphEditor::OnAlignRight));

	CommandList->MapAction(GraphEditorCommands.StraightenConnections,
	                               FExecuteAction::CreateSP(this, &SNarrativeGraphEditor::OnStraightenConnections));
}

FGraphAppearanceInfo SNarrativeGraphEditor::GetGraphAppearanceInfo() const
{
	FGraphAppearanceInfo AppearanceInfo;
	AppearanceInfo.CornerText = GetCornerText();

	if (UNarrativeDebuggerSubsystem::IsPlaySessionPaused())
	{
		AppearanceInfo.PIENotifyText = LOCTEXT("PausedLabel", "PAUSED");
	}

	return AppearanceInfo;
}

FText SNarrativeGraphEditor::GetCornerText() const
{
	return LOCTEXT("AppearanceCornerText_NarrativeAsset", "FLOW");
}

void SNarrativeGraphEditor::UndoGraphAction()
{
	GEditor->UndoTransaction();
}

void SNarrativeGraphEditor::RedoGraphAction()
{
	GEditor->RedoTransaction();
}

FReply SNarrativeGraphEditor::OnSpawnGraphNodeByShortcut(FInputChord InChord, const FVector2D& InPosition, UEdGraph* InGraph)
{
	UEdGraph* Graph = InGraph;

	if (FNarrativeSpawnNodeCommands::IsRegistered())
	{
		const TSharedPtr<FEdGraphSchemaAction> Action = FNarrativeSpawnNodeCommands::Get().GetActionByChord(InChord);
		if (Action.IsValid())
		{
			TArray<UEdGraphPin*> DummyPins;
			Action->PerformAction(Graph, DummyPins, InPosition);
			return FReply::Handled();
		}
	}

	return FReply::Unhandled();
}

void SNarrativeGraphEditor::OnCreateComment() const
{
	FNarrativeGraphSchemaAction_NewComment CommentAction;
	CommentAction.PerformAction(NarrativeAsset->GetGraph(), nullptr, GetPasteLocation());
}

bool SNarrativeGraphEditor::CanEdit()
{
	return GEditor->PlayWorld == nullptr;
}

bool SNarrativeGraphEditor::IsPIE()
{
	return GEditor->PlayWorld != nullptr;
}

bool SNarrativeGraphEditor::IsTabFocused() const
{
	return NarrativeAssetEditor.Pin()->IsTabFocused(FNarrativeAssetEditor::GraphTab);
}

void SNarrativeGraphEditor::SelectSingleNode(UEdGraphNode* Node)
{
	ClearSelectionSet();
	SetNodeSelection(Node, true);
}

void SNarrativeGraphEditor::OnSelectedNodesChanged(const TSet<UObject*>& Nodes)
{
	TArray<UObject*> SelectedObjects;

	if (Nodes.Num() > 0)
	{
		NarrativeAssetEditor.Pin()->SetUISelectionState(FNarrativeAssetEditor::GraphTab);

		for (TSet<UObject*>::TConstIterator SetIt(Nodes); SetIt; ++SetIt)
		{
			if (const UNarrativeGraphNode* GraphNode = Cast<UNarrativeGraphNode>(*SetIt))
			{
				SelectedObjects.Add(Cast<UObject>(GraphNode->GetNarrativeNode()));
			}
			else
			{
				SelectedObjects.Add(*SetIt);
			}
		}
	}
	else
	{
		NarrativeAssetEditor.Pin()->SetUISelectionState(NAME_None);
		SelectedObjects.Add(NarrativeAsset.Get());
	}

	if (DetailsView.IsValid())
	{
		DetailsView->SetObjects(SelectedObjects);
	}

	OnSelectionChangedEvent.ExecuteIfBound(Nodes);
}

TSet<UNarrativeGraphNode*> SNarrativeGraphEditor::GetSelectedNarrativeNodes() const
{
	TSet<UNarrativeGraphNode*> Result;

	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		if (UNarrativeGraphNode* SelectedNode = Cast<UNarrativeGraphNode>(*NodeIt))
		{
			Result.Emplace(SelectedNode);
		}
	}

	return Result;
}

void SNarrativeGraphEditor::DeleteSelectedNodes()
{
	const FScopedTransaction Transaction(LOCTEXT("DeleteSelectedNode", "Delete Selected Node"));
	GetCurrentGraph()->Modify();
	NarrativeAsset->Modify();

	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	NarrativeAssetEditor.Pin()->SetUISelectionState(NAME_None);

	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UEdGraphNode* Node = CastChecked<UEdGraphNode>(*NodeIt);
		if (Node->CanUserDeleteNode())
		{
			if (const UNarrativeGraphNode* NarrativeGraphNode = Cast<UNarrativeGraphNode>(Node))
			{
				if (NarrativeGraphNode->GetNarrativeNode())
				{
					const FGuid NodeGuid = NarrativeGraphNode->GetNarrativeNode()->GetGuid();

					GetCurrentGraph()->GetSchema()->BreakNodeLinks(*Node);
					Node->DestroyNode();

					NarrativeAsset->UnregisterNode(NodeGuid);
					continue;
				}
			}

			GetCurrentGraph()->GetSchema()->BreakNodeLinks(*Node);
			Node->DestroyNode();
		}
	}
}

void SNarrativeGraphEditor::DeleteSelectedDuplicableNodes()
{
	// Cache off the old selection
	const FGraphPanelSelectionSet OldSelectedNodes = GetSelectedNodes();

	// Clear the selection and only select the nodes that can be duplicated
	FGraphPanelSelectionSet RemainingNodes;
	ClearSelectionSet();

	for (FGraphPanelSelectionSet::TConstIterator SelectedIt(OldSelectedNodes); SelectedIt; ++SelectedIt)
	{
		if (UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIt))
		{
			if (Node->CanDuplicateNode())
			{
				SetNodeSelection(Node, true);
			}
			else
			{
				RemainingNodes.Add(Node);
			}
		}
	}

	// Delete the duplicable nodes
	DeleteSelectedNodes();

	for (FGraphPanelSelectionSet::TConstIterator SelectedIt(RemainingNodes); SelectedIt; ++SelectedIt)
	{
		if (UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIt))
		{
			SetNodeSelection(Node, true);
		}
	}
}

bool SNarrativeGraphEditor::CanDeleteNodes() const
{
	if (CanEdit() && IsTabFocused())
	{
		const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
		for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
		{
			if (const UEdGraphNode* Node = Cast<UEdGraphNode>(*NodeIt))
			{
				if (!Node->CanUserDeleteNode())
				{
					return false;
				}
			}
		}

		return SelectedNodes.Num() > 0;
	}

	return false;
}

void SNarrativeGraphEditor::CutSelectedNodes()
{
	CopySelectedNodes();

	// Cut should only delete nodes that can be duplicated
	DeleteSelectedDuplicableNodes();
}

bool SNarrativeGraphEditor::CanCutNodes() const
{
	return CanCopyNodes() && CanDeleteNodes();
}

void SNarrativeGraphEditor::CopySelectedNodes() const
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator SelectedIt(SelectedNodes); SelectedIt; ++SelectedIt)
	{
		if (UNarrativeGraphNode* Node = Cast<UNarrativeGraphNode>(*SelectedIt))
		{
			Node->PrepareForCopying();
		}
	}

	// Export the selected nodes and place the text on the clipboard
	FString ExportedText;
	FEdGraphUtilities::ExportNodesToText(SelectedNodes, /*out*/ ExportedText);
	FPlatformApplicationMisc::ClipboardCopy(*ExportedText);

	for (FGraphPanelSelectionSet::TConstIterator SelectedIt(SelectedNodes); SelectedIt; ++SelectedIt)
	{
		if (UNarrativeGraphNode* Node = Cast<UNarrativeGraphNode>(*SelectedIt))
		{
			Node->PostCopyNode();
		}
	}
}

bool SNarrativeGraphEditor::CanCopyNodes() const
{
	if (CanEdit() && IsTabFocused())
	{
		const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
		for (FGraphPanelSelectionSet::TConstIterator SelectedIt(SelectedNodes); SelectedIt; ++SelectedIt)
		{
			const UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIt);
			if (Node && Node->CanDuplicateNode())
			{
				return true;
			}
		}
	}

	return false;
}

void SNarrativeGraphEditor::PasteNodes()
{
	PasteNodesHere(GetPasteLocation());
}

void SNarrativeGraphEditor::PasteNodesHere(const FVector2D& Location)
{
	NarrativeAssetEditor.Pin()->SetUISelectionState(NAME_None);

	// Undo/Redo support
	const FScopedTransaction Transaction(LOCTEXT("PasteNode", "Paste Node"));
	NarrativeAsset->GetGraph()->Modify();
	NarrativeAsset->Modify();

	// Clear the selection set (newly pasted stuff will be selected)
	ClearSelectionSet();

	// Grab the text to paste from the clipboard.
	FString TextToImport;
	FPlatformApplicationMisc::ClipboardPaste(TextToImport);

	// Import the nodes
	TSet<UEdGraphNode*> PastedNodes;
	FEdGraphUtilities::ImportNodesFromText(NarrativeAsset->GetGraph(), TextToImport, /*out*/ PastedNodes);

	//Average position of nodes so we can move them while still maintaining relative distances to each other
	FVector2D AvgNodePosition(0.0f, 0.0f);

	for (TSet<UEdGraphNode*>::TIterator It(PastedNodes); It; ++It)
	{
		const UEdGraphNode* Node = *It;
		AvgNodePosition.X += Node->NodePosX;
		AvgNodePosition.Y += Node->NodePosY;
	}

	if (PastedNodes.Num() > 0)
	{
		const float InvNumNodes = 1.0f / static_cast<float>(PastedNodes.Num());
		AvgNodePosition.X *= InvNumNodes;
		AvgNodePosition.Y *= InvNumNodes;
	}

	for (TSet<UEdGraphNode*>::TIterator It(PastedNodes); It; ++It)
	{
		UEdGraphNode* Node = *It;

		// Give new node a different Guid from the old one
		Node->CreateNewGuid();

		if (const UNarrativeGraphNode* NarrativeGraphNode = Cast<UNarrativeGraphNode>(Node))
		{
			NarrativeAsset->RegisterNode(Node->NodeGuid, NarrativeGraphNode->GetNarrativeNode());
		}

		// Select the newly pasted stuff
		SetNodeSelection(Node, true);

		Node->NodePosX = (Node->NodePosX - AvgNodePosition.X) + Location.X;
		Node->NodePosY = (Node->NodePosY - AvgNodePosition.Y) + Location.Y;

		Node->SnapToGrid(SNodePanel::GetSnapGridSize());
	}

	// Force new pasted NarrativeNodes to have same connections as graph nodes
	NarrativeAsset->HarvestNodeConnections();

	// Update UI
	NotifyGraphChanged();

	NarrativeAsset->PostEditChange();
	NarrativeAsset->MarkPackageDirty();
}

bool SNarrativeGraphEditor::CanPasteNodes() const
{
	if (CanEdit() && IsTabFocused())
	{
		FString ClipboardContent;
		FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);

		return FEdGraphUtilities::CanImportNodesFromText(NarrativeAsset->GetGraph(), ClipboardContent);
	}

	return false;
}

void SNarrativeGraphEditor::DuplicateNodes()
{
	CopySelectedNodes();
	PasteNodes();
}

bool SNarrativeGraphEditor::CanDuplicateNodes() const
{
	return CanCopyNodes();
}

void SNarrativeGraphEditor::OnNodeDoubleClicked(class UEdGraphNode* Node) const
{
	UNarrativeNode* NarrativeNode = Cast<UNarrativeGraphNode>(Node)->GetNarrativeNode();

	if (NarrativeNode)
	{
		if (UNarrativeGraphEditorSettings::Get()->NodeDoubleClickTarget == ENarrativeNodeDoubleClickTarget::NodeDefinition)
		{
			Node->JumpToDefinition();
		}
		else
		{
			const FString AssetPath = NarrativeNode->GetAssetPath();
			if (!AssetPath.IsEmpty())
			{
				GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(AssetPath);
			}
			else if (UObject* AssetToEdit = NarrativeNode->GetAssetToEdit())
			{
				GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(AssetToEdit);

				if (IsPIE())
				{
					if (UNarrativeNode_SubGraph* SubGraphNode = Cast<UNarrativeNode_SubGraph>(NarrativeNode))
					{
						const TWeakObjectPtr<UNarrativeAsset> SubNarrativeInstance = SubGraphNode->GetNarrativeAsset()->GetNarrativeInstance(SubGraphNode);
						if (SubNarrativeInstance.IsValid())
						{
							SubGraphNode->GetNarrativeAsset()->GetTemplateAsset()->SetInspectedInstance(SubNarrativeInstance->GetDisplayName());
						}
					}
				}
			}
			else if (UNarrativeGraphEditorSettings::Get()->NodeDoubleClickTarget == ENarrativeNodeDoubleClickTarget::PrimaryAssetOrNodeDefinition)
			{
				Node->JumpToDefinition();
			}
		}
	}
}

void SNarrativeGraphEditor::OnNodeTitleCommitted(const FText& NewText, ETextCommit::Type CommitInfo, UEdGraphNode* NodeBeingChanged)
{
	if (NodeBeingChanged)
	{
		const FScopedTransaction Transaction(LOCTEXT("RenameNode", "Rename Node"));
		NodeBeingChanged->Modify();
		NodeBeingChanged->OnRenameNode(NewText.ToString());
	}
}

void SNarrativeGraphEditor::RefreshContextPins() const
{
	for (UNarrativeGraphNode* SelectedNode : GetSelectedNarrativeNodes())
	{
		SelectedNode->RefreshContextPins(true);
	}
}

bool SNarrativeGraphEditor::CanRefreshContextPins() const
{
	if (CanEdit() && GetSelectedNarrativeNodes().Num() == 1)
	{
		for (const UNarrativeGraphNode* SelectedNode : GetSelectedNarrativeNodes())
		{
			return SelectedNode->SupportsContextPins();
		}
	}

	return false;
}

void SNarrativeGraphEditor::AddInput() const
{
	for (UNarrativeGraphNode* SelectedNode : GetSelectedNarrativeNodes())
	{
		SelectedNode->AddUserInput();
	}
}

bool SNarrativeGraphEditor::CanAddInput() const
{
	if (CanEdit() && GetSelectedNarrativeNodes().Num() == 1)
	{
		for (const UNarrativeGraphNode* SelectedNode : GetSelectedNarrativeNodes())
		{
			return SelectedNode->CanUserAddInput();
		}
	}

	return false;
}

void SNarrativeGraphEditor::AddOutput() const
{
	for (UNarrativeGraphNode* SelectedNode : GetSelectedNarrativeNodes())
	{
		SelectedNode->AddUserOutput();
	}
}

bool SNarrativeGraphEditor::CanAddOutput() const
{
	if (CanEdit() && GetSelectedNarrativeNodes().Num() == 1)
	{
		for (const UNarrativeGraphNode* SelectedNode : GetSelectedNarrativeNodes())
		{
			return SelectedNode->CanUserAddOutput();
		}
	}

	return false;
}

void SNarrativeGraphEditor::RemovePin()
{
	if (UEdGraphPin* SelectedPin = GetGraphPinForMenu())
	{
		if (UNarrativeGraphNode* SelectedNode = Cast<UNarrativeGraphNode>(SelectedPin->GetOwningNode()))
		{
			SelectedNode->RemoveInstancePin(SelectedPin);
		}
	}
}

bool SNarrativeGraphEditor::CanRemovePin()
{
	if (CanEdit() && GetSelectedNarrativeNodes().Num() == 1)
	{
		if (const UEdGraphPin* Pin = GetGraphPinForMenu())
		{
			if (const UNarrativeGraphNode* GraphNode = Cast<UNarrativeGraphNode>(Pin->GetOwningNode()))
			{
				if (Pin->Direction == EGPD_Input)
				{
					return GraphNode->CanUserRemoveInput(Pin);
				}
				else
				{
					return GraphNode->CanUserRemoveOutput(Pin);
				}
			}
		}
	}

	return false;
}

void SNarrativeGraphEditor::OnAddBreakpoint() const
{
	for (UNarrativeGraphNode* SelectedNode : GetSelectedNarrativeNodes())
	{
		SelectedNode->NodeBreakpoint.AllowTrait();
	}
}

void SNarrativeGraphEditor::OnAddPinBreakpoint()
{
	if (UEdGraphPin* Pin = GetGraphPinForMenu())
	{
		if (UNarrativeGraphNode* GraphNode = Cast<UNarrativeGraphNode>(Pin->GetOwningNode()))
		{
			GraphNode->PinBreakpoints.Add(Pin, FNarrativePinTrait(true));
		}
	}
}

bool SNarrativeGraphEditor::CanAddBreakpoint() const
{
	for (const UNarrativeGraphNode* SelectedNode : GetSelectedNarrativeNodes())
	{
		return !SelectedNode->NodeBreakpoint.IsAllowed();
	}

	return false;
}

bool SNarrativeGraphEditor::CanAddPinBreakpoint()
{
	if (UEdGraphPin* Pin = GetGraphPinForMenu())
	{
		if (UNarrativeGraphNode* GraphNode = Cast<UNarrativeGraphNode>(Pin->GetOwningNode()))
		{
			return !GraphNode->PinBreakpoints.Contains(Pin) || !GraphNode->PinBreakpoints[Pin].IsAllowed();
		}
	}

	return false;
}

void SNarrativeGraphEditor::OnRemoveBreakpoint() const
{
	for (UNarrativeGraphNode* SelectedNode : GetSelectedNarrativeNodes())
	{
		SelectedNode->NodeBreakpoint.DisallowTrait();
	}
}

void SNarrativeGraphEditor::OnRemovePinBreakpoint()
{
	if (UEdGraphPin* Pin = GetGraphPinForMenu())
	{
		if (UNarrativeGraphNode* GraphNode = Cast<UNarrativeGraphNode>(Pin->GetOwningNode()))
		{
			GraphNode->PinBreakpoints.Remove(Pin);
		}
	}
}

bool SNarrativeGraphEditor::CanRemoveBreakpoint() const
{
	for (const UNarrativeGraphNode* SelectedNode : GetSelectedNarrativeNodes())
	{
		return SelectedNode->NodeBreakpoint.IsAllowed();
	}

	return false;
}

bool SNarrativeGraphEditor::CanRemovePinBreakpoint()
{
	if (UEdGraphPin* Pin = GetGraphPinForMenu())
	{
		if (const UNarrativeGraphNode* GraphNode = Cast<UNarrativeGraphNode>(Pin->GetOwningNode()))
		{
			return GraphNode->PinBreakpoints.Contains(Pin);
		}
	}

	return false;
}

void SNarrativeGraphEditor::OnEnableBreakpoint() const
{
	for (UNarrativeGraphNode* SelectedNode : GetSelectedNarrativeNodes())
	{
		SelectedNode->NodeBreakpoint.EnableTrait();
	}
}

void SNarrativeGraphEditor::OnEnablePinBreakpoint()
{
	if (UEdGraphPin* Pin = GetGraphPinForMenu())
	{
		if (UNarrativeGraphNode* GraphNode = Cast<UNarrativeGraphNode>(Pin->GetOwningNode()))
		{
			GraphNode->PinBreakpoints[Pin].EnableTrait();
		}
	}
}

bool SNarrativeGraphEditor::CanEnableBreakpoint()
{
	if (UEdGraphPin* Pin = GetGraphPinForMenu())
	{
		if (const UNarrativeGraphNode* GraphNode = Cast<UNarrativeGraphNode>(Pin->GetOwningNode()))
		{
			return GraphNode->PinBreakpoints.Contains(Pin);
		}
	}

	for (const UNarrativeGraphNode* SelectedNode : GetSelectedNarrativeNodes())
	{
		return SelectedNode->NodeBreakpoint.CanEnable();
	}

	return false;
}

bool SNarrativeGraphEditor::CanEnablePinBreakpoint()
{
	if (UEdGraphPin* Pin = GetGraphPinForMenu())
	{
		if (UNarrativeGraphNode* GraphNode = Cast<UNarrativeGraphNode>(Pin->GetOwningNode()))
		{
			return GraphNode->PinBreakpoints.Contains(Pin) && GraphNode->PinBreakpoints[Pin].CanEnable();
		}
	}

	return false;
}

void SNarrativeGraphEditor::OnDisableBreakpoint() const
{
	for (UNarrativeGraphNode* SelectedNode : GetSelectedNarrativeNodes())
	{
		SelectedNode->NodeBreakpoint.DisableTrait();
	}
}

void SNarrativeGraphEditor::OnDisablePinBreakpoint()
{
	if (UEdGraphPin* Pin = GetGraphPinForMenu())
	{
		if (UNarrativeGraphNode* GraphNode = Cast<UNarrativeGraphNode>(Pin->GetOwningNode()))
		{
			GraphNode->PinBreakpoints[Pin].DisableTrait();
		}
	}
}

bool SNarrativeGraphEditor::CanDisableBreakpoint() const
{
	for (const UNarrativeGraphNode* SelectedNode : GetSelectedNarrativeNodes())
	{
		return SelectedNode->NodeBreakpoint.IsEnabled();
	}

	return false;
}

bool SNarrativeGraphEditor::CanDisablePinBreakpoint()
{
	if (UEdGraphPin* Pin = GetGraphPinForMenu())
	{
		if (UNarrativeGraphNode* GraphNode = Cast<UNarrativeGraphNode>(Pin->GetOwningNode()))
		{
			return GraphNode->PinBreakpoints.Contains(Pin) && GraphNode->PinBreakpoints[Pin].IsEnabled();
		}
	}

	return false;
}

void SNarrativeGraphEditor::OnToggleBreakpoint() const
{
	for (UNarrativeGraphNode* SelectedNode : GetSelectedNarrativeNodes())
	{
		SelectedNode->NodeBreakpoint.ToggleTrait();
	}
}

void SNarrativeGraphEditor::OnTogglePinBreakpoint()
{
	if (UEdGraphPin* Pin = GetGraphPinForMenu())
	{
		if (UNarrativeGraphNode* GraphNode = Cast<UNarrativeGraphNode>(Pin->GetOwningNode()))
		{
			GraphNode->PinBreakpoints.Add(Pin, FNarrativePinTrait());
			GraphNode->PinBreakpoints[Pin].ToggleTrait();
		}
	}
}

bool SNarrativeGraphEditor::CanToggleBreakpoint() const
{
	return GetSelectedNarrativeNodes().Num() > 0;
}

bool SNarrativeGraphEditor::CanTogglePinBreakpoint()
{
	return GetGraphPinForMenu() != nullptr;
}

void SNarrativeGraphEditor::SetSignalMode(const ENarrativeSignalMode Mode) const
{
	for (UNarrativeGraphNode* SelectedNode : GetSelectedNarrativeNodes())
	{
		SelectedNode->SetSignalMode(Mode);
	}

	NarrativeAsset->Modify();
}

bool SNarrativeGraphEditor::CanSetSignalMode(const ENarrativeSignalMode Mode) const
{
	if (IsPIE())
	{
		return false;
	}

	for (const UNarrativeGraphNode* SelectedNode : GetSelectedNarrativeNodes())
	{
		return SelectedNode->CanSetSignalMode(Mode);
	}

	return false;
}

void SNarrativeGraphEditor::OnForcePinActivation()
{
	if (UEdGraphPin* Pin = GetGraphPinForMenu())
	{
		if (const UNarrativeGraphNode* GraphNode = Cast<UNarrativeGraphNode>(Pin->GetOwningNode()))
		{
			GraphNode->ForcePinActivation(Pin);
		}
	}
}

void SNarrativeGraphEditor::FocusViewport() const
{
	// Iterator used but should only contain one node
	for (UNarrativeGraphNode* SelectedNode : GetSelectedNarrativeNodes())
	{
		const UNarrativeNode* NarrativeNode = Cast<UNarrativeGraphNode>(SelectedNode)->GetNarrativeNode();
		if (UNarrativeNode* NodeInstance = NarrativeNode->GetInspectedInstance())
		{
			if (AActor* ActorToFocus = NodeInstance->GetActorToFocus())
			{
				GEditor->SelectNone(false, false, false);
				GEditor->SelectActor(ActorToFocus, true, true, true);
				GEditor->NoteSelectionChange();

				GEditor->MoveViewportCamerasToActor(*ActorToFocus, false);

				const FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
				const TSharedPtr<SDockTab> LevelEditorTab = LevelEditorModule.GetLevelEditorInstanceTab().Pin();
				if (LevelEditorTab.IsValid())
				{
					LevelEditorTab->DrawAttention();
				}
			}
		}

		return;
	}
}

bool SNarrativeGraphEditor::CanFocusViewport() const
{
	return GetSelectedNarrativeNodes().Num() == 1;
}

void SNarrativeGraphEditor::JumpToNodeDefinition() const
{
	// Iterator used but should only contain one node
	for (const UNarrativeGraphNode* SelectedNode : GetSelectedNarrativeNodes())
	{
		SelectedNode->JumpToDefinition();
		return;
	}
}

bool SNarrativeGraphEditor::CanJumpToNodeDefinition() const
{
	return GetSelectedNarrativeNodes().Num() == 1;
}

#undef LOCTEXT_NAMESPACE
