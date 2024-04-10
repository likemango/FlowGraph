// Copyright XiaoYao

#pragma once

#include "ISequencerSection.h"
#include "ISequencer.h"

class FSequencerSectionPainter;

class NARRATIVEEDITOR_API FNarrativeSectionBase : public FSequencerSection
{
public:
	FNarrativeSectionBase(UMovieSceneSection& InSectionObject, TWeakPtr<ISequencer> InSequencer)
		: FSequencerSection(InSectionObject)
		, Sequencer(InSequencer)
	{
	}

protected:
	void PaintEventName(FSequencerSectionPainter& Painter, int32 LayerId, const FString& EventString, float PixelPosition, bool bIsEventValid = true) const;
	bool IsSectionSelected() const;

	TWeakPtr<ISequencer> Sequencer;
};

/**
* An implementation of flow sections.
*/
class NARRATIVEEDITOR_API FNarrativeSection : public FNarrativeSectionBase
{
public:
	FNarrativeSection(UMovieSceneSection& InSectionObject, TWeakPtr<ISequencer> InSequencer)
		: FNarrativeSectionBase(InSectionObject, InSequencer)
	{
	}

	virtual int32 OnPaintSection(FSequencerSectionPainter& Painter) const override;
};

class NARRATIVEEDITOR_API FNarrativeTriggerSection : public FNarrativeSectionBase
{
public:
	FNarrativeTriggerSection(UMovieSceneSection& InSectionObject, TWeakPtr<ISequencer> InSequencer)
		: FNarrativeSectionBase(InSectionObject, InSequencer)
	{
	}

	virtual int32 OnPaintSection(FSequencerSectionPainter& Painter) const override;
};

class NARRATIVEEDITOR_API FNarrativeRepeaterSection : public FNarrativeSectionBase
{
public:
	FNarrativeRepeaterSection(UMovieSceneSection& InSectionObject, TWeakPtr<ISequencer> InSequencer)
		: FNarrativeSectionBase(InSectionObject, InSequencer)
	{
	}

	virtual int32 OnPaintSection(FSequencerSectionPainter& Painter) const override;
};
