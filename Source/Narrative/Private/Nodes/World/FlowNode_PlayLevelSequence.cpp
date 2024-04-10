// Copyright XiaoYao

#include "Nodes/World/NarrativeNode_PlayLevelSequence.h"

#include "NarrativeAsset.h"
#include "NarrativeLogChannels.h"
#include "NarrativeSubsystem.h"
#include "LevelSequence/NarrativeLevelSequencePlayer.h"

#if WITH_EDITOR
#include "MovieScene/MovieSceneNarrativeTrack.h"
#include "MovieScene/MovieSceneNarrativeTriggerSection.h"
#endif

#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "Runtime/Launch/Resources/Version.h"
#include "VisualLogger/VisualLogger.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativeNode_PlayLevelSequence)

FNarrativeNodeLevelSequenceEvent UNarrativeNode_PlayLevelSequence::OnPlaybackStarted;
FNarrativeNodeLevelSequenceEvent UNarrativeNode_PlayLevelSequence::OnPlaybackCompleted;

UNarrativeNode_PlayLevelSequence::UNarrativeNode_PlayLevelSequence(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bPlayReverse(false)
	, bUseGraphOwnerAsTransformOrigin(false)
	, bReplicates(false)
	, bAlwaysRelevant(false)
	, bApplyOwnerTimeDilation(true)
	, LoadedSequence(nullptr)
	, SequencePlayer(nullptr)
	, CachedPlayRate(0)
	, StartTime(0.0f)
	, ElapsedTime(0.0f)
	, TimeDilation(1.0f)
{
#if WITH_EDITOR
	Category = TEXT("World");
	NodeStyle = ENarrativeNodeStyle::Latent;
#endif

	InputPins.Empty();
	InputPins.Add(FNarrativePin(TEXT("Start")));
	InputPins.Add(FNarrativePin(TEXT("Pause")));
	InputPins.Add(FNarrativePin(TEXT("Resume")));
	InputPins.Add(FNarrativePin(TEXT("Stop")));

	OutputPins.Add(FNarrativePin(TEXT("PreStart")));
	OutputPins.Add(FNarrativePin(TEXT("Started")));
	OutputPins.Add(FNarrativePin(TEXT("Completed")));
	OutputPins.Add(FNarrativePin(TEXT("Stopped")));
}

#if WITH_EDITOR
TArray<FNarrativePin> UNarrativeNode_PlayLevelSequence::GetContextOutputs()
{
	if (Sequence.IsNull())
	{
		return TArray<FNarrativePin>();
	}

	TArray<FNarrativePin> Pins = {};

	Sequence = Sequence.LoadSynchronous();
	if (Sequence && Sequence->GetMovieScene())
	{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 2
		for (const UMovieSceneTrack* Track : Sequence->GetMovieScene()->GetMasterTracks())
#else
		for (const UMovieSceneTrack* Track : Sequence->GetMovieScene()->GetTracks())
#endif
		{
			if (Track->GetClass() == UMovieSceneNarrativeTrack::StaticClass())
			{
				for (UMovieSceneSection* Section : Track->GetAllSections())
				{
					if (UMovieSceneNarrativeSectionBase* NarrativeSection = Cast<UMovieSceneNarrativeSectionBase>(Section))
					{
						for (const FString& EventName : NarrativeSection->GetAllEntryPoints())
						{
							if (!EventName.IsEmpty())
							{
								Pins.Emplace(EventName);
							}
						}
					}
				}
			}
		}
	}

	return Pins;
}

void UNarrativeNode_PlayLevelSequence::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.Property && PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UNarrativeNode_PlayLevelSequence, Sequence))
	{
		OnReconstructionRequested.ExecuteIfBound();
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void UNarrativeNode_PlayLevelSequence::PreloadContent()
{
#if ENABLE_VISUAL_LOG
	UE_VLOG(this, LogNarrative, Log, TEXT("Preloading"));
#endif

	if (!Sequence.IsNull())
	{
		StreamableManager.RequestAsyncLoad({Sequence.ToSoftObjectPath()}, FStreamableDelegate());
	}
}

void UNarrativeNode_PlayLevelSequence::FlushContent()
{
#if ENABLE_VISUAL_LOG
	UE_VLOG(this, LogNarrative, Log, TEXT("Flushing preload"));
#endif

	if (!Sequence.IsNull())
	{
		StreamableManager.Unload(Sequence.ToSoftObjectPath());
	}
}

void UNarrativeNode_PlayLevelSequence::InitializeInstance()
{
	Super::InitializeInstance();

	// Cache Play Rate set by user
	CachedPlayRate = PlaybackSettings.PlayRate;
}

void UNarrativeNode_PlayLevelSequence::CreatePlayer()
{
	LoadedSequence = Sequence.LoadSynchronous();
	if (LoadedSequence)
	{
		ALevelSequenceActor* SequenceActor;

		AActor* OwningActor = TryGetRootNarrativeActorOwner();

		// Apply AActor::CustomTimeDilation from owner of the Root Narrative
		if (IsValid(OwningActor))
		{
			PlaybackSettings.PlayRate = CachedPlayRate * OwningActor->CustomTimeDilation;
		}

		// Apply Transform Origin
		AActor* TransformOriginActor = bUseGraphOwnerAsTransformOrigin ? OwningActor : nullptr;

		// Finally create the player
		SequencePlayer = UNarrativeLevelSequencePlayer::CreateNarrativeLevelSequencePlayer(this, LoadedSequence, PlaybackSettings, CameraSettings, TransformOriginActor, bReplicates, bAlwaysRelevant, SequenceActor);

		if (SequencePlayer)
		{
			SequencePlayer->SetNarrativeEventReceiver(this);
		}

		const FFrameRate FrameRate = LoadedSequence->GetMovieScene()->GetTickResolution();
		const FFrameNumber PlaybackStartFrame = LoadedSequence->GetMovieScene()->GetPlaybackRange().GetLowerBoundValue();
		StartTime = FQualifiedFrameTime(FFrameTime(PlaybackStartFrame, 0.0f), FrameRate).AsSeconds();
	}
}

void UNarrativeNode_PlayLevelSequence::ExecuteInput(const FName& PinName)
{
	if (PinName == TEXT("Start"))
	{
		LoadedSequence = Sequence.LoadSynchronous();

		if (GetNarrativeSubsystem()->GetWorld() && LoadedSequence)
		{
			CreatePlayer();

			if (SequencePlayer)
			{
				TriggerOutput(TEXT("PreStart"));

				SequencePlayer->OnFinished.AddDynamic(this, &UNarrativeNode_PlayLevelSequence::OnPlaybackFinished);

				if (bPlayReverse)
				{
					SequencePlayer->PlayReverse();
				}
				else
				{
					SequencePlayer->Play();
				}

				TriggerOutput(TEXT("Started"));
			}
		}

		TriggerFirstOutput(false);
	}
	else if (PinName == TEXT("Stop"))
	{
		StopPlayback();
	}
	else if (PinName == TEXT("Pause"))
	{
		SequencePlayer->Pause();
	}
	else if (PinName == TEXT("Resume") && SequencePlayer->IsPaused())
	{
		SequencePlayer->Play();
	}
}

void UNarrativeNode_PlayLevelSequence::OnSave_Implementation()
{
	if (SequencePlayer)
	{
		ElapsedTime = SequencePlayer->GetCurrentTime().AsSeconds();
	}
}

void UNarrativeNode_PlayLevelSequence::OnLoad_Implementation()
{
	if (ElapsedTime != 0.0f)
	{
		LoadedSequence = Sequence.LoadSynchronous();
		if (GetNarrativeSubsystem()->GetWorld() && LoadedSequence)
		{
			CreatePlayer();

			if (SequencePlayer)
			{
				SequencePlayer->OnFinished.AddDynamic(this, &UNarrativeNode_PlayLevelSequence::OnPlaybackFinished);

				SequencePlayer->SetPlaybackPosition(FMovieSceneSequencePlaybackParams(ElapsedTime, EUpdatePositionMethod::Jump));

				// Take into account Play Rate set in the Playback Settings
				SequencePlayer->SetPlayRate(TimeDilation * CachedPlayRate);

				if (bPlayReverse)
				{
					SequencePlayer->PlayReverse();
				}
				else
				{
					SequencePlayer->Play();
				}
			}
		}
	}
}

void UNarrativeNode_PlayLevelSequence::TriggerEvent(const FString& EventName)
{
	TriggerOutput(*EventName, false);
}

void UNarrativeNode_PlayLevelSequence::OnTimeDilationUpdate(const float NewTimeDilation)
{
	if (SequencePlayer)
	{
		TimeDilation = NewTimeDilation;

		// Take into account Play Rate set in the Playback Settings
		SequencePlayer->SetPlayRate(NewTimeDilation * CachedPlayRate);
	}
}

void UNarrativeNode_PlayLevelSequence::OnPlaybackFinished()
{
	TriggerOutput(TEXT("Completed"), true);
}

void UNarrativeNode_PlayLevelSequence::StopPlayback()
{
	if (SequencePlayer)
	{
		SequencePlayer->Stop();
	}

	TriggerOutput(TEXT("Stopped"), true);
}

void UNarrativeNode_PlayLevelSequence::Cleanup()
{
	if (SequencePlayer)
	{
		SequencePlayer->SetNarrativeEventReceiver(nullptr);
		SequencePlayer->OnFinished.RemoveAll(this);
		if (!PlaybackSettings.bPauseAtEnd)
		{
			SequencePlayer->Stop();
		}
		SequencePlayer = nullptr;
	}

	LoadedSequence = nullptr;
	StartTime = 0.0f;
	ElapsedTime = 0.0f;
	TimeDilation = 1.0f;

#if ENABLE_VISUAL_LOG
	UE_VLOG(this, LogNarrative, Log, TEXT("Finished playback: %s"), *Sequence.ToString());
#endif
}

FString UNarrativeNode_PlayLevelSequence::GetPlaybackProgress() const
{
	if (SequencePlayer && SequencePlayer->IsPlaying())
	{
		return FString::Printf(TEXT("%.*f / %.*f"), 2, SequencePlayer->GetCurrentTime().AsSeconds() - StartTime, 2, SequencePlayer->GetDuration().AsSeconds());
	}

	return FString();
}

#if WITH_EDITOR
FString UNarrativeNode_PlayLevelSequence::GetNodeDescription() const
{
	return Sequence.IsNull() ? TEXT("[No sequence]") : Sequence.GetAssetName();
}

EDataValidationResult UNarrativeNode_PlayLevelSequence::ValidateNode()
{
	if (Sequence.IsNull())
	{
		ValidationLog.Error<UNarrativeNode>(TEXT("Level Sequence asset not assigned or invalid!"), this);
		return EDataValidationResult::Invalid;
	}

	return EDataValidationResult::Valid;
}

FString UNarrativeNode_PlayLevelSequence::GetStatusString() const
{
	return GetPlaybackProgress();
}

UObject* UNarrativeNode_PlayLevelSequence::GetAssetToEdit()
{
	return Sequence.IsNull() ? nullptr : Sequence.LoadSynchronous();
}
#endif

#if ENABLE_VISUAL_LOG
void UNarrativeNode_PlayLevelSequence::GrabDebugSnapshot(struct FVisualLogEntry* Snapshot) const
{
	FVisualLogStatusCategory NewCategory = FVisualLogStatusCategory(TEXT("Sequence"));
	NewCategory.Add(*Sequence.ToString(), FString());
	Snapshot->Status.Add(NewCategory);
}
#endif
