// Copyright XiaoYao

#include "LevelSequence/NarrativeLevelSequencePlayer.h"
#include "LevelSequence/NarrativeLevelSequenceActor.h"
#include "Nodes/NarrativeNode.h"

#include "DefaultLevelSequenceInstanceData.h"
#include "Runtime/Launch/Resources/Version.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativeLevelSequencePlayer)

UNarrativeLevelSequencePlayer::UNarrativeLevelSequencePlayer(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, NarrativeEventReceiver(nullptr)
{
}

UNarrativeLevelSequencePlayer* UNarrativeLevelSequencePlayer::CreateNarrativeLevelSequencePlayer(
	const UObject* WorldContextObject,
	ULevelSequence* LevelSequence,
	FMovieSceneSequencePlaybackSettings Settings,
	FLevelSequenceCameraSettings CameraSettings,
	AActor* TransformOriginActor,
	const bool bReplicates,
	const bool bAlwaysRelevant,
	ALevelSequenceActor*& OutActor
)
{
	if (LevelSequence == nullptr)
	{
		return nullptr;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (World == nullptr || World->bIsTearingDown)
	{
		return nullptr;
	}

	// Sequence Actor might be spawned exactly where playback happens
	FTransform SpawnTransform = FTransform::Identity;
	{
		// apply Transform Origin
		// https://docs.unrealengine.com/5.0/en-US/creating-level-sequences-with-dynamic-transforms-in-unreal-engine/
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION > 3
		if (TransformOriginActor->IsValidLowLevel())
#else
		if (IsValid(TransformOriginActor))
#endif
		{
			// moving Level Sequence Actor might allow proper distance-based actor replication in networked games
			SpawnTransform = TransformOriginActor->GetTransform();
			SpawnTransform = FTransform(SpawnTransform.GetRotation(), SpawnTransform.GetLocation(), FVector::OneVector);
		}
	}

	// Create Sequence Actor
	// We use deferred spawn, so we can set all actor properties prior to its initialization.
	// This also helpful in case of multiplayer, since all actor settings are replicated with the spawned actor. No need to call replication just after spawn.
	ANarrativeLevelSequenceActor* Actor = World->SpawnActorDeferred<ANarrativeLevelSequenceActor>(ANarrativeLevelSequenceActor::StaticClass(), SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	Actor->SetPlaybackSettings(Settings);
	Actor->CameraSettings = CameraSettings;

	// apply Transform Origin to spawned actor
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION > 3
	if (TransformOriginActor->IsValidLowLevel())
#else
	if (IsValid(TransformOriginActor))
#endif
	{
		if (UDefaultLevelSequenceInstanceData* InstanceData = Cast<UDefaultLevelSequenceInstanceData>(Actor->DefaultInstanceData))
		{
			Actor->bOverrideInstanceData = true;
			InstanceData->TransformOriginActor = TransformOriginActor;
		}
	}

	// support networking
	if (bReplicates)
	{
		Actor->bReplicatePlayback = true;
		Actor->bAlwaysRelevant = bAlwaysRelevant;
		Actor->SetReplicatedLevelSequenceAsset(LevelSequence);
	}
	else
	{
		Actor->LevelSequenceAsset = LevelSequence;
	}

	// finish deferred spawn
	Actor->FinishSpawning(SpawnTransform);
	OutActor = Actor;

	// Sequence Player is created by Level Sequence Actor
	return Cast<UNarrativeLevelSequencePlayer>(Actor->GetSequencePlayer());
}

TArray<UObject*> UNarrativeLevelSequencePlayer::GetEventContexts() const
{
	TArray<UObject*> EventContexts;

	if (NarrativeEventReceiver)
	{
		EventContexts.Add(NarrativeEventReceiver);
	}

	return EventContexts;
}
