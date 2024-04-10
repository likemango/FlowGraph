// Copyright XiaoYao

#include "Asset/NarrativeAssetEditor.h"

#include "NarrativeEditorCommands.h"
#include "NarrativeEditorLogChannels.h"
#include "NarrativeMessageLog.h"

#include "Asset/NarrativeAssetEditorContext.h"
#include "Asset/NarrativeAssetToolbar.h"
#include "Asset/NarrativeMessageLogListing.h"
#include "Graph/NarrativeGraphEditor.h"
#include "Graph/NarrativeGraphSchema.h"
#include "Graph/Widgets/SNarrativePalette.h"

#include "NarrativeAsset.h"

#include "EdGraph/EdGraphNode.h"
#include "Editor.h"
#include "EditorClassUtils.h"
#include "GraphEditor.h"
#include "IDetailsView.h"
#include "IMessageLogListing.h"
#include "Kismet2/DebuggerCommands.h"
#include "MessageLogModule.h"
#include "Misc/UObjectToken.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"

#if ENABLE_SEARCH_IN_ASSET_EDITOR
#include "Source/Private/Widgets/SSearchBrowser.h"
#endif

#define LOCTEXT_NAMESPACE "NarrativeAssetEditor"

const FName FNarrativeAssetEditor::DetailsTab(TEXT("Details"));
const FName FNarrativeAssetEditor::GraphTab(TEXT("Graph"));
const FName FNarrativeAssetEditor::PaletteTab(TEXT("Palette"));
const FName FNarrativeAssetEditor::RuntimeLogTab(TEXT("RuntimeLog"));
const FName FNarrativeAssetEditor::SearchTab(TEXT("Search"));
const FName FNarrativeAssetEditor::ValidationLogTab(TEXT("ValidationLog"));

FNarrativeAssetEditor::FNarrativeAssetEditor()
	: NarrativeAsset(nullptr)
{
}

FNarrativeAssetEditor::~FNarrativeAssetEditor()
{
	GEditor->UnregisterForUndo(this);
}

void FNarrativeAssetEditor::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(NarrativeAsset);
}

void FNarrativeAssetEditor::PostUndo(bool bSuccess)
{
	HandleUndoTransaction();
}

void FNarrativeAssetEditor::PostRedo(bool bSuccess)
{
	HandleUndoTransaction();
}

void FNarrativeAssetEditor::HandleUndoTransaction()
{
	SetUISelectionState(NAME_None);
	GraphEditor->NotifyGraphChanged();
	FSlateApplication::Get().DismissAllMenus();
}

void FNarrativeAssetEditor::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged)
{
	if (PropertyChangedEvent.ChangeType != EPropertyChangeType::Interactive)
	{
		GraphEditor->NotifyGraphChanged();
	}
}

FName FNarrativeAssetEditor::GetToolkitFName() const
{
	return FName("NarrativeEditor");
}

FText FNarrativeAssetEditor::GetBaseToolkitName() const
{
	return LOCTEXT("AppLabel", "NarrativeAsset Editor");
}

FString FNarrativeAssetEditor::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("WorldCentricTabPrefix", "NarrativeAsset").ToString();
}

FLinearColor FNarrativeAssetEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor(0.3f, 0.2f, 0.5f, 0.5f);
}

void FNarrativeAssetEditor::RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_NarrativeAssetEditor", "Narrative Editor"));
	const auto WorkspaceMenuCategoryRef = WorkspaceMenuCategory.ToSharedRef();

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner(DetailsTab, FOnSpawnTab::CreateSP(this, &FNarrativeAssetEditor::SpawnTab_Details))
				.SetDisplayName(LOCTEXT("DetailsTab", "Details"))
				.SetGroup(WorkspaceMenuCategoryRef)
				.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));

	InTabManager->RegisterTabSpawner(GraphTab, FOnSpawnTab::CreateSP(this, &FNarrativeAssetEditor::SpawnTab_Graph))
				.SetDisplayName(LOCTEXT("GraphTab", "Graph"))
				.SetGroup(WorkspaceMenuCategoryRef)
				.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "GraphEditor.EventGraph_16x"));

	InTabManager->RegisterTabSpawner(PaletteTab, FOnSpawnTab::CreateSP(this, &FNarrativeAssetEditor::SpawnTab_Palette))
				.SetDisplayName(LOCTEXT("PaletteTab", "Palette"))
				.SetGroup(WorkspaceMenuCategoryRef)
				.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Kismet.Tabs.Palette"));

	InTabManager->RegisterTabSpawner(RuntimeLogTab, FOnSpawnTab::CreateSP(this, &FNarrativeAssetEditor::SpawnTab_RuntimeLog))
				.SetDisplayName(LOCTEXT("RuntimeLog", "Runtime Log"))
				.SetGroup(WorkspaceMenuCategoryRef)
				.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Kismet.Tabs.CompilerResults"));

