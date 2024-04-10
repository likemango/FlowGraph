// Copyright XiaoYao

#pragma once

#include "Channels/MovieSceneStringChannel.h"

#include "MovieSceneNarrativeSectionBase.h"
#include "MovieSceneNarrativeTriggerSection.generated.h"

/**
 * Narrative section that triggers specific timed events.
 */
UCLASS()
class NARRATIVE_API UMovieSceneNarrativeTriggerSection : public UMovieSceneNarrativeSectionBase
{
	GENERATED_BODY()

public:
	UMovieSceneNarrativeTriggerSection(const FObjectInitializer& ObjInit);

#if WITH_EDITORONLY_DATA
	virtual TArrayView<FString> GetAllEntryPoints() override { return StringChannel.GetData().GetValues(); }
#endif

	/** The channel that defines this section's timed events */
	UPROPERTY()
	FMovieSceneStringChannel StringChannel;
};
