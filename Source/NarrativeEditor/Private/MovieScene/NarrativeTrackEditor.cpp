// Copyright XiaoYao

#include "MovieScene/NarrativeTrackEditor.h"
#include "MovieScene/NarrativeSection.h"

#include "MovieScene/MovieSceneNarrativeRepeaterSection.h"
#include "MovieScene/MovieSceneNarrativeTrack.h"
#include "MovieScene/MovieSceneNarrativeTriggerSection.h"

#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "ISequencerSection.h"
#include "LevelSequence.h"
#include "MovieSceneSequenceEditor.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Sections/MovieSceneEventSection.h"
#include "SequencerUtilities.h"

#define LOCTEXT_NAMESPACE "FNarrativeTrackEditor"

TSharedRef<ISequencerTrackEditor> FNarrativeTrackEditor::CreateTrackEditor(TSharedRef<ISequencer> InSequencer)
{
	return MakeShareable(new FNarrativeTrackEditor(InSequencer));
}

TSharedRef<ISequencerSection> FNarrativeTrackEditor::MakeSectionInterface(UMovieSceneSection& SectionObject, UMovieSceneTrack& Track, FGuid ObjectBinding)
{
	if (SectionObject.IsA<UMovieSceneNarrativeTriggerSection>())
	{
		return MakeShared<FNarrativeTriggerSection>(SectionObject, GetSequencer());
	}

	if (SectionObject.IsA<UMovieSceneNarrativeRepeaterSection>())
	{
		return MakeShared<FNarrativeRepeaterSection>(SectionObject, GetSequencer());
	}

	return MakeShared<FSequencerSection>(SectionObject);
}

FNarrativeTrackEditor::FNarrativeTrackEditor(TSharedRef<ISequencer> InSequencer)
	: FMovieSceneTrackEditor(InSequencer)
{
}

void FNarrativeTrackEditor::AddNarrativeSubMenu(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.AddMenuEntry(
		LOCTEXT("AddNewTriggerSection", "Narrative Trigger"),
		LOCTEXT("AddNewTriggerSectionTooltip", "Adds a new section that can trigger a Narrative event at a specific time"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateRaw(this, &FNarrativeTrackEditor::HandleAddNarrativeTrackMenuEntryExecute, UMovieSceneNarrativeTriggerSection::StaticClass())
		)
	);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("AddNewRepeaterSection", "Narrative Repeater"),
		LOCTEXT("AddNewRepeaterSectionTooltip", "Adds a new section that triggers a Narrative event every time it's evaluated"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateRaw(this, &FNarrativeTrackEditor::HandleAddNarrativeTrackMenuEntryExecute, UMovieSceneNarrativeRepeaterSection::StaticClass())
		)
	);
}

void FNarrativeTrackEditor::BuildAddTrackMenu(FMenuBuilder& MenuBuilder)
{
	UMovieSceneSequence* RootMovieSceneSequence = GetSequencer()->GetRootMovieSceneSequence();
	const FMovieSceneSequenceEditor* SequenceEditor = FMovieSceneSequenceEditor::Find(RootMovieSceneSequence);

	if (SequenceEditor && SequenceEditor->SupportsEvents(RootMovieSceneSequence))
	{
		MenuBuilder.AddSubMenu(
			LOCTEXT("AddTrack", "Narrative Track"),
			LOCTEXT("AddTooltip", "Adds a new flow track that can trigger events in the Narrative graph."),
			FNewMenuDelegate::CreateRaw(this, &FNarrativeTrackEditor::AddNarrativeSubMenu),
			false,
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Sequencer.Tracks.Event")
		);
	}
}

TSharedPtr<SWidget> FNarrativeTrackEditor::BuildOutlinerEditWidget(const FGuid& ObjectBinding, UMovieSceneTrack* Track, const FBuildEditWidgetParams& Params)
{
	check(Track);

	const TSharedPtr<ISequencer> SequencerPtr = GetSequencer();
	if (!SequencerPtr.IsValid())
	{
		return SNullWidget::NullWidget;
	}

	TWeakObjectPtr<UMovieSceneTrack> WeakTrack = Track;
	const int32 RowIndex = Params.TrackInsertRowIndex;
	auto SubMenuCallback = [this, WeakTrack, RowIndex]
	{
		FMenuBuilder MenuBuilder(true, nullptr);

		UMovieSceneTrack* TrackPtr = WeakTrack.Get();
		if (TrackPtr)
		{
			MenuBuilder.AddMenuEntry(
				LOCTEXT("AddNewTriggerSection", "Narrative Trigger"),
				LOCTEXT("AddNewTriggerSectionTooltip", "Adds a new section that can trigger a Narrative event at a specific time"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &FNarrativeTrackEditor::CreateNewSection, TrackPtr, RowIndex + 1, UMovieSceneNarrativeTriggerSection::StaticClass(), true))
			);

			MenuBuilder.AddMenuEntry(
				LOCTEXT("AddNewRepeaterSection", "Narrative Repeater"),
				LOCTEXT("AddNewRepeaterSectionTooltip", "Adds a new section that triggers a Narrative event every time it's evaluated"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &FNarrativeTrackEditor::CreateNewSection, TrackPtr, RowIndex + 1, UMovieSceneNarrativeRepeaterSection::StaticClass(), true))
			);
		}
		else
		{
			MenuBuilder.AddWidget(SNew(STextBlock).Text(LOCTEXT("InvalidTrack", "Track is no longer valid")), FText(), true);
		}

		return MenuBuilder.MakeWidget();
	};

	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			FSequencerUtilities::MakeAddButton(LOCTEXT("AddSection", "Section"), FOnGetContent::CreateLambda(SubMenuCallback), Params.NodeIsHovered, GetSequencer())
		];
}

