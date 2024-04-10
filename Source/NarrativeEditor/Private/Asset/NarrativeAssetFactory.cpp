// Copyright XiaoYao

#include "Asset/NarrativeAssetFactory.h"
#include "NarrativeAsset.h"
#include "Graph/NarrativeGraph.h"
#include "Graph/NarrativeGraphSettings.h"

#include "ClassViewerFilter.h"
#include "ClassViewerModule.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/SClassPickerDialog.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "NarrativeAssetFactory"

class FAssetClassParentFilter final : public IClassViewerFilter
{
public:
	FAssetClassParentFilter()
		: DisallowedClassFlags(CLASS_None)
		, bDisallowBlueprintBase(false)
	{
	}

	/** All children of these classes will be included unless filtered out by another setting. */
	TSet<const UClass*> AllowedChildrenOfClasses;

	/** Disallowed class flags. */
	EClassFlags DisallowedClassFlags;

	/** Disallow blueprint base classes. */
	bool bDisallowBlueprintBase;

	virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, const TSharedRef<FClassViewerFilterFuncs> InFilterFuncs) override
	{
		const bool bAllowed = !InClass->HasAnyClassFlags(DisallowedClassFlags) && InFilterFuncs->IfInChildOfClassesSet(AllowedChildrenOfClasses, InClass) != EFilterReturn::Failed;

		if (bAllowed && bDisallowBlueprintBase)
		{
			if (FKismetEditorUtilities::CanCreateBlueprintOfClass(InClass))
			{
				return false;
			}
		}

		return bAllowed;
	}

	virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef<const IUnloadedBlueprintData> InUnloadedClassData, TSharedRef<FClassViewerFilterFuncs> InFilterFuncs) override
	{
		if (bDisallowBlueprintBase)
		{
			return false;
		}

		return !InUnloadedClassData->HasAnyClassFlags(DisallowedClassFlags) && InFilterFuncs->IfInChildOfClassesSet(AllowedChildrenOfClasses, InUnloadedClassData) != EFilterReturn::Failed;
	}
};

UNarrativeAssetFactory::UNarrativeAssetFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UNarrativeAsset::StaticClass();

	bCreateNew = true;
	bEditorImport = false;
	bEditAfterNew = true;
}

bool UNarrativeAssetFactory::ConfigureProperties()
{
	const FText TitleText = LOCTEXT("CreateNarrativeAssetOptions", "Pick Narrative Asset Class");

	return ConfigurePropertiesInternal(TitleText);
}

bool UNarrativeAssetFactory::ConfigurePropertiesInternal(const FText& TitleText)
{
	AssetClass = UNarrativeGraphSettings::Get()->DefaultNarrativeAssetClass;
	if (AssetClass) // Class was set in settings
	{
		return true;
	}

	// Load the Class Viewer module to display a class picker
	FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer");

	// Fill in options
	FClassViewerInitializationOptions Options;
	Options.Mode = EClassViewerMode::ClassPicker;

	const TSharedPtr<FAssetClassParentFilter> Filter = MakeShareable(new FAssetClassParentFilter);
	Filter->DisallowedClassFlags = CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists | CLASS_HideDropDown;
	Filter->AllowedChildrenOfClasses.Add(SupportedClass);

	Options.ClassFilters = {Filter.ToSharedRef()};

	UClass* ChosenClass = nullptr;
	const bool bPressedOk = SClassPickerDialog::PickClass(TitleText, Options, ChosenClass, SupportedClass);

	if (bPressedOk)
	{
		AssetClass = ChosenClass;
	}

	return bPressedOk;
}

UObject* UNarrativeAssetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UNarrativeAsset* NewNarrativeAsset;
	if (AssetClass)
	{
		NewNarrativeAsset = NewObject<UNarrativeAsset>(InParent, AssetClass, Name, Flags | RF_Transactional, Context);
	}
	else
	{
		// if we have no asset class, use the passed-in class instead
		NewNarrativeAsset = NewObject<UNarrativeAsset>(InParent, Class, Name, Flags | RF_Transactional, Context);
	}

	UNarrativeGraph::CreateGraph(NewNarrativeAsset);
	return NewNarrativeAsset;
}

#undef LOCTEXT_NAMESPACE
