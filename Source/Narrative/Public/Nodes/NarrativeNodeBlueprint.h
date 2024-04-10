// Copyright XiaoYao

#pragma once

#include "Engine/Blueprint.h"
#include "NarrativeNodeBlueprint.generated.h"

/**
 * A specialized blueprint class required for customizing Asset Type Actions
 */
UCLASS(BlueprintType)
class NARRATIVE_API UNarrativeNodeBlueprint : public UBlueprint
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITOR
	// UBlueprint
	virtual bool SupportedByDefaultBlueprintFactory() const override { return false; }
	virtual bool SupportsDelegates() const override { return false; }
	// --
#endif
};
