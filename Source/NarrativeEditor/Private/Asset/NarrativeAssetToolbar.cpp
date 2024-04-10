// Copyright XiaoYao

#include "Asset/NarrativeAssetToolbar.h"

#include "Asset/NarrativeAssetEditor.h"
#include "Asset/NarrativeAssetEditorContext.h"
#include "Asset/SAssetRevisionMenu.h"
#include "NarrativeEditorCommands.h"

#include "NarrativeAsset.h"

#include "Kismet2/DebuggerCommands.h"
#include "Misc/Attribute.h"
#include "Misc/MessageDialog.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "ToolMenu.h"
#include "ToolMenuSection.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#include "AssetToolsModule.h"
#include "IAssetTypeActions.h"
#include "ISourceControlModule.h"
#include "ISourceControlProvider.h"
#include "SourceControlHelpers.h"

#define LOCTEXT_NAMESPACE "NarrativeDebuggerToolbar"

//////////////////////////////////////////////////////////////////////////
// Narrative Asset Instance List

FText SNarrativeAssetInstanceList::NoInstanceSelectedText = LOCTEXT("NoInstanceSelected", "No instance selected");

void SNarrativeAssetInstanceList::Construct(const FArguments& InArgs, const TWeakObjectPtr<UNarrativeAsset> InTemplateAsset)
{
	TemplateAsset = InTemplateAsset;
	if (TemplateAsset.IsValid())
	{
		TemplateAsset->OnDebuggerRefresh().AddSP(this, &SNarrativeAssetInstanceList::RefreshInstances);
		RefreshInstances();
	}

	// create dropdown
	SAssignNew(Dropdown, SComboBox<TSharedPtr<FName>>)
		.OptionsSource(&InstanceNames)
		.Visibility_Static(&SNarrativeAssetInstanceList::GetDebuggerVisibility)
		.OnGenerateWidget(this, &SNarrativeAssetInstanceList::OnGenerateWidget)
		.OnSelectionChanged(this, &SNarrativeAssetInstanceList::OnSelectionChanged)
		[
			SNew(STextBlock)
			.Text(this, &SNarrativeAssetInstanceList::GetSelectedInstanceName)
		];

	ChildSlot
	[
		Dropdown.ToSharedRef()
	];
}

SNarrativeAssetInstanceList::~SNarrativeAssetInstanceList()
{
	if (TemplateAsset.IsValid())
	{
		TemplateAsset->OnDebuggerRefresh().RemoveAll(this);
	}
}

void SNarrativeAssetInstanceList::RefreshInstances()
{
	// collect instance names of this Narrative Asset
	InstanceNames = {MakeShareable(new FName(*NoInstanceSelectedText.ToString()))};
	TemplateAsset->GetInstanceDisplayNames(InstanceNames);

	// select instance
	if (const UNarrativeAsset* InspectedInstance = TemplateAsset->GetInspectedInstance())
	{
		const FName& InspectedInstanceName = InspectedInstance->GetDisplayName();
		for (const TSharedPtr<FName>& Instance : InstanceNames)
		{
			if (*Instance == InspectedInstanceName)
			{
				SelectedInstance = Instance;
				break;
			}
		}
	}
	else
	{
		// default object is always available
		SelectedInstance = InstanceNames[0];
	}
}

EVisibility SNarrativeAssetInstanceList::GetDebuggerVisibility()
{
	return GEditor->PlayWorld ? EVisibility::Visible : EVisibility::Collapsed;
}

TSharedRef<SWidget> SNarrativeAssetInstanceList::OnGenerateWidget(const TSharedPtr<FName> Item) const
{
	return SNew(STextBlock).Text(FText::FromName(*Item.Get()));
}

void SNarrativeAssetInstanceList::OnSelectionChanged(const TSharedPtr<FName> SelectedItem, const ESelectInfo::Type SelectionType)
{
	if (SelectionType != ESelectInfo::Direct)
	{
		SelectedInstance = SelectedItem;

		if (TemplateAsset.IsValid())
		{
			const FName NewSelectedInstanceName = (SelectedInstance.IsValid() && *SelectedInstance != *InstanceNames[0]) ? *SelectedInstance : NAME_None;
			TemplateAsset->SetInspectedInstance(NewSelectedInstanceName);
		}
	}
}

FText SNarrativeAssetInstanceList::GetSelectedInstanceName() const
{
	return SelectedInstance.IsValid() ? FText::FromName(*SelectedInstance) : NoInstanceSelectedText;
}

//////////////////////////////////////////////////////////////////////////
// Narrative Asset Breadcrumb

