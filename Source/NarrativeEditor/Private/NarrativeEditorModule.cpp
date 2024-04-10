// Copyright XiaoYao

#include "NarrativeEditorModule.h"
#include "NarrativeEditorStyle.h"

#include "Asset/AssetTypeActions_NarrativeAsset.h"
#include "Asset/NarrativeAssetEditor.h"
#include "Asset/NarrativeAssetIndexer.h"
#include "Graph/NarrativeGraphConnectionDrawingPolicy.h"
#include "Graph/NarrativeGraphSettings.h"
#include "Utils/SLevelEditorNarrative.h"
#include "MovieScene/NarrativeTrackEditor.h"
#include "Nodes/AssetTypeActions_NarrativeNodeBlueprint.h"
#include "Pins/SNarrativeInputPinHandle.h"
#include "Pins/SNarrativeOutputPinHandle.h"

#include "DetailCustomizations/NarrativeAssetDetails.h"
#include "DetailCustomizations/NarrativeNode_Details.h"
#include "DetailCustomizations/NarrativeNode_ComponentObserverDetails.h"
#include "DetailCustomizations/NarrativeNode_CustomInputDetails.h"
#include "DetailCustomizations/NarrativeNode_CustomOutputDetails.h"
#include "DetailCustomizations/NarrativeNode_PlayLevelSequenceDetails.h"
#include "DetailCustomizations/NarrativeOwnerFunctionRefCustomization.h"
#include "DetailCustomizations/NarrativeNode_SubGraphDetails.h"

#include "NarrativeAsset.h"
#include "Nodes/Route/NarrativeNode_CustomInput.h"
#include "Nodes/Route/NarrativeNode_CustomOutput.h"
#include "Nodes/Route/NarrativeNode_SubGraph.h"
#include "Nodes/World/NarrativeNode_ComponentObserver.h"
#include "Nodes/World/NarrativeNode_PlayLevelSequence.h"

#include "AssetToolsModule.h"
#include "EdGraphUtilities.h"
#include "IAssetSearchModule.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "ISequencerChannelInterface.h" // ignore Rider's false "unused include" warning
#include "ISequencerModule.h"
#include "LevelEditor.h"
#include "Modules/ModuleManager.h"

static FName AssetSearchModuleName = TEXT("AssetSearch");

#define LOCTEXT_NAMESPACE "NarrativeEditorModule"

EAssetTypeCategories::Type FNarrativeEditorModule::NarrativeAssetCategory = static_cast<EAssetTypeCategories::Type>(0);

void FNarrativeEditorModule::StartupModule()
{
	FNarrativeEditorStyle::Initialize();

	RegisterAssets();

	// register visual utilities
	FEdGraphUtilities::RegisterVisualPinConnectionFactory(MakeShareable(new FNarrativeGraphConnectionDrawingPolicyFactory));
	FEdGraphUtilities::RegisterVisualPinFactory(MakeShareable(new FNarrativeInputPinHandleFactory()));
	FEdGraphUtilities::RegisterVisualPinFactory(MakeShareable(new FNarrativeOutputPinHandleFactory()));

	// add Narrative Toolbar
	if (UNarrativeGraphSettings::Get()->bShowAssetToolbarAboveLevelEditor)
	{
		if (FLevelEditorModule* LevelEditorModule = FModuleManager::GetModulePtr<FLevelEditorModule>(TEXT("LevelEditor")))
		{
			const TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
			MenuExtender->AddToolBarExtension("Play", EExtensionHook::After, nullptr, FToolBarExtensionDelegate::CreateRaw(this, &FNarrativeEditorModule::CreateNarrativeToolbar));
			LevelEditorModule->GetToolBarExtensibilityManager()->AddExtender(MenuExtender);
		}
	}

	// register Narrative sequence track
	ISequencerModule& SequencerModule = FModuleManager::Get().LoadModuleChecked<ISequencerModule>("Sequencer");
	NarrativeTrackCreateEditorHandle = SequencerModule.RegisterTrackEditor(FOnCreateTrackEditor::CreateStatic(&FNarrativeTrackEditor::CreateTrackEditor));

	RegisterDetailCustomizations();

	// register asset indexers
	if (FModuleManager::Get().IsModuleLoaded(AssetSearchModuleName))
	{
		RegisterAssetIndexers();
	}
	ModulesChangedHandle = FModuleManager::Get().OnModulesChanged().AddRaw(this, &FNarrativeEditorModule::ModulesChangesCallback);
}

void FNarrativeEditorModule::ShutdownModule()
{
	FNarrativeEditorStyle::Shutdown();

	UnregisterDetailCustomizations();

	UnregisterAssets();

	// unregister track editors
	ISequencerModule& SequencerModule = FModuleManager::Get().LoadModuleChecked<ISequencerModule>("Sequencer");
	SequencerModule.UnRegisterTrackEditor(NarrativeTrackCreateEditorHandle);

	FModuleManager::Get().OnModulesChanged().Remove(ModulesChangedHandle);
}

