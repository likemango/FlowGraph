// Copyright XiaoYao

#include "LevelSequence/NarrativeLevelSequenceActor.h"
#include "LevelSequence/NarrativeLevelSequencePlayer.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativeLevelSequenceActor)

ANarrativeLevelSequenceActor::ANarrativeLevelSequenceActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UNarrativeLevelSequencePlayer>("AnimationPlayer"))
	, ReplicatedLevelSequenceAsset(nullptr)
{
}

void ANarrativeLevelSequenceActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANarrativeLevelSequenceActor, ReplicatedLevelSequenceAsset);
}

void ANarrativeLevelSequenceActor::SetPlaybackSettings(FMovieSceneSequencePlaybackSettings NewPlaybackSettings)
{
	PlaybackSettings = NewPlaybackSettings;
	GetSequencePlayer()->SetPlaybackSettings(PlaybackSettings);
}

void ANarrativeLevelSequenceActor::SetReplicatedLevelSequenceAsset(ULevelSequence* Asset)
{
	if (HasAuthority())
	{
		LevelSequenceAsset = Asset;
		ReplicatedLevelSequenceAsset = LevelSequenceAsset;
	}
}

void ANarrativeLevelSequenceActor::OnRep_ReplicatedLevelSequenceAsset()
{
	LevelSequenceAsset = ReplicatedLevelSequenceAsset;
	ReplicatedLevelSequenceAsset = nullptr;

	InitializePlayer();
}
