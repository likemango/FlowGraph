// Copyright XiaoYao

#pragma once

#include "Nodes/NarrativeNode.h"
#include "NarrativeNode_LogicalAND.generated.h"

/**
 * Logical AND
 * Output will be triggered only once
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "AND", Keywords = "&"))
class NARRATIVE_API UNarrativeNode_LogicalAND final : public UNarrativeNode
{
	GENERATED_UCLASS_BODY()

private:
	UPROPERTY(SaveGame)
	TSet<FName> ExecutedInputNames;
	
#if WITH_EDITOR
public:
	virtual bool CanUserAddInput() const override { return true; }
#endif

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;
};
