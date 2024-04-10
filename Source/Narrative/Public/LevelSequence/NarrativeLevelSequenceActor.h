// Copyright XiaoYao

#pragma once

#include "LevelSequenceActor.h"
#include "NarrativeLevelSequenceActor.generated.h"

class ULevelSequence;

/**
 * Custom ALevelSequenceActor is needed to override ULevelSequencePlayer class
 */
UCLASS(hideCategories=(Rendering, Physics, LOD, Activation, Input))
class NARRATIVE_API ANarrativeLevelSequenceActor : public ALevelSequenceActor
{
	GENERATED_UCLASS_BODY()

protected:
	UPROPERTY(ReplicatedUsing = OnRep_ReplicatedLevelSequenceAsset)
	TObjectPtr<ULevelSequence> ReplicatedLevelSequenceAsset;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	void SetPlaybackSettings(FMovieSceneSequencePlaybackSettings NewPlaybackSettings);
	void SetReplicatedLevelSequenceAsset(ULevelSequence* Asset);

protected:
	UFUNCTION()
	void OnRep_ReplicatedLevelSequenceAsset();
};