bool FNarrativeTrackEditor::SupportsType(TSubclassOf<UMovieSceneTrack> Type) const
{
	return (Type == UMovieSceneNarrativeTrack::StaticClass());
}

bool FNarrativeTrackEditor::SupportsSequence(UMovieSceneSequence* InSequence) const
{
	return InSequence && InSequence->GetClass()->IsChildOf(ULevelSequence::StaticClass());
}

const FSlateBrush* FNarrativeTrackEditor::GetIconBrush() const
{
	return FAppStyle::GetBrush("Sequencer.Tracks.Event");
}

void FNarrativeTrackEditor::HandleAddNarrativeTrackMenuEntryExecute(UClass* SectionType) const
{
	UMovieScene* FocusedMovieScene = GetFocusedMovieScene();

	if (FocusedMovieScene == nullptr)
	{
		return;
	}

	if (FocusedMovieScene->IsReadOnly())
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("AddNarrativeTrack_Transaction", "Add Narrative Track"));
	FocusedMovieScene->Modify();

	TArray<UMovieSceneNarrativeTrack*> NewTracks;

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 2
	UMovieSceneNarrativeTrack* NewMasterTrack = FocusedMovieScene->AddMasterTrack<UMovieSceneNarrativeTrack>();
#else
	UMovieSceneNarrativeTrack* NewMasterTrack = FocusedMovieScene->AddTrack<UMovieSceneNarrativeTrack>();
#endif

	NewTracks.Add(NewMasterTrack);
	if (GetSequencer().IsValid())
	{
		GetSequencer()->OnAddTrack(NewMasterTrack, FGuid());
	}

	check(NewTracks.Num() != 0);

	for (UMovieSceneNarrativeTrack* NewTrack : NewTracks)
	{
		CreateNewSection(NewTrack, 0, SectionType, false);
		NewTrack->SetDisplayName(LOCTEXT("TrackName", "Narrative Events"));
	}
}

void FNarrativeTrackEditor::CreateNewSection(UMovieSceneTrack* Track, const int32 RowIndex, UClass* SectionType, const bool bSelect) const
{
	const TSharedPtr<ISequencer> SequencerPtr = GetSequencer();
	if (SequencerPtr.IsValid())
	{
		const UMovieScene* FocusedMovieScene = GetFocusedMovieScene();
		const FQualifiedFrameTime CurrentTime = SequencerPtr->GetLocalTime();

		FScopedTransaction Transaction(LOCTEXT("CreateNewNarrativeSectionTransactionText", "Add Narrative Section"));

		UMovieSceneSection* NewSection = NewObject<UMovieSceneSection>(Track, SectionType);
		check(NewSection);

		int32 OverlapPriority = 0;
		for (UMovieSceneSection* Section : Track->GetAllSections())
		{
			if (Section->GetRowIndex() >= RowIndex)
			{
				Section->SetRowIndex(Section->GetRowIndex() + 1);
			}
			OverlapPriority = FMath::Max(Section->GetOverlapPriority() + 1, OverlapPriority);
		}

		Track->Modify();

		if (SectionType == UMovieSceneNarrativeTriggerSection::StaticClass())
		{
			NewSection->SetRange(TRange<FFrameNumber>::All());
		}
		else
		{
			TRange<FFrameNumber> NewSectionRange;

			if (CurrentTime.Time.FrameNumber < FocusedMovieScene->GetPlaybackRange().GetUpperBoundValue())
			{
				NewSectionRange = TRange<FFrameNumber>(CurrentTime.Time.FrameNumber, FocusedMovieScene->GetPlaybackRange().GetUpperBoundValue());
			}
			else
			{
				constexpr float DefaultLengthInSeconds = 5.f;
				NewSectionRange = TRange<FFrameNumber>(CurrentTime.Time.FrameNumber, CurrentTime.Time.FrameNumber + (DefaultLengthInSeconds * SequencerPtr->GetFocusedTickResolution()).FloorToFrame());
			}

			NewSection->SetRange(NewSectionRange);
		}

		NewSection->SetOverlapPriority(OverlapPriority);
		NewSection->SetRowIndex(RowIndex);

		Track->AddSection(*NewSection);
		Track->UpdateEasing();

		if (bSelect)
		{
			SequencerPtr->EmptySelection();
			SequencerPtr->SelectSection(NewSection);
			SequencerPtr->ThrobSectionSelection();
		}

		SequencerPtr->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemAdded);
	}
}

#undef LOCTEXT_NAMESPACE
