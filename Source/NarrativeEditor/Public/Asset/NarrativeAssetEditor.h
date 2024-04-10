// Copyright XiaoYao

#pragma once

#include "EditorUndoClient.h"
#include "Misc/NotifyHook.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "Toolkits/IToolkitHost.h"
#include "UObject/GCObject.h"

#include "NarrativeEditorDefines.h"

class FNarrativeMessageLog;
class SNarrativeGraphEditor;
class SNarrativePalette;
class UNarrativeAsset;
class UNarrativeGraphNode;

class IDetailsView;
class SDockableTab;
class SGraphEditor;
class UEdGraphNode;
struct FSlateBrush;
struct FPropertyChangedEvent;
struct Rect;

class NARRATIVEEDITOR_API FNarrativeAssetEditor : public FAssetEditorToolkit, public FEditorUndoClient, public FGCObject, public FNotifyHook
{
public:
	/**	The tab ids for all the tabs used */
	static const FName DetailsTab;
	static const FName GraphTab;
	static const FName PaletteTab;
	static const FName RuntimeLogTab;
	static const FName SearchTab;
	static const FName ValidationLogTab;
	
protected:
	/** The Narrative Asset being edited */
	TObjectPtr<UNarrativeAsset> NarrativeAsset;

	TSharedPtr<class FNarrativeAssetToolbar> AssetToolbar;

	TSharedPtr<SNarrativeGraphEditor> GraphEditor;
	TSharedPtr<class IDetailsView> DetailsView;
	TSharedPtr<class SNarrativePalette> Palette;

#if ENABLE_SEARCH_IN_ASSET_EDITOR
	TSharedPtr<class SSearchBrowser> SearchBrowser;
#endif

	/** Runtime message log, with the log listing that it reflects */
	TSharedPtr<class SWidget> RuntimeLog;
	TSharedPtr<class IMessageLogListing> RuntimeLogListing;

	/** Asset Validation message log, with the log listing that it reflects */
	TSharedPtr<class SWidget> ValidationLog;
	TSharedPtr<class IMessageLogListing> ValidationLogListing;

private:
	/** The current UI selection state of this editor */
	FName CurrentUISelection;

public:
	FNarrativeAssetEditor();
	virtual ~FNarrativeAssetEditor() override;

	UNarrativeAsset* GetNarrativeAsset() const { return NarrativeAsset; }
	TSharedPtr<SNarrativeGraphEditor> GetNarrativeGraph() const { return GraphEditor; }

	// FGCObject
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

	virtual FString GetReferencerName() const override
	{
		return TEXT("FNarrativeAssetEditor");
	}
	// --

	// FEditorUndoClient
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override;
	// --

	virtual void HandleUndoTransaction();

	// FNotifyHook
	virtual void NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged) override;
	// --

	// IToolkit
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;

	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;
	// --

	// FAssetEditorToolkit
	virtual void InitToolMenuContext(FToolMenuContext& MenuContext) override;
	virtual void PostRegenerateMenusAndToolbars() override;
	// --

	bool IsTabFocused(const FTabId& TabId) const;

private:
	TSharedRef<SDockTab> SpawnTab_Details(const FSpawnTabArgs& Args) const;
	TSharedRef<SDockTab> SpawnTab_Graph(const FSpawnTabArgs& Args) const;
	TSharedRef<SDockTab> SpawnTab_Palette(const FSpawnTabArgs& Args) const;
	TSharedRef<SDockTab> SpawnTab_RuntimeLog(const FSpawnTabArgs& Args) const;
#if ENABLE_SEARCH_IN_ASSET_EDITOR
	TSharedRef<SDockTab> SpawnTab_Search(const FSpawnTabArgs& Args) const;
#endif
	TSharedRef<SDockTab> SpawnTab_ValidationLog(const FSpawnTabArgs& Args) const;

public:
	/** Edits the specified NarrativeAsset object */
	void InitNarrativeAssetEditor(const EToolkitMode::Type Mode, const TSharedPtr<class IToolkitHost>& InitToolkitHost, UObject* ObjectToEdit);

protected:
	virtual void CreateToolbar();
	virtual void BindToolbarCommands();
	
	virtual void RefreshAsset();

private:
	void ValidateAsset_Internal();

protected:
	virtual void ValidateAsset(FNarrativeMessageLog& MessageLog);

#if ENABLE_SEARCH_IN_ASSET_EDITOR
	virtual void SearchInAsset();
#endif

	void EditAssetDefaults_Clicked() const;

	virtual void GoToParentInstance();
	virtual bool CanGoToParentInstance();

	virtual void CreateWidgets();
	virtual void CreateGraphWidget();

	static bool CanEdit();

public:
	void SetUISelectionState(const FName SelectionOwner);
	virtual void ClearSelectionStateFor(const FName SelectionOwner);
	FName GetUISelectionState() const;

	virtual void OnSelectedNodesChanged(const TSet<UObject*>& Nodes) {}

#if ENABLE_JUMP_TO_INNER_OBJECT
	// FAssetEditorToolkit
	virtual void JumpToInnerObject(UObject* InnerObject) override;
	// --
#endif

protected:
	void OnLogTokenClicked(const TSharedRef<class IMessageToken>& Token) const;
};
