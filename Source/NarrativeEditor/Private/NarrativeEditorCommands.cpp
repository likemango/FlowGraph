// Copyright XiaoYao

#include "NarrativeEditorCommands.h"
#include "NarrativeEditorStyle.h"
#include "Graph/NarrativeGraphSchema_Actions.h"

#include "Nodes/NarrativeNode.h"

#include "Misc/ConfigCacheIni.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "NarrativeGraphCommands"

FNarrativeToolbarCommands::FNarrativeToolbarCommands()
	: TCommands<FNarrativeToolbarCommands>("NarrativeToolbar", LOCTEXT("NarrativeToolbar", "Narrative Toobar"), NAME_None, FNarrativeEditorStyle::GetStyleSetName())
{
}

void FNarrativeToolbarCommands::RegisterCommands()
{
	UI_COMMAND(RefreshAsset, "Refresh", "Refresh asset and all nodes", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ValidateAsset, "Validate", "Validate asset and all nodes", EUserInterfaceActionType::Button, FInputChord());

	UI_COMMAND(SearchInAsset, "Search", "Search in the current Narrative Graph", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control | EModifierKey::Shift, EKeys::F));
	UI_COMMAND(EditAssetDefaults, "Asset Defaults", "Edit the NarrativeAsset default properties", EUserInterfaceActionType::Button, FInputChord());

	UI_COMMAND(GoToParentInstance, "Go To Parent", "Open editor for the Narrative Asset that created this Narrative instance", EUserInterfaceActionType::Button, FInputChord());
}

FNarrativeGraphCommands::FNarrativeGraphCommands()
	: TCommands<FNarrativeGraphCommands>("NarrativeGraph", LOCTEXT("NarrativeGraph", "Narrative Graph"), NAME_None, FAppStyle::GetAppStyleSetName())
{
}