#if ENABLE_SEARCH_IN_ASSET_EDITOR
	InTabManager->RegisterTabSpawner(SearchTab, FOnSpawnTab::CreateSP(this, &FNarrativeAssetEditor::SpawnTab_Search))
				.SetDisplayName(LOCTEXT("SearchTab", "Search"))
				.SetGroup(WorkspaceMenuCategoryRef)
				.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Kismet.Tabs.FindResults"));
#endif

	InTabManager->RegisterTabSpawner(ValidationLogTab, FOnSpawnTab::CreateSP(this, &FNarrativeAssetEditor::SpawnTab_ValidationLog))
				.SetDisplayName(LOCTEXT("ValidationLog", "Validation Log"))
				.SetGroup(WorkspaceMenuCategoryRef)
				.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Debug"));
}

void FNarrativeAssetEditor::UnregisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner(DetailsTab);
	InTabManager->UnregisterTabSpawner(GraphTab);
	InTabManager->UnregisterTabSpawner(ValidationLogTab);
	InTabManager->UnregisterTabSpawner(PaletteTab);
#if ENABLE_SEARCH_IN_ASSET_EDITOR
	InTabManager->UnregisterTabSpawner(SearchTab);
#endif
}

void FNarrativeAssetEditor::InitToolMenuContext(FToolMenuContext& MenuContext)
{
	FAssetEditorToolkit::InitToolMenuContext(MenuContext);

	UNarrativeAssetEditorContext* Context = NewObject<UNarrativeAssetEditorContext>();
	Context->NarrativeAssetEditor = SharedThis(this);
	MenuContext.AddObject(Context);
}

void FNarrativeAssetEditor::PostRegenerateMenusAndToolbars()
{
	// Provide a hyperlink to view our class
	const TSharedRef<SHorizontalBox> MenuOverlayBox = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			.ShadowOffset(FVector2D::UnitVector)
			.Text(LOCTEXT("NarrativeAssetEditor_AssetType", "Asset Type: "))
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0.0f, 0.0f, 8.0f, 0.0f)
		[
			FEditorClassUtils::GetSourceLink(NarrativeAsset->GetClass())
		];

	SetMenuOverlay(MenuOverlayBox);
}

bool FNarrativeAssetEditor::IsTabFocused(const FTabId& TabId) const
{
	if (const TSharedPtr<SDockTab> CurrentGraphTab = GetToolkitHost()->GetTabManager()->FindExistingLiveTab(TabId))
	{
		return CurrentGraphTab->IsActive();
	}

	return false;
}

TSharedRef<SDockTab> FNarrativeAssetEditor::SpawnTab_Details(const FSpawnTabArgs& Args) const
{
	check(Args.GetTabId() == DetailsTab);

	return SNew(SDockTab)
		.Label(LOCTEXT("NarrativeDetailsTitle", "Details"))
		[
			DetailsView.ToSharedRef()
		];
}

TSharedRef<SDockTab> FNarrativeAssetEditor::SpawnTab_Graph(const FSpawnTabArgs& Args) const
{
	check(Args.GetTabId() == GraphTab);

	TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab)
		.Label(LOCTEXT("NarrativeGraphTitle", "Graph"));

	if (GraphEditor.IsValid())
	{
		SpawnedTab->SetContent(GraphEditor.ToSharedRef());
	}

	return SpawnedTab;
}

TSharedRef<SDockTab> FNarrativeAssetEditor::SpawnTab_Palette(const FSpawnTabArgs& Args) const
{
	check(Args.GetTabId() == PaletteTab);

	return SNew(SDockTab)
		.Label(LOCTEXT("NarrativePaletteTitle", "Palette"))
		[
			Palette.ToSharedRef()
		];
}

