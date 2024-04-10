// Copyright XiaoYao

#include "NarrativeEditorStyle.h"

#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleRegistry.h"

#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define IMAGE_BRUSH_SVG( RelativePath, ... ) FSlateVectorImageBrush(StyleSet->RootToContentDir(RelativePath, TEXT(".svg")), __VA_ARGS__)

TSharedPtr<FSlateStyleSet> FNarrativeEditorStyle::StyleSet = nullptr;

FName FNarrativeEditorStyle::GetStyleSetName()
{
	static FName NarrativeEditorStyleName(TEXT("NarrativeEditorStyle"));
	return NarrativeEditorStyleName;
}

void FNarrativeEditorStyle::Initialize()
{
	StyleSet = MakeShareable(new FSlateStyleSet(TEXT("NarrativeEditorStyle")));

	const FVector2D Icon16(16.0f, 16.0f);
	const FVector2D Icon20(20.0f, 20.0f);
	const FVector2D Icon30(30.0f, 30.0f);
	const FVector2D Icon40(40.0f, 40.0f);
	const FVector2D Icon64(64.0f, 64.0f);

	// engine assets
	StyleSet->SetContentRoot(FPaths::EngineContentDir() / TEXT("Editor/Slate/"));

	StyleSet->Set("NarrativeToolbar.RefreshAsset", new IMAGE_BRUSH_SVG( "Starship/Common/Apply", Icon20));
	StyleSet->Set("NarrativeToolbar.ValidateAsset", new IMAGE_BRUSH_SVG( "Starship/Common/Debug", Icon20));

	StyleSet->Set("NarrativeToolbar.SearchInAsset", new IMAGE_BRUSH_SVG( "Starship/Common/Search", Icon20));
	StyleSet->Set("NarrativeToolbar.EditAssetDefaults", new IMAGE_BRUSH_SVG("Starship/Common/Details", Icon20));

	StyleSet->Set("NarrativeToolbar.GoToParentInstance", new IMAGE_BRUSH("Icons/icon_DebugStepOut_40x", Icon40));

	StyleSet->Set("NarrativeGraph.BreakpointEnabled", new IMAGE_BRUSH("Old/Kismet2/Breakpoint_Valid", FVector2D(24.0f, 24.0f)));
	StyleSet->Set("NarrativeGraph.BreakpointDisabled", new IMAGE_BRUSH("Old/Kismet2/Breakpoint_Disabled", FVector2D(24.0f, 24.0f)));
	StyleSet->Set("NarrativeGraph.BreakpointHit", new IMAGE_BRUSH("Old/Kismet2/IP_Breakpoint", Icon40));
	StyleSet->Set("NarrativeGraph.PinBreakpointHit", new IMAGE_BRUSH("Old/Kismet2/IP_Breakpoint", Icon30));

	StyleSet->Set("GraphEditor.Sequence_16x", new IMAGE_BRUSH("Icons/icon_Blueprint_Sequence_16x", Icon16));

	// Narrative assets
	StyleSet->SetContentRoot(IPluginManager::Get().FindPlugin(TEXT("Narrative"))->GetBaseDir() / TEXT("Resources"));

	StyleSet->Set("ClassIcon.NarrativeAsset", new IMAGE_BRUSH(TEXT("Icons/NarrativeAsset_16x"), Icon16));
	StyleSet->Set("ClassThumbnail.NarrativeAsset", new IMAGE_BRUSH(TEXT("Icons/NarrativeAsset_64x"), Icon64));

	StyleSet->Set("Narrative.Node.Title", new BOX_BRUSH("Icons/NarrativeNode_Title", FMargin(8.0f/64.0f, 0, 0, 0)));
	StyleSet->Set("Narrative.Node.Body", new BOX_BRUSH("Icons/NarrativeNode_Body", FMargin(16.f/64.f)));
	StyleSet->Set("Narrative.Node.ActiveShadow", new BOX_BRUSH("Icons/NarrativeNode_Shadow_Active", FMargin(18.0f/64.0f)));
	StyleSet->Set("Narrative.Node.WasActiveShadow", new BOX_BRUSH("Icons/NarrativeNode_Shadow_WasActive", FMargin(18.0f/64.0f)));

	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());
};

void FNarrativeEditorStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet.Get());
	ensure(StyleSet.IsUnique());
	StyleSet.Reset();
}

#undef BORDER_BRUSH
#undef BOX_BRUSH
#undef IMAGE_BRUSH
#undef IMAGE_BRUSH_SVG