void FNarrativeGraphCommands::RegisterCommands()
{
	UI_COMMAND(RefreshContextPins, "Refresh context pins", "Refresh pins generated from the context asset", EUserInterfaceActionType::Button, FInputChord());

	UI_COMMAND(AddInput, "Add Input", "Adds an input to the node", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(AddOutput, "Add Output", "Adds an output to the node", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(RemovePin, "Remove Pin", "Removes a pin from the node", EUserInterfaceActionType::Button, FInputChord());

	UI_COMMAND(AddPinBreakpoint, "Add Pin Breakpoint", "Adds a breakpoint to the pin", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(RemovePinBreakpoint, "Remove Pin Breakpoint", "Removes a breakpoint from the pin", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(EnablePinBreakpoint, "Enable Pin Breakpoint", "Enables a breakpoint on the pin", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(DisablePinBreakpoint, "Disable Pin Breakpoint", "Disables a breakpoint on the pin", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(TogglePinBreakpoint, "Toggle Pin Breakpoint", "Toggles a breakpoint on the pin", EUserInterfaceActionType::Button, FInputChord());

	UI_COMMAND(EnableNode, "Enable Node", "Default state, node is fully executed.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(DisableNode, "Disable Node", "No logic executed, any Input Pin activation is ignored. Node instantly enters a deactivated state.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(SetPassThrough, "Set Pass Through", "Internal node logic not executed. All connected outputs are triggered, node finishes its work.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ForcePinActivation, "Force Pin Activation", "Forces execution of the pin in a graph, used to bypass blockers", EUserInterfaceActionType::Button, FInputChord());

	UI_COMMAND(FocusViewport, "Focus Viewport", "Focus viewport on actor assigned to the node", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(JumpToNodeDefinition, "Jump to Node Definition", "Jump to the node definition", EUserInterfaceActionType::Button, FInputChord());
}

FNarrativeSpawnNodeCommands::FNarrativeSpawnNodeCommands()
	: TCommands<FNarrativeSpawnNodeCommands>(TEXT("FNarrativeSpawnNodeCommands"), LOCTEXT("NarrativeGraph_SpawnNodes", "Narrative Graph - Spawn Nodes"), NAME_None, FAppStyle::GetAppStyleSetName())
{
}

void FNarrativeSpawnNodeCommands::RegisterCommands()
{
	const FString ConfigSection = TEXT("NarrativeSpawnNodes");
	const FString SettingName = TEXT("Node");
	TArray<FString> NodeSpawns;
	GConfig->GetArray(*ConfigSection, *SettingName, NodeSpawns, GEditorPerProjectIni);

	for (int32 x = 0; x < NodeSpawns.Num(); ++x)
	{
		UClass* NodeClass = nullptr;

		FString ClassName;
		if (FParse::Value(*NodeSpawns[x], TEXT("Class="), ClassName))
		{
			UClass* FoundClass = FindFirstObject<UClass>(*ClassName, EFindFirstObjectOptions::ExactClass, ELogVerbosity::Warning, TEXT("looking for SpawnNodes"));
			if (FoundClass && FoundClass->IsChildOf(UNarrativeNode::StaticClass()))
			{
				NodeClass = FoundClass;
			}
		}
		else
		{
			FString CustomName;
			if (FParse::Value(*NodeSpawns[x], TEXT("Name="), CustomName))
			{
				NodeClass = UNarrativeNode::StaticClass();
			}
		}

		if (NodeClass == nullptr)
		{
			// Matching node not found
			continue;
		}

		// Setup a UI Command for key-binding
		TSharedPtr<FUICommandInfo> CommandInfo;

		FKey Key;
		bool bShift = false;
		bool bCtrl = false;
		bool bAlt = false;

		// Parse the key-binding information
		FString KeyString;
		if (FParse::Value(*NodeSpawns[x], TEXT("Key="), KeyString))
		{
			Key = *KeyString;
		}

		if (Key.IsValid())
		{
			FParse::Bool(*NodeSpawns[x], TEXT("Shift="), bShift);
			FParse::Bool(*NodeSpawns[x], TEXT("Alt="), bAlt);
			FParse::Bool(*NodeSpawns[x], TEXT("Ctrl="), bCtrl);
		}

		FInputChord Chord(Key, EModifierKey::FromBools(bCtrl, bAlt, bShift, false));

		const FText CommandLabelText = FText::FromString(NodeClass->GetName());
		const FText Description = FText::Format(LOCTEXT("NodeSpawnDescription", "Hold down the bound keys and left click in the graph panel to spawn a {0} node."), CommandLabelText);

		FUICommandInfo::MakeCommandInfo(this->AsShared(), CommandInfo, FName(*NodeSpawns[x]), CommandLabelText, Description, FSlateIcon(FAppStyle::GetAppStyleSetName(), *FString::Printf(TEXT("%s.%s"), *this->GetContextName().ToString(), *NodeSpawns[x])), EUserInterfaceActionType::Button, Chord);

		NodeCommands.Add(NodeClass, CommandInfo);
	}
}

TSharedPtr<const FInputChord> FNarrativeSpawnNodeCommands::GetChordByClass(const UClass* NodeClass) const
{
	if (NodeCommands.Contains(NodeClass) && NodeCommands[NodeClass]->GetFirstValidChord()->IsValidChord())
	{
		return NodeCommands[NodeClass]->GetFirstValidChord();
	}

	return nullptr;
}

TSharedPtr<FEdGraphSchemaAction> FNarrativeSpawnNodeCommands::GetActionByChord(const FInputChord& InChord) const
{
	if (InChord.IsValidChord())
	{
		for (const TPair<UClass*, TSharedPtr<FUICommandInfo>>& NodeCommand : NodeCommands)
		{
			if (NodeCommand.Value.Get()->HasActiveChord(InChord))
			{
				return GetActionByClass(NodeCommand.Key);
			}
		}
	}

	return TSharedPtr<FEdGraphSchemaAction>();
}

TSharedPtr<FEdGraphSchemaAction> FNarrativeSpawnNodeCommands::GetActionByClass(UClass* NodeClass) const
{
	if (NodeClass == UNarrativeNode::StaticClass())
	{
		return MakeShareable(new FNarrativeGraphSchemaAction_NewComment);
	}
	else
	{
		TSharedPtr<FNarrativeGraphSchemaAction_NewNode> NewNodeAction(new FNarrativeGraphSchemaAction_NewNode(NodeClass));
		return NewNodeAction;
	}
}

#undef LOCTEXT_NAMESPACE