TSharedRef<SDockTab> FNarrativeAssetEditor::SpawnTab_RuntimeLog(const FSpawnTabArgs& Args) const
{
	check(Args.GetTabId() == RuntimeLogTab);

	return SNew(SDockTab)
		.Label(LOCTEXT("NarrativeRuntimeLogTitle", "Runtime Log"))
		[
			RuntimeLog.ToSharedRef()
		];
}

#if ENABLE_SEARCH_IN_ASSET_EDITOR
TSharedRef<SDockTab> FNarrativeAssetEditor::SpawnTab_Search(const FSpawnTabArgs& Args) const
{
	check(Args.GetTabId() == SearchTab);

	return SNew(SDockTab)
		.Label(LOCTEXT("NarrativeSearchTitle", "Search"))
		[
			SNew(SBox)
			.AddMetaData<FTagMetaData>(FTagMetaData(TEXT("NarrativeSearch")))
			[
				SearchBrowser.ToSharedRef()
			]
		];
}
#endif

TSharedRef<SDockTab> FNarrativeAssetEditor::SpawnTab_ValidationLog(const FSpawnTabArgs& Args) const
{
	check(Args.GetTabId() == ValidationLogTab);

	return SNew(SDockTab)
		.Label(LOCTEXT("NarrativeValidationLogTitle", "Validation Log"))
		[
			ValidationLog.ToSharedRef()
		];
}

void FNarrativeAssetEditor::InitNarrativeAssetEditor(const EToolkitMode::Type Mode, const TSharedPtr<class IToolkitHost>& InitToolkitHost, UObject* ObjectToEdit)
{
	NarrativeAsset = CastChecked<UNarrativeAsset>(ObjectToEdit);

	// Support undo/redo
	NarrativeAsset->SetFlags(RF_Transactional);
	GEditor->RegisterForUndo(this);

	UNarrativeGraphSchema::SubscribeToAssetChanges();

	BindToolbarCommands();
	CreateToolbar();

	CreateWidgets();

	const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("NarrativeAssetEditor_Layout_v5.1")
		->AddArea
		(
			FTabManager::NewPrimaryArea()->SetOrientation(Orient_Horizontal)
										->Split
										(
											FTabManager::NewStack()
											->SetSizeCoefficient(0.225f)
											->AddTab(DetailsTab, ETabState::OpenedTab)
										)
										->Split
										(
											FTabManager::NewSplitter()
											->SetSizeCoefficient(0.65f)
											->SetOrientation(Orient_Vertical)
											->Split
											(
												FTabManager::NewStack()
												->SetSizeCoefficient(0.8f)
												->SetHideTabWell(true)
												->AddTab(GraphTab, ETabState::OpenedTab)
											)
											->Split
											(
												FTabManager::NewStack()
												->SetSizeCoefficient(0.15f)
												->AddTab(RuntimeLogTab, ETabState::ClosedTab)
											)
											->Split
											(
												FTabManager::NewStack()
												->SetSizeCoefficient(0.15f)
												->AddTab(SearchTab, ETabState::ClosedTab)
											)
											->Split
											(
												FTabManager::NewStack()
												->SetSizeCoefficient(0.15f)
												->AddTab(ValidationLogTab, ETabState::ClosedTab)
											)
										)
										->Split
										(
											FTabManager::NewStack()
											->SetSizeCoefficient(0.125f)
											->AddTab(PaletteTab, ETabState::OpenedTab)
										)
		);

	constexpr bool bCreateDefaultStandaloneMenu = true;
	constexpr bool bCreateDefaultToolbar = true;
	InitAssetEditor(Mode, InitToolkitHost, TEXT("NarrativeEditorApp"), StandaloneDefaultLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, ObjectToEdit, false);

	RegenerateMenusAndToolbars();
}

void FNarrativeAssetEditor::CreateToolbar()
{
	FName ParentToolbarName;
	const FName ToolBarName = GetToolMenuToolbarName(ParentToolbarName);

	UToolMenus* ToolMenus = UToolMenus::Get();
	UToolMenu* FoundMenu = ToolMenus->FindMenu(ToolBarName);
	if (!FoundMenu || !FoundMenu->IsRegistered())
	{
		FoundMenu = ToolMenus->RegisterMenu(ToolBarName, ParentToolbarName, EMultiBoxType::ToolBar);
	}

	if (FoundMenu)
	{
		AssetToolbar = MakeShareable(new FNarrativeAssetToolbar(SharedThis(this), FoundMenu));
	}
}

