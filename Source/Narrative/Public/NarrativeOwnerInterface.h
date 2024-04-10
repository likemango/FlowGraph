// Copyright XiaoYao

#pragma once

#include "UObject/Interface.h"

#include "NarrativeOwnerInterface.generated.h"

// (optional) interface to enable a Narrative owner object to execute CallOwnerFunction nodes
UINTERFACE(MinimalAPI, Blueprintable, BlueprintType)
class UNarrativeOwnerInterface : public UInterface
{
	GENERATED_BODY()
};

class NARRATIVE_API INarrativeOwnerInterface
{
	GENERATED_BODY()
};
