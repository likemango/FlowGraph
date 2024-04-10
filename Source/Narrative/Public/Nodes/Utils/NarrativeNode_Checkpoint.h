// Copyright XiaoYao

#pragma once

#include "Nodes/NarrativeNode.h"
#include "NarrativeNode_Checkpoint.generated.h"

/**
 * Save the state of the game to the save file
 * It's recommended to replace this with game-specific variant and this node to UNarrativeGraphSettings::HiddenNodes
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Checkpoint", Keywords = "autosave, save"))
class NARRATIVE_API UNarrativeNode_Checkpoint final : public UNarrativeNode
{
	GENERATED_UCLASS_BODY()

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void OnLoad_Implementation() override;
};
