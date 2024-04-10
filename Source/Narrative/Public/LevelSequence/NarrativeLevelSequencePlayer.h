// Copyright XiaoYao

#pragma once

#include "LevelSequencePlayer.h"
#include "NarrativeLevelSequencePlayer.generated.h"

class UNarrativeNode;

/**
 * Custom ULevelSequencePlayer allows for binding Narrative Nodes to Level Sequence events
 */
UCLASS()
class NARRATIVE_API UNarrativeLevelSequencePlayer : public ULevelSequencePlayer
{
	GENERATED_UCLASS_BODY()

private:
	// most likely this is a UNarrativeNode_PlayLevelSequence or its child
	UPROPERTY()
	UNarrativeNode* NarrativeEventReceiver;

public:
	// variant of ULevelSequencePlayer::CreateLevelSequencePlayer
	static UNarrativeLevelSequencePlayer* CreateNarrativeLevelSequencePlayer(
		const UObject* WorldContextObject,
		ULevelSequence* LevelSequence,
		FMovieSceneSequencePlaybackSettings Settings,
		FLevelSequenceCameraSettings CameraSettings,
		AActor* TransformOriginActor,
		const bool bReplicates,
		const bool bAlwaysRelevant,
		ALevelSequenceActor*& OutActor);

	void SetNarrativeEventReceiver(UNarrativeNode* NarrativeNode) { NarrativeEventReceiver = NarrativeNode; }

	// IMovieScenePlayer
	virtual TArray<UObject*> GetEventContexts() const override;
	// --
};
