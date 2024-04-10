// Copyright XiaoYao

#pragma once

#include "Evaluation/MovieSceneEvalTemplate.h"

#include "MovieSceneNarrativeRepeaterSection.h"
#include "MovieSceneNarrativeTrack.h"
#include "MovieSceneNarrativeTriggerSection.h"
#include "MovieSceneNarrativeTemplate.generated.h"

USTRUCT()
struct FMovieSceneNarrativeTemplateBase : public FMovieSceneEvalTemplate
{
	GENERATED_BODY()

	FMovieSceneNarrativeTemplateBase()
		: bFireEventsWhenForwards(false)
		, bFireEventsWhenBackwards(false)
	{
	}

	FMovieSceneNarrativeTemplateBase(const UMovieSceneNarrativeTrack& InTrack, const UMovieSceneNarrativeSectionBase& InSection)
		: bFireEventsWhenForwards(InTrack.bFireEventsWhenForwards)
		, bFireEventsWhenBackwards(InTrack.bFireEventsWhenBackwards)
	{
	}

protected:
	UPROPERTY()
	uint32 bFireEventsWhenForwards : 1;

	UPROPERTY()
	uint32 bFireEventsWhenBackwards : 1;

private:
	virtual UScriptStruct& GetScriptStructImpl() const override { return *StaticStruct(); }
};

USTRUCT()
struct FMovieSceneNarrativeTriggerTemplate : public FMovieSceneNarrativeTemplateBase
{
	GENERATED_BODY()

	FMovieSceneNarrativeTriggerTemplate() {}
	FMovieSceneNarrativeTriggerTemplate(const UMovieSceneNarrativeTriggerSection& Section, const UMovieSceneNarrativeTrack& Track);

	UPROPERTY()
	TArray<FFrameNumber> EventTimes;

	UPROPERTY()
	TArray<FString> EventNames;

private:
	virtual UScriptStruct& GetScriptStructImpl() const override { return *StaticStruct(); }
	virtual void EvaluateSwept(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const TRange<FFrameNumber>& SweptRange, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const override;
};

USTRUCT()
struct FMovieSceneNarrativeRepeaterTemplate : public FMovieSceneNarrativeTemplateBase
{
	GENERATED_BODY()

	FMovieSceneNarrativeRepeaterTemplate() {}
	FMovieSceneNarrativeRepeaterTemplate(const UMovieSceneNarrativeRepeaterSection& Section, const UMovieSceneNarrativeTrack& Track);

	UPROPERTY()
	FString EventName;

private:
	virtual UScriptStruct& GetScriptStructImpl() const override { return *StaticStruct(); }
	virtual void EvaluateSwept(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const TRange<FFrameNumber>& SweptRange, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const override;
};
