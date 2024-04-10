// Copyright XiaoYao

#pragma once

#include "SGraphPalette.h"

class FNarrativeAssetEditor;

/** Widget displaying a single item  */
class NARRATIVEEDITOR_API SNarrativePaletteItem : public SGraphPaletteItem
{
public:
	SLATE_BEGIN_ARGS(SNarrativePaletteItem) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, FCreateWidgetForActionData* const InCreateData);

private:
	TSharedRef<SWidget> CreateHotkeyDisplayWidget(const FSlateFontInfo& NameFont, const TSharedPtr<const FInputChord> HotkeyChord) const;
	virtual FText GetItemTooltip() const override;
};

/** Narrative Palette  */
class NARRATIVEEDITOR_API SNarrativePalette : public SGraphPalette
{
public:
	SLATE_BEGIN_ARGS(SNarrativePalette) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TWeakPtr<FNarrativeAssetEditor> InNarrativeAssetEditor);
	virtual ~SNarrativePalette() override;

protected:
	void Refresh();
	void UpdateCategoryNames();

	// SGraphPalette
	virtual TSharedRef<SWidget> OnCreateWidgetForAction(FCreateWidgetForActionData* const InCreateData) override;
	virtual void CollectAllActions(FGraphActionListBuilderBase& OutAllActions) override;
	// --

	FString GetFilterCategoryName() const;
	void CategorySelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);

	void OnActionSelected(const TArray<TSharedPtr<FEdGraphSchemaAction>>& InActions, ESelectInfo::Type InSelectionType) const;

public:
	void ClearGraphActionMenuSelection() const;

protected:
	TWeakPtr<FNarrativeAssetEditor> NarrativeAssetEditor;
	TArray<TSharedPtr<FString>> CategoryNames;
	TSharedPtr<STextComboBox> CategoryComboBox;
};