void FNarrativeAssetEditor::BindToolbarCommands()
{
	FNarrativeToolbarCommands::Register();
	const FNarrativeToolbarCommands& ToolbarCommands = FNarrativeToolbarCommands::Get();

	// Editing
	ToolkitCommands->MapAction(ToolbarCommands.RefreshAsset,
								FExecuteAction::CreateSP(this, &FNarrativeAssetEditor::RefreshAsset),
								FCanExecuteAction::CreateStatic(&FNarrativeAssetEditor::CanEdit));

	ToolkitCommands->MapAction(ToolbarCommands.ValidateAsset,
								FExecuteAction::CreateSP(this, &FNarrativeAssetEditor::ValidateAsset_Internal),
								FCanExecuteAction());

#if ENABLE_SEARCH_IN_ASSET_EDITOR
	ToolkitCommands->MapAction(ToolbarCommands.SearchInAsset,
								FExecuteAction::CreateSP(this, &FNarrativeAssetEditor::SearchInAsset),
								FCanExecuteAction());
#endif

	ToolkitCommands->MapAction(ToolbarCommands.EditAssetDefaults,
								FExecuteAction::CreateSP(this, &FNarrativeAssetEditor::EditAssetDefaults_Clicked),
								FCanExecuteAction());

	// Engine's Play commands
	ToolkitCommands->Append(FPlayWorldCommands::GlobalPlayWorldActions.ToSharedRef());

	// Debugging
	ToolkitCommands->MapAction(ToolbarCommands.GoToParentInstance,
								FExecuteAction::CreateSP(this, &FNarrativeAssetEditor::GoToParentInstance),
								FCanExecuteAction::CreateSP(this, &FNarrativeAssetEditor::CanGoToParentInstance),
								FIsActionChecked(),
								FIsActionButtonVisible::CreateSP(this, &FNarrativeAssetEditor::CanGoToParentInstance));
}

void FNarrativeAssetEditor::RefreshAsset()
{
	// attempt to refresh graph, fix common issues automatically
	CastChecked<UNarrativeGraph>(NarrativeAsset->GetGraph())->RefreshGraph();
}

void FNarrativeAssetEditor::ValidateAsset_Internal()
{
	FNarrativeMessageLog LogResults;
	ValidateAsset(LogResults);

	// push messages to its window
	ValidationLogListing->ClearMessages();
	if (LogResults.Messages.Num() > 0)
	{
		TabManager->TryInvokeTab(ValidationLogTab);
		ValidationLogListing->AddMessages(LogResults.Messages);
	}
	ValidationLogListing->OnDataChanged().Broadcast();
}

void FNarrativeAssetEditor::ValidateAsset(FNarrativeMessageLog& MessageLog)
{
	NarrativeAsset->ValidateAsset(MessageLog);
}

#if ENABLE_SEARCH_IN_ASSET_EDITOR
void FNarrativeAssetEditor::SearchInAsset()
{
	TabManager->TryInvokeTab(SearchTab);
	SearchBrowser->FocusForUse();
}
#endif

void FNarrativeAssetEditor::EditAssetDefaults_Clicked() const
{
	DetailsView->SetObject(NarrativeAsset);
}

void FNarrativeAssetEditor::GoToParentInstance()
{
	const UNarrativeAsset* AssetThatInstancedThisAsset = NarrativeAsset->GetInspectedInstance()->GetParentInstance();

	GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(AssetThatInstancedThisAsset->GetTemplateAsset());
	AssetThatInstancedThisAsset->GetTemplateAsset()->SetInspectedInstance(AssetThatInstancedThisAsset->GetDisplayName());
}

bool FNarrativeAssetEditor::CanGoToParentInstance()
{
	return NarrativeAsset->GetInspectedInstance() && NarrativeAsset->GetInspectedInstance()->GetNodeOwningThisAssetInstance() != nullptr;
}

