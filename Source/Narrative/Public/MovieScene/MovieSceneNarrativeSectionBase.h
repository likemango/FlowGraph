// Copyright XiaoYao

#pragma once

#include "MovieSceneSection.h"
#include "MovieSceneNarrativeSectionBase.generated.h"

/**
 * Base class for flow sections
 */
UCLASS()
class NARRATIVE_API UMovieSceneNarrativeSectionBase : public UMovieSceneSection
{
	GENERATED_BODY()

public:
	virtual TArrayView<FString> GetAllEntryPoints() { return TArrayView<FString>(); }
};
