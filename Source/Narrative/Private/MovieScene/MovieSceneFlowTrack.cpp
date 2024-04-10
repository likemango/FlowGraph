// Copyright XiaoYao

#include "MovieScene/MovieSceneNarrativeTrack.h"
#include "MovieScene/MovieSceneNarrativeRepeaterSection.h"
#include "MovieScene/MovieSceneNarrativeTemplate.h"
#include "MovieScene/MovieSceneNarrativeTriggerSection.h"

#include "Evaluation/MovieSceneEvaluationTrack.h"
#include "IMovieSceneTracksModule.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MovieSceneNarrativeTrack)

#define LOCTEXT_NAMESPACE "MovieSceneNarrativeTrack"

void UMovieSceneNarrativeTrack::AddSection(UMovieSceneSection& Section)
{
	Sections.Add(&Section);
}

bool UMovieSceneNarrativeTrack::SupportsType(TSubclassOf<UMovieSceneSection> SectionClass) const
{
	return SectionClass->IsChildOf(UMovieSceneNarrativeSectionBase::StaticClass());
}

UMovieSceneSection* UMovieSceneNarrativeTrack::CreateNewSection()
{
	return NewObject<UMovieSceneNarrativeTriggerSection>(this, NAME_None, RF_Transactional);
}

const TArray<UMovieSceneSection*>& UMovieSceneNarrativeTrack::GetAllSections() const
{
	return Sections;
}

bool UMovieSceneNarrativeTrack::HasSection(const UMovieSceneSection& Section) const
{
	return Sections.Contains(&Section);
}

bool UMovieSceneNarrativeTrack::IsEmpty() const
{
	return (Sections.Num() == 0);
}

void UMovieSceneNarrativeTrack::RemoveAllAnimationData()
{
	Sections.Empty();
}

void UMovieSceneNarrativeTrack::RemoveSection(UMovieSceneSection& Section)
{
	Sections.Remove(&Section);
}

void UMovieSceneNarrativeTrack::RemoveSectionAt(int32 SectionIndex)
{
	Sections.RemoveAt(SectionIndex);
}

FMovieSceneEvalTemplatePtr UMovieSceneNarrativeTrack::CreateTemplateForSection(const UMovieSceneSection& InSection) const
{
	if (const UMovieSceneNarrativeTriggerSection* TriggerSection = Cast<const UMovieSceneNarrativeTriggerSection>(&InSection))
	{
		return FMovieSceneNarrativeTriggerTemplate(*TriggerSection, *this);
	}

	if (const UMovieSceneNarrativeRepeaterSection* RepeaterSection = Cast<const UMovieSceneNarrativeRepeaterSection>(&InSection))
	{
		return FMovieSceneNarrativeRepeaterTemplate(*RepeaterSection, *this);
	}

	return FMovieSceneEvalTemplatePtr();
}

void UMovieSceneNarrativeTrack::PostCompile(FMovieSceneEvaluationTrack& Track, const FMovieSceneTrackCompilerArgs& Args) const
{
	switch (EventPosition)
	{
		case EFireEventsAtPosition::AtStartOfEvaluation:
			Track.SetEvaluationGroup(IMovieSceneTracksModule::GetEvaluationGroupName(EBuiltInEvaluationGroup::PreEvaluation));
			break;

		case EFireEventsAtPosition::AtEndOfEvaluation:
			Track.SetEvaluationGroup(IMovieSceneTracksModule::GetEvaluationGroupName(EBuiltInEvaluationGroup::PostEvaluation));
			break;

		default:
			Track.SetEvaluationGroup(IMovieSceneTracksModule::GetEvaluationGroupName(EBuiltInEvaluationGroup::SpawnObjects));
			Track.SetEvaluationPriority(UMovieSceneSpawnTrack::GetEvaluationPriority() - 100);
			break;
	}

	Track.SetEvaluationMethod(EEvaluationMethod::Swept);
}

FMovieSceneTrackSegmentBlenderPtr UMovieSceneNarrativeTrack::GetTrackSegmentBlender() const
{
	// This is a temporary measure to alleviate some issues with event tracks with finite ranges.
	// By filling empty space between sections, we're essentially always making this track evaluate
	// which allows it to sweep sections correctly when the play-head moves from a finite section
	// to empty space. This doesn't address the issue of the play-head moving from inside a sub-sequence
	// to outside, but that specific issue is even more nuanced and complicated to address.

	struct FMovieSceneNarrativeTrackSegmentBlender : FMovieSceneTrackSegmentBlender
	{
		FMovieSceneNarrativeTrackSegmentBlender()
		{
			bCanFillEmptySpace = true;
			bAllowEmptySegments = true;
		}

		virtual TOptional<FMovieSceneSegment> InsertEmptySpace(const TRange<FFrameNumber>& Range, const FMovieSceneSegment* PreviousSegment, const FMovieSceneSegment* NextSegment) const override
		{
			return FMovieSceneSegment(Range);
		}
	};

	return FMovieSceneNarrativeTrackSegmentBlender();
}

#if WITH_EDITORONLY_DATA

FText UMovieSceneNarrativeTrack::GetDefaultDisplayName() const
{
	return LOCTEXT("TrackName", "Narrative Events");
}

#endif

#undef LOCTEXT_NAMESPACE
