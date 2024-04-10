// Copyright XiaoYao

#include "Graph/Widgets/SNarrativePalette.h"
#include "Asset/NarrativeAssetEditor.h"
#include "NarrativeEditorCommands.h"
#include "Graph/NarrativeGraphSchema.h"
#include "Graph/NarrativeGraphSchema_Actions.h"

#include "NarrativeAsset.h"
#include "Nodes/NarrativeNode.h"

#include "Fonts/SlateFontInfo.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateColor.h"
#include "Widgets/Input/STextComboBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "NarrativeGraphPalette"

void SNarrativePaletteItem::Construct(const FArguments& InArgs, FCreateWidgetForActionData* const InCreateData)
{
	const FSlateFontInfo NameFont = FCoreStyle::GetDefaultFontStyle("Regular", 10);

	check(InCreateData->Action.IsValid());

	const TSharedPtr<FEdGraphSchemaAction> GraphAction = InCreateData->Action;
	ActionPtr = InCreateData->Action;

	// Get the hotkey chord if one exists for this action
	TSharedPtr<const FInputChord> HotkeyChord;

	if (FNarrativeSpawnNodeCommands::IsRegistered())
	{
		if (GraphAction->GetTypeId() == FNarrativeGraphSchemaAction_NewNode::StaticGetTypeId())
		{
			UClass* NarrativeNodeClass = StaticCastSharedPtr<FNarrativeGraphSchemaAction_NewNode>(GraphAction)->NodeClass;
			HotkeyChord = FNarrativeSpawnNodeCommands::Get().GetChordByClass(NarrativeNodeClass);
		}
		else if (GraphAction->GetTypeId() == FNarrativeGraphSchemaAction_NewComment::StaticGetTypeId())
		{
			HotkeyChord = FNarrativeSpawnNodeCommands::Get().GetChordByClass(UNarrativeNode::StaticClass());
		}
	}

	// Find icons
	const FSlateBrush* IconBrush = FAppStyle::GetBrush(TEXT("NoBrush"));
	const FSlateColor IconColor = FSlateColor::UseForeground();
	const FText IconToolTip = GraphAction->GetTooltipDescription();
	constexpr bool bIsReadOnly = false;

	const TSharedRef<SWidget> IconWidget = CreateIconWidget(IconToolTip, IconBrush, IconColor);
	const TSharedRef<SWidget> NameSlotWidget = CreateTextSlotWidget(InCreateData, bIsReadOnly);
	const TSharedRef<SWidget> HotkeyDisplayWidget = CreateHotkeyDisplayWidget(NameFont, HotkeyChord);

	// Create the actual widget
	this->ChildSlot
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				IconWidget
			]
		+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.VAlign(VAlign_Center)
			.Padding(3, 0)
			[
				NameSlotWidget
			]
		+ SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Right)
			[
				HotkeyDisplayWidget
			]
	];
}

TSharedRef<SWidget> SNarrativePaletteItem::CreateHotkeyDisplayWidget(const FSlateFontInfo& NameFont, const TSharedPtr<const FInputChord> HotkeyChord) const
{
	FText HotkeyText;
	if (HotkeyChord.IsValid())
	{
		HotkeyText = HotkeyChord->GetInputText();
	}

	return SNew(STextBlock)
		.Text(HotkeyText)
		.Font(NameFont);
}

FText SNarrativePaletteItem::GetItemTooltip() const
{
	return ActionPtr.Pin()->GetTooltipDescription();
}

