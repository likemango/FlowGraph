// Copyright XiaoYao

#pragma once

#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"

class FNarrativeAssetEditor;
class SNarrativeGraphEditor;
class UEdGraph;

class NARRATIVEEDITOR_API FNarrativeGraphUtils
{
public:
	FNarrativeGraphUtils() {}

	static TSharedPtr<FNarrativeAssetEditor> GetNarrativeAssetEditor(const UEdGraph* Graph);
	static TSharedPtr<SNarrativeGraphEditor> GetNarrativeGraphEditor(const UEdGraph* Graph);
};
