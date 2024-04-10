// Copyright XiaoYao

#include "Asset/AssetTypeActions_NarrativeAsset.h"
#include "Asset/SNarrativeDiff.h"
#include "NarrativeEditorModule.h"
#include "Graph/NarrativeGraphSettings.h"

#include "NarrativeAsset.h"

#include "Toolkits/IToolkit.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions_NarrativeAsset"

FText FAssetTypeActions_NarrativeAsset::GetName() const
{
	return LOCTEXT("AssetTypeActions_NarrativeAsset", "Narrative Asset");
}

uint32 FAssetTypeActions_NarrativeAsset::GetCategories()
{
	return UNarrativeGraphSettings::Get()->bExposeNarrativeAssetCreation ? FNarrativeEditorModule::NarrativeAssetCategory : 0;
}

UClass* FAssetTypeActions_NarrativeAsset::GetSupportedClass() const
{
	return UNarrativeAsset::StaticClass();
}

void FAssetTypeActions_NarrativeAsset::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	const EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		if (UNarrativeAsset* NarrativeAsset = Cast<UNarrativeAsset>(*ObjIt))
		{
			const FNarrativeEditorModule* NarrativeModule = &FModuleManager::LoadModuleChecked<FNarrativeEditorModule>("NarrativeEditor");
			NarrativeModule->CreateNarrativeAssetEditor(Mode, EditWithinLevelEditor, NarrativeAsset);
		}
	}
}

void FAssetTypeActions_NarrativeAsset::PerformAssetDiff(UObject* OldAsset, UObject* NewAsset, const FRevisionInfo& OldRevision, const FRevisionInfo& NewRevision) const
{
	const UNarrativeAsset* OldNarrative = CastChecked<UNarrativeAsset>(OldAsset);
	const UNarrativeAsset* NewNarrative = CastChecked<UNarrativeAsset>(NewAsset);

	// sometimes we're comparing different revisions of one single asset (other 
	// times we're comparing two completely separate assets altogether)
	const bool bIsSingleAsset = (OldNarrative->GetName() == NewNarrative->GetName());

	static const FText BasicWindowTitle = LOCTEXT("NarrativeAssetDiff", "NarrativeAsset Diff");
	const FText WindowTitle = !bIsSingleAsset ? BasicWindowTitle : FText::Format(LOCTEXT("NarrativeAsset Diff", "{0} - NarrativeAsset Diff"), FText::FromString(NewNarrative->GetName()));

	SNarrativeDiff::CreateDiffWindow(WindowTitle, OldNarrative, NewNarrative, OldRevision, NewRevision);
}

#undef LOCTEXT_NAMESPACE