void FNarrativeEditorModule::RegisterAssets()
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	// try to merge asset category with a built-in one
	{
		const FText AssetCategoryText = UNarrativeGraphSettings::Get()->NarrativeAssetCategoryName;

		// Find matching built-in category
		if (!AssetCategoryText.IsEmpty())
		{
			TArray<FAdvancedAssetCategory> AllCategories;
			AssetTools.GetAllAdvancedAssetCategories(AllCategories);
			for (const FAdvancedAssetCategory& ExistingCategory : AllCategories)
			{
				if (ExistingCategory.CategoryName.EqualTo(AssetCategoryText))
				{
					NarrativeAssetCategory = ExistingCategory.CategoryType;
					break;
				}
			}
		}

		if (NarrativeAssetCategory == EAssetTypeCategories::None)
		{
			NarrativeAssetCategory = AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("Narrative")), AssetCategoryText);
		}
	}

	const TSharedRef<IAssetTypeActions> NarrativeAssetActions = MakeShareable(new FAssetTypeActions_NarrativeAsset());
	RegisteredAssetActions.Add(NarrativeAssetActions);
	AssetTools.RegisterAssetTypeActions(NarrativeAssetActions);

	const TSharedRef<IAssetTypeActions> NarrativeNodeActions = MakeShareable(new FAssetTypeActions_NarrativeNodeBlueprint());
	RegisteredAssetActions.Add(NarrativeNodeActions);
	AssetTools.RegisterAssetTypeActions(NarrativeNodeActions);
}

void FNarrativeEditorModule::UnregisterAssets()
{
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		for (const TSharedRef<IAssetTypeActions>& TypeAction : RegisteredAssetActions)
		{
			AssetTools.UnregisterAssetTypeActions(TypeAction);
		}
	}

	RegisteredAssetActions.Empty();
}

void FNarrativeEditorModule::RegisterCustomClassLayout(const TSubclassOf<UObject> Class, const FOnGetDetailCustomizationInstance DetailLayout)
{
	if (Class)
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.RegisterCustomClassLayout(Class->GetFName(), DetailLayout);

		CustomClassLayouts.Add(Class->GetFName());
	}
}

void FNarrativeEditorModule::RegisterCustomStructLayout(const UScriptStruct& Struct, const FOnGetPropertyTypeCustomizationInstance DetailLayout)
{
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.RegisterCustomPropertyTypeLayout(Struct.GetFName(), DetailLayout);

		CustomStructLayouts.Add(Struct.GetFName());
	}
}

void FNarrativeEditorModule::RegisterDetailCustomizations()
{
	// register detail customizations
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

		RegisterCustomClassLayout(UNarrativeAsset::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FNarrativeAssetDetails::MakeInstance));
		RegisterCustomClassLayout(UNarrativeNode::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FNarrativeNode_Details::MakeInstance));
		RegisterCustomClassLayout(UNarrativeNode_ComponentObserver::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FNarrativeNode_ComponentObserverDetails::MakeInstance));
		RegisterCustomClassLayout(UNarrativeNode_CustomInput::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FNarrativeNode_CustomInputDetails::MakeInstance));
		RegisterCustomClassLayout(UNarrativeNode_CustomOutput::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FNarrativeNode_CustomOutputDetails::MakeInstance));
		RegisterCustomClassLayout(UNarrativeNode_PlayLevelSequence::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FNarrativeNode_PlayLevelSequenceDetails::MakeInstance));
    RegisterCustomClassLayout(UNarrativeNode_SubGraph::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FNarrativeNode_SubGraphDetails::MakeInstance));
		RegisterCustomStructLayout(*FNarrativeOwnerFunctionRef::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FNarrativeOwnerFunctionRefCustomization::MakeInstance));

		PropertyModule.NotifyCustomizationModuleChanged();
	}
}

void FNarrativeEditorModule::UnregisterDetailCustomizations()
{
	// unregister details customizations
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

		for (auto It = CustomClassLayouts.CreateConstIterator(); It; ++It)
		{
			if (It->IsValid())
			{
				PropertyModule.UnregisterCustomClassLayout(*It);
			}
		}

		for (auto It = CustomStructLayouts.CreateConstIterator(); It; ++It)
		{
			if (It->IsValid())
			{
				PropertyModule.UnregisterCustomPropertyTypeLayout(*It);
			}
		}

		PropertyModule.NotifyCustomizationModuleChanged();
	}
}

void FNarrativeEditorModule::ModulesChangesCallback(const FName ModuleName, const EModuleChangeReason ReasonForChange) const
{
	if (ReasonForChange == EModuleChangeReason::ModuleLoaded && ModuleName == AssetSearchModuleName)
	{
		RegisterAssetIndexers();
	}
}

void FNarrativeEditorModule::RegisterAssetIndexers() const
{
	IAssetSearchModule::Get().RegisterAssetIndexer(UNarrativeAsset::StaticClass(), MakeUnique<FNarrativeAssetIndexer>());
}

void FNarrativeEditorModule::CreateNarrativeToolbar(FToolBarBuilder& ToolbarBuilder) const
{
	ToolbarBuilder.BeginSection("Narrative");
	{
		ToolbarBuilder.AddWidget(SNew(SLevelEditorNarrative));
	}
	ToolbarBuilder.EndSection();
}

TSharedRef<FNarrativeAssetEditor> FNarrativeEditorModule::CreateNarrativeAssetEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UNarrativeAsset* NarrativeAsset)
{
	TSharedRef<FNarrativeAssetEditor> NewNarrativeAssetEditor(new FNarrativeAssetEditor());
	NewNarrativeAssetEditor->InitNarrativeAssetEditor(Mode, InitToolkitHost, NarrativeAsset);
	return NewNarrativeAssetEditor;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FNarrativeEditorModule, NarrativeEditor)