void SNarrativePalette::Construct(const FArguments& InArgs, TWeakPtr<FNarrativeAssetEditor> InNarrativeAssetEditor)
{
	NarrativeAssetEditor = InNarrativeAssetEditor;

	UpdateCategoryNames();
	UNarrativeGraphSchema::OnNodeListChanged.AddSP(this, &SNarrativePalette::Refresh);

	struct LocalUtils
	{
		static TSharedRef<SExpanderArrow> CreateCustomExpanderStatic(const FCustomExpanderData& ActionMenuData, bool bShowFavoriteToggle)
		{
			TSharedPtr<SExpanderArrow> CustomExpander;
			// in SBlueprintSubPalette here would be a difference depending on bShowFavoriteToggle
			SAssignNew(CustomExpander, SExpanderArrow, ActionMenuData.TableRow);
			return CustomExpander.ToSharedRef();
		}
	};

	this->ChildSlot
	[
		SNew(SBorder)
			.Padding(2.0f)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot() // Filter UI
					.AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
							.VAlign(VAlign_Center)
							[
								SAssignNew(CategoryComboBox, STextComboBox)
									.OptionsSource(&CategoryNames)
									.OnSelectionChanged(this, &SNarrativePalette::CategorySelectionChanged)
									.InitiallySelectedItem(CategoryNames[0])
							]
					]
				+ SVerticalBox::Slot() // Content list
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Fill)
					[
						SAssignNew(GraphActionMenu, SGraphActionMenu)
							.OnActionDragged(this, &SNarrativePalette::OnActionDragged)
							.OnActionSelected(this, &SNarrativePalette::OnActionSelected)
							.OnCreateWidgetForAction(this, &SNarrativePalette::OnCreateWidgetForAction)
							.OnCollectAllActions(this, &SNarrativePalette::CollectAllActions)
							.OnCreateCustomRowExpander_Static(&LocalUtils::CreateCustomExpanderStatic, false)
							.AutoExpandActionMenu(true)
					]
			]
	];
}

SNarrativePalette::~SNarrativePalette()
{
	UNarrativeGraphSchema::OnNodeListChanged.RemoveAll(this);
}

void SNarrativePalette::Refresh()
{
	const FString LastSelectedCategory = CategoryComboBox->GetSelectedItem().IsValid() ? *CategoryComboBox->GetSelectedItem().Get() : FString();

	UpdateCategoryNames();
	RefreshActionsList(true);

	// refresh list of category and currently selected category
	CategoryComboBox->RefreshOptions();
	TSharedPtr<FString> SelectedCategory = CategoryNames[0];
	if (!LastSelectedCategory.IsEmpty())
	{
		for (const TSharedPtr<FString>& CategoryName : CategoryNames)
		{
			if (*CategoryName.Get() == LastSelectedCategory)
			{
				SelectedCategory = CategoryName;
				break;
			}
		}
	}
	CategoryComboBox->SetSelectedItem(SelectedCategory);
}

void SNarrativePalette::UpdateCategoryNames()
{
	CategoryNames = {MakeShareable(new FString(TEXT("All")))};
	CategoryNames.Append(UNarrativeGraphSchema::GetNarrativeNodeCategories());
}

TSharedRef<SWidget> SNarrativePalette::OnCreateWidgetForAction(FCreateWidgetForActionData* const InCreateData)
{
	return SNew(SNarrativePaletteItem, InCreateData);
}

void SNarrativePalette::CollectAllActions(FGraphActionListBuilderBase& OutAllActions)
{
	ensureAlways(NarrativeAssetEditor.Pin() && NarrativeAssetEditor.Pin()->GetNarrativeAsset());
	const UNarrativeAsset* EditedNarrativeAsset = NarrativeAssetEditor.Pin()->GetNarrativeAsset();
	
	FGraphActionMenuBuilder ActionMenuBuilder;
	UNarrativeGraphSchema::GetPaletteActions(ActionMenuBuilder, EditedNarrativeAsset, GetFilterCategoryName());
	OutAllActions.Append(ActionMenuBuilder);
}

FString SNarrativePalette::GetFilterCategoryName() const
{
	if (CategoryComboBox.IsValid() && CategoryComboBox->GetSelectedItem() != CategoryNames[0])
	{
		return *CategoryComboBox->GetSelectedItem();
	}

	return FString();
}

void SNarrativePalette::CategorySelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	RefreshActionsList(true);
}

void SNarrativePalette::OnActionSelected(const TArray<TSharedPtr<FEdGraphSchemaAction>>& InActions, ESelectInfo::Type InSelectionType) const
{
	if (InSelectionType == ESelectInfo::OnMouseClick || InSelectionType == ESelectInfo::OnKeyPress || InSelectionType == ESelectInfo::OnNavigation || InActions.Num() == 0)
	{
		NarrativeAssetEditor.Pin()->SetUISelectionState(FNarrativeAssetEditor::PaletteTab);
	}
}

void SNarrativePalette::ClearGraphActionMenuSelection() const
{
	GraphActionMenu->SelectItemByName(NAME_None);
}

#undef LOCTEXT_NAMESPACE
