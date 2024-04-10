// Copyright XiaoYao

#include "DetailCustomizations/NarrativeNode_CustomEventBaseDetails.h"
#include "NarrativeAsset.h"
#include "Nodes/Route/NarrativeNode_CustomEventBase.h"

#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "PropertyEditing.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SWidget.h"

void FNarrativeNode_CustomEventBaseDetails::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	// Subclasses must override this function (and call CustomizeDetailsInternal with the localized text)
	checkNoEntry();
}

void FNarrativeNode_CustomEventBaseDetails::CustomizeDetailsInternal(IDetailLayoutBuilder& DetailLayout, const FText& CustomRowNameText, const FText& EventNameText)
{
	DetailLayout.GetObjectsBeingCustomized(ObjectsBeingEdited);

	if (ObjectsBeingEdited[0].IsValid())
	{
		const UNarrativeNode_CustomEventBase* EventNode = CastChecked<UNarrativeNode_CustomEventBase>(ObjectsBeingEdited[0]);
		CachedEventNameSelected = MakeShared<FName>(EventNode->GetEventName());
	}

	IDetailCategoryBuilder& Category = CreateDetailCategory(DetailLayout);

	Category.AddCustomRow(CustomRowNameText)
	        .NameContent()
			[
				SNew(STextBlock)
				.Text(EventNameText)
			]
			.ValueContent()
			.HAlign(HAlign_Fill)
			[
				SAssignNew(EventTextListWidget, SComboBox<TSharedPtr<FName>>)
								.OptionsSource(&EventNames)
								.OnGenerateWidget(this, &FNarrativeNode_CustomEventBaseDetails::GenerateEventWidget)
								.OnComboBoxOpening(this, &FNarrativeNode_CustomEventBaseDetails::OnComboBoxOpening)
								.OnSelectionChanged(this, &FNarrativeNode_CustomEventBaseDetails::PinSelectionChanged)
								[
									SNew(STextBlock)
									.Text(this, &FNarrativeNode_CustomEventBaseDetails::GetSelectedEventText)
								]
			];
}

void FNarrativeNode_CustomEventBaseDetails::OnComboBoxOpening()
{
	RebuildEventNames();
}

void FNarrativeNode_CustomEventBaseDetails::RebuildEventNames()
{
	EventNames.Empty();

	check(CachedEventNameSelected.IsValid());
	EventNames.Add(CachedEventNameSelected);

	if (ObjectsBeingEdited[0].IsValid() && ObjectsBeingEdited[0].Get()->GetOuter())
	{
		const UNarrativeAsset* NarrativeAsset = CastChecked<UNarrativeAsset>(ObjectsBeingEdited[0].Get()->GetOuter());
		TArray<FName> SortedNames = BuildEventNames(*NarrativeAsset);

		if (bExcludeReferencedEvents)
		{
			for (const TPair<FGuid, UNarrativeNode*>& Node : NarrativeAsset->GetNodes())
			{
				if (Node.Value->GetClass()->IsChildOf(UNarrativeNode_CustomEventBase::StaticClass()))
				{
					SortedNames.Remove(Cast<UNarrativeNode_CustomEventBase>(Node.Value)->GetEventName());
				}
			}
		}

		SortedNames.Sort([](const FName& A, const FName& B)
		{
			return A.LexicalLess(B);
		});

		for (const FName& EventName : SortedNames)
		{
			const bool bIsCurrentSelection = (EventName == *CachedEventNameSelected);
			if (!EventName.IsNone() && !bIsCurrentSelection)
			{
				EventNames.Add(MakeShared<FName>(EventName));
			}
		}
	}

	if (!IsInEventNames(NAME_None))
	{
		EventNames.Add(MakeShared<FName>(NAME_None));
	}
}

bool FNarrativeNode_CustomEventBaseDetails::IsInEventNames(const FName& EventName) const
{
	const bool bIsInEventNames = EventNames.ContainsByPredicate([&EventName](const TSharedPtr<FName>& ExistingName)
	{
		return *ExistingName == EventName;
	});

	return bIsInEventNames;
}

TSharedRef<SWidget> FNarrativeNode_CustomEventBaseDetails::GenerateEventWidget(const TSharedPtr<FName> Item) const
{
	return SNew(STextBlock)
		.Text(FText::FromName(*Item.Get()));
}

FText FNarrativeNode_CustomEventBaseDetails::GetSelectedEventText() const
{
	check(CachedEventNameSelected.IsValid());
	return FText::FromName(*CachedEventNameSelected.Get());
}

void FNarrativeNode_CustomEventBaseDetails::PinSelectionChanged(const TSharedPtr<FName> Item, ESelectInfo::Type SelectInfo)
{
	ensure(ObjectsBeingEdited[0].IsValid());

	UNarrativeNode_CustomEventBase* Node = Cast<UNarrativeNode_CustomEventBase>(ObjectsBeingEdited[0].Get());
	if (IsValid(Node) && Item)
	{
		const bool bIsChanged = (*CachedEventNameSelected != *Item);

		if (bIsChanged)
		{
			CachedEventNameSelected = Item;

			const FName ItemAsName = *CachedEventNameSelected;
			Node->SetEventName(ItemAsName);

			if (EventTextListWidget.IsValid())
			{
				// Tell UDE to refresh the widget to show the new change
				EventTextListWidget->Invalidate(EInvalidateWidgetReason::Paint);
			}
		}
	}
}
