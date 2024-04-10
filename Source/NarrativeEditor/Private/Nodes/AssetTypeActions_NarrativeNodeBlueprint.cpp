// Copyright XiaoYao

#include "Nodes/AssetTypeActions_NarrativeNodeBlueprint.h"
#include "Nodes/NarrativeNodeBlueprintFactory.h"
#include "NarrativeEditorModule.h"
#include "Graph/NarrativeGraphSettings.h"

#include "Nodes/NarrativeNodeBlueprint.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions_NarrativeNodeBlueprint"

FText FAssetTypeActions_NarrativeNodeBlueprint::GetName() const
{
	return LOCTEXT("AssetTypeActions_NarrativeNodeBlueprint", "Narrative Node Blueprint");
}

uint32 FAssetTypeActions_NarrativeNodeBlueprint::GetCategories()
{
	return UNarrativeGraphSettings::Get()->bExposeNarrativeNodeCreation ? FNarrativeEditorModule::NarrativeAssetCategory : 0;
}

UClass* FAssetTypeActions_NarrativeNodeBlueprint::GetSupportedClass() const
{
	return UNarrativeNodeBlueprint::StaticClass();
}

UFactory* FAssetTypeActions_NarrativeNodeBlueprint::GetFactoryForBlueprintType(UBlueprint* InBlueprint) const
{
	UNarrativeNodeBlueprintFactory* NarrativeNodeBlueprintFactory = NewObject<UNarrativeNodeBlueprintFactory>();
	NarrativeNodeBlueprintFactory->ParentClass = TSubclassOf<UNarrativeNode>(*InBlueprint->GeneratedClass);
	return NarrativeNodeBlueprintFactory;
}

#undef LOCTEXT_NAMESPACE