void FNarrativeAssetEditor::CreateWidgets()
{
	// Details View
	{
		FDetailsViewArgs Args;
		Args.bHideSelectionTip = true;
		Args.bShowPropertyMatrixButton = false;
		Args.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Hide;
		Args.NotifyHook = this;

		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		DetailsView = PropertyModule.CreateDetailView(Args);
		DetailsView->SetIsPropertyEditingEnabledDelegate(FIsPropertyEditingEnabled::CreateStatic(&FNarrativeAssetEditor::CanEdit));
		DetailsView->SetObject(NarrativeAsset);
	}

	// Graph
	CreateGraphWidget();
	GraphEditor->OnSelectionChangedEvent.BindRaw(this, &FNarrativeAssetEditor::OnSelectedNodesChanged);

	// Palette
	Palette = SNew(SNarrativePalette, SharedThis(this));

	// Search
#if ENABLE_SEARCH_IN_ASSET_EDITOR
	SearchBrowser = SNew(SSearchBrowser, GetNarrativeAsset());
#endif

	// Logs
	FMessageLogModule& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
	{
		RuntimeLogListing = FNarrativeMessageLogListing::GetLogListing(NarrativeAsset, ENarrativeLogType::Runtime);
		RuntimeLogListing->OnMessageTokenClicked().AddSP(this, &FNarrativeAssetEditor::OnLogTokenClicked);
		RuntimeLog = MessageLogModule.CreateLogListingWidget(RuntimeLogListing.ToSharedRef());
	}
	{
		ValidationLogListing = FNarrativeMessageLogListing::GetLogListing(NarrativeAsset, ENarrativeLogType::Validation);
		ValidationLogListing->OnMessageTokenClicked().AddSP(this, &FNarrativeAssetEditor::OnLogTokenClicked);
		ValidationLog = MessageLogModule.CreateLogListingWidget(ValidationLogListing.ToSharedRef());
	}
}

void FNarrativeAssetEditor::CreateGraphWidget()
{
	 SAssignNew(GraphEditor, SNarrativeGraphEditor, SharedThis(this))
		.DetailsView(DetailsView);
}

bool FNarrativeAssetEditor::CanEdit()
{
	return GEditor->PlayWorld == nullptr;
}

void FNarrativeAssetEditor::SetUISelectionState(const FName SelectionOwner)
{
	if (SelectionOwner != CurrentUISelection)
	{
		ClearSelectionStateFor(CurrentUISelection);
		CurrentUISelection = SelectionOwner;
	}
}

void FNarrativeAssetEditor::ClearSelectionStateFor(const FName SelectionOwner)
{
	if (SelectionOwner == GraphTab)
	{
		GraphEditor->ClearSelectionSet();
	}
	else if (SelectionOwner == PaletteTab)
	{
		if (Palette.IsValid())
		{
			Palette->ClearGraphActionMenuSelection();
		}
	}
}

FName FNarrativeAssetEditor::GetUISelectionState() const
{
	return CurrentUISelection;
}

#if ENABLE_JUMP_TO_INNER_OBJECT
void FNarrativeAssetEditor::JumpToInnerObject(UObject* InnerObject)
{
	if (const UNarrativeNode* NarrativeNode = Cast<UNarrativeNode>(InnerObject))
	{
		GraphEditor->JumpToNode(NarrativeNode->GetGraphNode(), true);
	}
	else if (const UEdGraphNode* GraphNode = Cast<UEdGraphNode>(InnerObject))
	{
		GraphEditor->JumpToNode(GraphNode, true);
	}
}
#endif

void FNarrativeAssetEditor::OnLogTokenClicked(const TSharedRef<IMessageToken>& Token) const
{
	if (Token->GetType() == EMessageToken::Object)
	{
		const TSharedRef<FUObjectToken> ObjectToken = StaticCastSharedRef<FUObjectToken>(Token);
		if (const UObject* Object = ObjectToken->GetObject().Get())
		{
			if (Object->IsAsset())
			{
				GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(const_cast<UObject*>(Object));
			}
			else
			{
				UE_LOG(LogNarrativeEditor, Warning, TEXT("Unknown type of hyperlinked object (%s), cannot focus it"), *GetNameSafe(Object));
			}
		}
	}
	else if (Token->GetType() == EMessageToken::EdGraph && GraphEditor.IsValid())
	{
		const TSharedRef<FNarrativeGraphToken> EdGraphToken = StaticCastSharedRef<FNarrativeGraphToken>(Token);

		if (const UEdGraphPin* GraphPin = EdGraphToken->GetPin())
		{
			if (!GraphPin->IsPendingKill())
			{
				GraphEditor->JumpToPin(GraphPin);
			}
		}
		else if (const UEdGraphNode* GraphNode = EdGraphToken->GetGraphNode())
		{
			GraphEditor->JumpToNode(GraphNode, true);
		}
	}
}

#undef LOCTEXT_NAMESPACE
