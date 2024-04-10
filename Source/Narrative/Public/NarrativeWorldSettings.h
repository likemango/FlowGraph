// Copyright XiaoYao

#pragma once

#include "GameFramework/WorldSettings.h"
#include "NarrativeWorldSettings.generated.h"

class UNarrativeComponent;

/**
 * World Settings used to start a Narrative for this world
 */
UCLASS()
class NARRATIVE_API ANarrativeWorldSettings : public AWorldSettings
{
	GENERATED_UCLASS_BODY()

private:
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Narrative", meta = (AllowPrivateAccess = "true"))
	UNarrativeComponent* NarrativeComponent;

public:
	UNarrativeComponent* GetNarrativeComponent() const { return NarrativeComponent; }
};
