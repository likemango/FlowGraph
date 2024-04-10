// Copyright XiaoYao

#include "MovieScene/MovieSceneNarrativeTemplate.h"
#include "MovieScene/MovieSceneNarrativeTrack.h"
#include "Nodes/World/NarrativeNode_PlayLevelSequence.h"

#include "Evaluation/MovieSceneEvaluation.h"
#include "IMovieScenePlayer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MovieSceneNarrativeTemplate)

#define LOCTEXT_NAMESPACE "MovieSceneNarrativeTemplate"

DECLARE_CYCLE_STAT(TEXT("Narrative Track Token Execute"), MovieSceneEval_NarrativeTrack_TokenExecute, STATGROUP_MovieSceneEval);

struct FNarrativeTrackExecutionToken final : IMovieSceneExecutionToken
{
	FNarrativeTrackExecutionToken(TArray<FString> InEventNames)
		: EventNames(MoveTemp(InEventNames))
	{
	}

	TArray<FString> EventNames;

	virtual void Execute(const FMovieSceneContext& Context, const FMovieSceneEvaluationOperand& Operand, FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) override
	{
		MOVIESCENE_DETAILED_SCOPE_CYCLE_COUNTER(MovieSceneEval_NarrativeTrack_TokenExecute)

		for (const FString& EventName : EventNames)
		{
			for (UObject* EventReceiver : Player.GetEventContexts())
			{
				if (UNarrativeNode_PlayLevelSequence* NarrativeNode = Cast<UNarrativeNode_PlayLevelSequence>(EventReceiver))
				{
					NarrativeNode->TriggerEvent(EventName);
				}
			}
		}
	}
};

FMovieSceneNarrativeTriggerTemplate::FMovieSceneNarrativeTriggerTemplate(const UMovieSceneNarrativeTriggerSection& Section, const UMovieSceneNarrativeTrack& Track)
	: FMovieSceneNarrativeTemplateBase(Track, Section)
{
	const TMovieSceneChannelData<const FString> EventData = Section.StringChannel.GetData();
	const TArrayView<const FFrameNumber> Times = EventData.GetTimes();
	const TArrayView<const FString> EntryPoints = EventData.GetValues();

	EventTimes.Reserve(Times.Num());
	EventNames.Reserve(Times.Num());

	for (int32 Index = 0; Index < Times.Num(); ++Index)
	{
		EventTimes.Add(Times[Index]);
		EventNames.Add(EntryPoints[Index]);
	}
}

void FMovieSceneNarrativeTriggerTemplate::EvaluateSwept(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const TRange<FFrameNumber>& SweptRange, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const
{
	// Don't allow events to fire when playback is in a stopped state. This can occur when stopping 
	// playback and returning the current position to the start of playback. It's not desirable to have 
	// all the events from the last playback position to the start of playback be fired.
	if (Context.GetStatus() == EMovieScenePlayerStatus::Stopped || Context.IsSilent())
	{
		return;
	}

	const bool bBackwards = Context.GetDirection() == EPlayDirection::Backwards;

	if ((!bBackwards && !bFireEventsWhenForwards) || (bBackwards && !bFireEventsWhenBackwards))
	{
		return;
	}

	TArray<FString> EventsToTrigger;

	if (bBackwards)
	{
		// Trigger events backwards
		for (int32 KeyIndex = EventTimes.Num() - 1; KeyIndex >= 0; --KeyIndex)
		{
			FFrameNumber Time = EventTimes[KeyIndex];
			if (!EventNames[KeyIndex].IsEmpty() && SweptRange.Contains(Time))
			{
				EventsToTrigger.Add(EventNames[KeyIndex]);
			}
		}
	}
	else
	{
		// Trigger events forwards
		for (int32 KeyIndex = 0; KeyIndex < EventTimes.Num(); ++KeyIndex)
		{
			FFrameNumber Time = EventTimes[KeyIndex];
			if (!EventNames[KeyIndex].IsEmpty() && SweptRange.Contains(Time))
			{
				EventsToTrigger.Add(EventNames[KeyIndex]);
			}
		}
	}

	if (EventsToTrigger.Num())
	{
		ExecutionTokens.Add(FNarrativeTrackExecutionToken(MoveTemp(EventsToTrigger)));
	}
}

FMovieSceneNarrativeRepeaterTemplate::FMovieSceneNarrativeRepeaterTemplate(const UMovieSceneNarrativeRepeaterSection& Section, const UMovieSceneNarrativeTrack& Track)
	: FMovieSceneNarrativeTemplateBase(Track, Section)
	, EventName(Section.EventName)
{
}

void FMovieSceneNarrativeRepeaterTemplate::EvaluateSwept(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const TRange<FFrameNumber>& SweptRange, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const
{
	const bool bBackwards = Context.GetDirection() == EPlayDirection::Backwards;
	const FFrameNumber CurrentFrame = bBackwards ? Context.GetTime().CeilToFrame() : Context.GetTime().FloorToFrame();

	// Don't allow events to fire when playback is in a stopped state. This can occur when stopping 
	// playback and returning the current position to the start of playback. It's not desirable to have 
	// all the events from the last playback position to the start of playback be fired.
	if (EventName.IsEmpty() || !SweptRange.Contains(CurrentFrame) || Context.GetStatus() == EMovieScenePlayerStatus::Stopped || Context.IsSilent())
	{
		return;
	}

	if ((!bBackwards && bFireEventsWhenForwards) || (bBackwards && bFireEventsWhenBackwards))
	{
		ExecutionTokens.Add(FNarrativeTrackExecutionToken({EventName}));
	}
}

#undef LOCTEXT_NAMESPACE
