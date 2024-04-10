// Copyright XiaoYao

#include "Graph/NarrativeGraphEditorSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativeGraphEditorSettings)

UNarrativeGraphEditorSettings::UNarrativeGraphEditorSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, NodeDoubleClickTarget(ENarrativeNodeDoubleClickTarget::PrimaryAssetOrNodeDefinition)
	, bShowNodeClass(false)
	, bShowNodeDescriptionWhilePlaying(true)
	, bEnforceFriendlyPinNames(false)
	, bShowSubGraphPreview(true)
	, bShowSubGraphPath(true)
	, SubGraphPreviewSize(FVector2D(640.f, 360.f))
	, bHotReloadNativeNodes(false)
	, bHighlightInputWiresOfSelectedNodes(false)
	, bHighlightOutputWiresOfSelectedNodes(false)
{
}
