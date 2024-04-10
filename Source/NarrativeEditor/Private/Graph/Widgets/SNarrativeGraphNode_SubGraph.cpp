// Copyright XiaoYao

#include "Graph/Widgets/SNarrativeGraphNode_SubGraph.h"
#include "Graph/NarrativeGraphEditorSettings.h"

#include "NarrativeAsset.h"
#include "Nodes/Route/NarrativeNode_SubGraph.h"

#include "SGraphPreviewer.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SToolTip.h"

#define LOCTEXT_NAMESPACE "SNarrativeGraphNode_SubGraph"

TSharedPtr<SToolTip> SNarrativeGraphNode_SubGraph::GetComplexTooltip()
{
	if (UNarrativeGraphEditorSettings::Get()->bShowSubGraphPreview && NarrativeGraphNode)
	{
		if (UNarrativeNode* NarrativeNode = NarrativeGraphNode->GetNarrativeNode())
		{
			const UNarrativeAsset* AssetToEdit = Cast<UNarrativeAsset>(NarrativeNode->GetAssetToEdit());
			if (AssetToEdit && AssetToEdit->GetGraph())
			{
				TSharedPtr<SWidget> TitleBarWidget = SNullWidget::NullWidget;
				if (UNarrativeGraphEditorSettings::Get()->bShowSubGraphPath)
				{
					FString CleanAssetName = AssetToEdit->GetPathName(nullptr);
					const int32 SubStringIdx = CleanAssetName.Find(".", ESearchCase::IgnoreCase, ESearchDir::FromEnd);
					CleanAssetName.LeftInline(SubStringIdx);
					
					TitleBarWidget = SNew(SBox)
					.Padding(10.f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(CleanAssetName))
					];
				}
				
				return SNew(SToolTip)
				[
					SNew(SBox)
					.WidthOverride(UNarrativeGraphEditorSettings::Get()->SubGraphPreviewSize.X)
					.HeightOverride(UNarrativeGraphEditorSettings::Get()->SubGraphPreviewSize.Y)
					[
						SNew(SOverlay)
						+SOverlay::Slot()
						[
							SNew(SGraphPreviewer, AssetToEdit->GetGraph())
							.CornerOverlayText(LOCTEXT("NarrativeNodePreviewGraphOverlayText", "GRAPH PREVIEW"))
							.ShowGraphStateOverlay(false)
							.TitleBar(TitleBarWidget)
						]
					]
				];
			}
		}
	}

	return SNarrativeGraphNode::GetComplexTooltip();
}

#undef LOCTEXT_NAMESPACE