void SNarrativeAssetBreadcrumb::Construct(const FArguments& InArgs, const TWeakObjectPtr<UNarrativeAsset> InTemplateAsset)
{
	TemplateAsset = InTemplateAsset;

	// create breadcrumb
	SAssignNew(BreadcrumbTrail, SBreadcrumbTrail<FNarrativeBreadcrumb>)
		.OnCrumbClicked(this, &SNarrativeAssetBreadcrumb::OnCrumbClicked)
		.Visibility_Static(&SNarrativeAssetInstanceList::GetDebuggerVisibility)
		.ButtonStyle(FAppStyle::Get(), "FlatButton")
		.DelimiterImage(FAppStyle::GetBrush("Sequencer.BreadcrumbIcon"))
		.PersistentBreadcrumbs(true)
		.TextStyle(FAppStyle::Get(), "Sequencer.BreadcrumbText");

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		  .HAlign(HAlign_Right)
		  .VAlign(VAlign_Center)
		  .AutoHeight()
		  .Padding(25.0f, 10.0f)
		[
			BreadcrumbTrail.ToSharedRef()
		]
	];

	// fill breadcrumb
	BreadcrumbTrail->ClearCrumbs();
	if (UNarrativeAsset* InspectedInstance = TemplateAsset->GetInspectedInstance())
	{
		TArray<TWeakObjectPtr<UNarrativeAsset>> InstancesFromRoot = {InspectedInstance};

		const UNarrativeAsset* CheckedInstance = InspectedInstance;
		while (UNarrativeAsset* ParentInstance = CheckedInstance->GetParentInstance())
		{
			InstancesFromRoot.Insert(ParentInstance, 0);
			CheckedInstance = ParentInstance;
		}

		for (TWeakObjectPtr<UNarrativeAsset> Instance : InstancesFromRoot)
		{
			if (Instance.IsValid())
			{
				const FNarrativeBreadcrumb NewBreadcrumb = FNarrativeBreadcrumb(Instance);
				BreadcrumbTrail->PushCrumb(FText::FromName(NewBreadcrumb.InstanceName), FNarrativeBreadcrumb(Instance));
			}
		}
	}
}

void SNarrativeAssetBreadcrumb::OnCrumbClicked(const FNarrativeBreadcrumb& Item) const
{
	const UNarrativeAsset* InspectedInstance = TemplateAsset->GetInspectedInstance();
	if (InspectedInstance == nullptr || Item.InstanceName != InspectedInstance->GetDisplayName())
	{
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Item.AssetPathName);
	}
}

//////////////////////////////////////////////////////////////////////////
// Narrative Asset Toolbar

FNarrativeAssetToolbar::FNarrativeAssetToolbar(const TSharedPtr<FNarrativeAssetEditor> InAssetEditor, UToolMenu* ToolbarMenu)
	: NarrativeAssetEditor(InAssetEditor)
{
	BuildAssetToolbar(ToolbarMenu);
	BuildDebuggerToolbar(ToolbarMenu);
}

void FNarrativeAssetToolbar::BuildAssetToolbar(UToolMenu* ToolbarMenu) const
{
	{
		FToolMenuSection& Section = ToolbarMenu->AddSection("NarrativeAsset");
		Section.InsertPosition = FToolMenuInsert("Asset", EToolMenuInsertType::After);

		// add buttons
		Section.AddEntry(FToolMenuEntry::InitToolBarButton(FNarrativeToolbarCommands::Get().RefreshAsset));
		Section.AddEntry(FToolMenuEntry::InitToolBarButton(FNarrativeToolbarCommands::Get().ValidateAsset));
		Section.AddEntry(FToolMenuEntry::InitToolBarButton(FNarrativeToolbarCommands::Get().EditAssetDefaults));
	}
	
	{
		FToolMenuSection& Section = ToolbarMenu->AddSection("View");
		Section.InsertPosition = FToolMenuInsert("NarrativeAsset", EToolMenuInsertType::After);

		// Visual Diff: menu to choose asset revision compared with the current one 
		Section.AddDynamicEntry("SourceControlCommands", FNewToolMenuSectionDelegate::CreateLambda([this](FToolMenuSection& InSection)
		{
			InSection.InsertPosition = FToolMenuInsert();
			FToolMenuEntry DiffEntry = FToolMenuEntry::InitComboButton(
				"Diff",
				FUIAction(),
				FOnGetContent::CreateRaw(this, &FNarrativeAssetToolbar::MakeDiffMenu),
				LOCTEXT("Diff", "Diff"),
				LOCTEXT("NarrativeAssetEditorDiffToolTip", "Diff against previous revisions"),
				FSlateIcon(FAppStyle::Get().GetStyleSetName(), "BlueprintDiff.ToolbarIcon")
			);
			DiffEntry.StyleNameOverride = "CalloutToolbar";
			InSection.AddEntry(DiffEntry);
		}));

#if ENABLE_SEARCH_IN_ASSET_EDITOR
		Section.AddEntry(FToolMenuEntry::InitToolBarButton(FNarrativeToolbarCommands::Get().SearchInAsset));
#endif
	}
}

