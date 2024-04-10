// Copyright XiaoYao

#pragma once

#include "MovieSceneNarrativeSectionBase.h"
#include "MovieSceneNarrativeRepeaterSection.generated.h"

/**
 * Narrative section that will trigger its event exactly once, every time it is evaluated.
 */
UCLASS()
class NARRATIVE_API UMovieSceneNarrativeRepeaterSection : public UMovieSceneNarrativeSectionBase
{
	GENERATED_BODY()

public:
#if WITH_EDITORONLY_DATA
	virtual TArrayView<FString> GetAllEntryPoints() override { return MakeArrayView(&EventName, 1); }
#endif

	/** The event that should be triggered each time this section is evaluated */
	UPROPERTY(EditAnywhere, Category = "Narrative")
	FString EventName;
};
