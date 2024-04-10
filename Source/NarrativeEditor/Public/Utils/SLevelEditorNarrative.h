// Copyright XiaoYao

#pragma once

#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

struct FAssetData;

class NARRATIVEEDITOR_API SLevelEditorNarrative : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLevelEditorNarrative) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

protected:
	void OnMapOpened(const FString& Filename, bool bAsTemplate);
	void CreateNarrativeWidget();

	FString GetNarrativeAssetPath() const;
	void OnNarrativeChanged(const FAssetData& NewAsset);

	static class UNarrativeComponent* FindNarrativeComponent();
	
	FString NarrativeAssetPath;
};