/** Delegate called to diff a specific revision with the current */
// Copy from FBlueprintEditorToolbar::OnDiffRevisionPicked
static void OnDiffRevisionPicked(FRevisionInfo const& RevisionInfo, const FString& Filename, TWeakObjectPtr<UObject> CurrentAsset)
{
	ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();

	// Get the SCC state
	const FSourceControlStatePtr SourceControlState = SourceControlProvider.GetState(Filename, EStateCacheUsage::Use);
	if (SourceControlState.IsValid())
	{
		for (int32 HistoryIndex = 0; HistoryIndex < SourceControlState->GetHistorySize(); HistoryIndex++)
		{
			TSharedPtr<ISourceControlRevision, ESPMode::ThreadSafe> Revision = SourceControlState->GetHistoryItem(HistoryIndex);
			check(Revision.IsValid());
			if (Revision->GetRevision() == RevisionInfo.Revision)
			{
				// Get the revision of this package from source control
				FString PreviousTempPkgName;
				if (Revision->Get(PreviousTempPkgName))
				{
					// Try and load that package
					UPackage* PreviousTempPkg = LoadPackage(nullptr, *PreviousTempPkgName, LOAD_ForDiff | LOAD_DisableCompileOnLoad);
					if (PreviousTempPkg)
					{
						const FString PreviousAssetName = FPaths::GetBaseFilename(Filename, true);
						UObject* PreviousAsset = FindObject<UObject>(PreviousTempPkg, *PreviousAssetName);
						if (PreviousAsset)
						{
							const FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
							const FRevisionInfo OldRevision = {Revision->GetRevision(), Revision->GetCheckInIdentifier(), Revision->GetDate()};
							const FRevisionInfo CurrentRevision = {TEXT(""), Revision->GetCheckInIdentifier(), Revision->GetDate()};
							AssetToolsModule.Get().DiffAssets(PreviousAsset, CurrentAsset.Get(), OldRevision, CurrentRevision);
						}
					}
					else
					{
						FMessageDialog::Open(EAppMsgType::Ok, NSLOCTEXT("SourceControl.HistoryWindow", "UnableToLoadAssets", "Unable to load assets to diff. Content may no longer be supported?"));
					}
				}
				break;
			}
		}
	}
}

// Variant of FBlueprintEditorToolbar::MakeDiffMenu
TSharedRef<SWidget> FNarrativeAssetToolbar::MakeDiffMenu() const
{
	if (ISourceControlModule::Get().IsEnabled() && ISourceControlModule::Get().GetProvider().IsAvailable())
	{
		UNarrativeAsset* NarrativeAsset = NarrativeAssetEditor.Pin()->GetNarrativeAsset();
		if (NarrativeAsset)
		{
			FString Filename = SourceControlHelpers::PackageFilename(NarrativeAsset->GetPathName());
			TWeakObjectPtr<UObject> AssetPtr = NarrativeAsset;

			// Add our async SCC task widget
			return SNew(SAssetRevisionMenu, Filename)
				.OnRevisionSelected_Static(&OnDiffRevisionPicked, AssetPtr);
		}
		else
		{
			// if asset is null then this means that multiple assets are selected
			FMenuBuilder MenuBuilder(true, nullptr);
			MenuBuilder.AddMenuEntry(LOCTEXT("NoRevisionsForMultipleNarrativeAssets", "Multiple Narrative Assets selected"), FText(), FSlateIcon(), FUIAction());
			return MenuBuilder.MakeWidget();
		}
	}

	FMenuBuilder MenuBuilder(true, nullptr);
	MenuBuilder.AddMenuEntry(LOCTEXT("SourceControlDisabled", "Source control is disabled"), FText(), FSlateIcon(), FUIAction());
	return MenuBuilder.MakeWidget();
}

void FNarrativeAssetToolbar::BuildDebuggerToolbar(UToolMenu* ToolbarMenu) const
{
	FToolMenuSection& Section = ToolbarMenu->AddSection("Debug");
	Section.InsertPosition = FToolMenuInsert("View", EToolMenuInsertType::After);

	Section.AddDynamicEntry("DebuggingCommands", FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
	{
		const UNarrativeAssetEditorContext* Context = InSection.FindContext<UNarrativeAssetEditorContext>();
		if (Context && Context->GetNarrativeAsset())
		{
			FPlayWorldCommands::BuildToolbar(InSection);

			InSection.AddEntry(FToolMenuEntry::InitWidget("AssetInstances", SNew(SNarrativeAssetInstanceList, Context->GetNarrativeAsset()), FText(), true));

			InSection.AddEntry(FToolMenuEntry::InitToolBarButton(FNarrativeToolbarCommands::Get().GoToParentInstance));
			InSection.AddEntry(FToolMenuEntry::InitWidget("AssetBreadcrumb", SNew(SNarrativeAssetBreadcrumb, Context->GetNarrativeAsset()), FText(), true));
		}
	}));
}

#undef LOCTEXT_NAMESPACE
