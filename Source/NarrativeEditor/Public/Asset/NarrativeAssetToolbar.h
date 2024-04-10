// Copyright XiaoYao

#pragma once

#include "Widgets/Input/SComboBox.h"
#include "Widgets/Navigation/SBreadcrumbTrail.h"

#include "NarrativeAsset.h"

class FNarrativeAssetEditor;
class UToolMenu;

//////////////////////////////////////////////////////////////////////////
// Narrative Asset Instance List

class NARRATIVEEDITOR_API SNarrativeAssetInstanceList : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SNarrativeAssetInstanceList) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TWeakObjectPtr<UNarrativeAsset> InTemplateAsset);
	virtual ~SNarrativeAssetInstanceList() override;

	static EVisibility GetDebuggerVisibility();

private:
	void RefreshInstances();

	TSharedRef<SWidget> OnGenerateWidget(TSharedPtr<FName> Item) const;
	void OnSelectionChanged(TSharedPtr<FName> SelectedItem, ESelectInfo::Type SelectionType);
	FText GetSelectedInstanceName() const;

	TWeakObjectPtr<UNarrativeAsset> TemplateAsset;
	TSharedPtr<SComboBox<TSharedPtr<FName>>> Dropdown;

	TArray<TSharedPtr<FName>> InstanceNames;
	TSharedPtr<FName> SelectedInstance;

	static FText NoInstanceSelectedText;
};

//////////////////////////////////////////////////////////////////////////
// Narrative Asset Breadcrumb

/**
 * The kind of breadcrumbs that Narrative Debugger uses
 */
struct NARRATIVEEDITOR_API FNarrativeBreadcrumb
{
	const FString AssetPathName;
	const FName InstanceName;

	FNarrativeBreadcrumb()
		: AssetPathName(FString())
		, InstanceName(NAME_None)
	{}

	explicit FNarrativeBreadcrumb(const TWeakObjectPtr<UNarrativeAsset> NarrativeAsset)
		: AssetPathName(NarrativeAsset->GetTemplateAsset()->GetPathName())
		, InstanceName(NarrativeAsset->GetDisplayName())
	{}
};

class NARRATIVEEDITOR_API SNarrativeAssetBreadcrumb : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SNarrativeAssetInstanceList) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TWeakObjectPtr<UNarrativeAsset> InTemplateAsset);

private:
	void OnCrumbClicked(const FNarrativeBreadcrumb& Item) const;

	TWeakObjectPtr<UNarrativeAsset> TemplateAsset;
	TSharedPtr<SBreadcrumbTrail<FNarrativeBreadcrumb>> BreadcrumbTrail;
};

//////////////////////////////////////////////////////////////////////////
// Narrative Asset Toolbar

class NARRATIVEEDITOR_API FNarrativeAssetToolbar : public TSharedFromThis<FNarrativeAssetToolbar>
{
public:
	explicit FNarrativeAssetToolbar(const TSharedPtr<FNarrativeAssetEditor> InAssetEditor, UToolMenu* ToolbarMenu);

private:
	void BuildAssetToolbar(UToolMenu* ToolbarMenu) const;
	TSharedRef<SWidget> MakeDiffMenu() const;
	
	void BuildDebuggerToolbar(UToolMenu* ToolbarMenu) const;

private:
	TWeakPtr<FNarrativeAssetEditor> NarrativeAssetEditor;
};
