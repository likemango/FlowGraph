// Copyright XiaoYao

#pragma once

#include "EdGraph/EdGraphSchema.h"
#include "Framework/Commands/Commands.h"
#include "Framework/Commands/UICommandInfo.h"
#include "Templates/SharedPointer.h"

class NARRATIVEEDITOR_API FNarrativeToolbarCommands : public TCommands<FNarrativeToolbarCommands>
{
public:
	FNarrativeToolbarCommands();

	TSharedPtr<FUICommandInfo> RefreshAsset;
	TSharedPtr<FUICommandInfo> ValidateAsset;

	TSharedPtr<FUICommandInfo> SearchInAsset;
	TSharedPtr<FUICommandInfo> EditAssetDefaults;

	TSharedPtr<FUICommandInfo> GoToParentInstance;

	virtual void RegisterCommands() override;
};

/** Generic graph commands for the flow graph */
class NARRATIVEEDITOR_API FNarrativeGraphCommands : public TCommands<FNarrativeGraphCommands>
{
public:
	FNarrativeGraphCommands();

	/** Context Pins */
	TSharedPtr<FUICommandInfo> RefreshContextPins;

	/** Pins */
	TSharedPtr<FUICommandInfo> AddInput;
	TSharedPtr<FUICommandInfo> AddOutput;
	TSharedPtr<FUICommandInfo> RemovePin;

	/** Breakpoints */
	TSharedPtr<FUICommandInfo> AddPinBreakpoint;
	TSharedPtr<FUICommandInfo> RemovePinBreakpoint;
	TSharedPtr<FUICommandInfo> EnablePinBreakpoint;
	TSharedPtr<FUICommandInfo> DisablePinBreakpoint;
	TSharedPtr<FUICommandInfo> TogglePinBreakpoint;

	/** Execution Override */
	TSharedPtr<FUICommandInfo> EnableNode;
	TSharedPtr<FUICommandInfo> DisableNode;
	TSharedPtr<FUICommandInfo> SetPassThrough;
	TSharedPtr<FUICommandInfo> ForcePinActivation;

	/** Jumps */
	TSharedPtr<FUICommandInfo> FocusViewport;
	TSharedPtr<FUICommandInfo> JumpToNodeDefinition;

	virtual void RegisterCommands() override;
};

/** Handles spawning nodes by keyboard shortcut */
class NARRATIVEEDITOR_API FNarrativeSpawnNodeCommands : public TCommands<FNarrativeSpawnNodeCommands>
{
public:
	FNarrativeSpawnNodeCommands();

	virtual void RegisterCommands() override;

	TSharedPtr<const FInputChord> GetChordByClass(const UClass* NodeClass) const;
	TSharedPtr<FEdGraphSchemaAction> GetActionByChord(const FInputChord& InChord) const;

private:
	TSharedPtr<FEdGraphSchemaAction> GetActionByClass(UClass* NodeClass) const;

	TMap<UClass*, TSharedPtr<FUICommandInfo>> NodeCommands;
};
