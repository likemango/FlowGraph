// Copyright XiaoYao

#include "Graph/NarrativeGraphUtils.h"
#include "Asset/NarrativeAssetEditor.h"
#include "Graph/NarrativeGraph.h"

#include "NarrativeAsset.h"

#include "Toolkits/ToolkitManager.h"

TSharedPtr<FNarrativeAssetEditor> FNarrativeGraphUtils::GetNarrativeAssetEditor(const UEdGraph* Graph)
{
	check(Graph);

	TSharedPtr<FNarrativeAssetEditor> NarrativeAssetEditor;
	if (const UNarrativeAsset* NarrativeAsset = Cast<const UNarrativeGraph>(Graph)->GetNarrativeAsset())
	{
		const TSharedPtr<IToolkit> FoundAssetEditor = FToolkitManager::Get().FindEditorForAsset(NarrativeAsset);
		if (FoundAssetEditor.IsValid())
		{
			NarrativeAssetEditor = StaticCastSharedPtr<FNarrativeAssetEditor>(FoundAssetEditor);
		}
	}
	return NarrativeAssetEditor;
}

TSharedPtr<SNarrativeGraphEditor> FNarrativeGraphUtils::GetNarrativeGraphEditor(const UEdGraph* Graph)
{
	TSharedPtr<SNarrativeGraphEditor> NarrativeGraphEditor;
	
	const TSharedPtr<FNarrativeAssetEditor> NarrativeEditor = GetNarrativeAssetEditor(Graph);
	if (NarrativeEditor.IsValid())
	{
		NarrativeGraphEditor = NarrativeEditor->GetNarrativeGraph();
	}

	return NarrativeGraphEditor;
}
