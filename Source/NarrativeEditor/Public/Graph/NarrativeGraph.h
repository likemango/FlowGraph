// Copyright XiaoYao

#pragma once

#include "EdGraph/EdGraph.h"

#include "NarrativeAsset.h"
#include "NarrativeGraph.generated.h"

class NARRATIVEEDITOR_API FNarrativeGraphInterface : public INarrativeGraphInterface
{
public:
	virtual ~FNarrativeGraphInterface() override {}

	virtual void OnInputTriggered(UEdGraphNode* GraphNode, const int32 Index) const override;
	virtual void OnOutputTriggered(UEdGraphNode* GraphNode, const int32 Index) const override;
};

UCLASS()
class NARRATIVEEDITOR_API UNarrativeGraph : public UEdGraph
{
	GENERATED_UCLASS_BODY()

	static UEdGraph* CreateGraph(UNarrativeAsset* InNarrativeAsset);
	static UEdGraph* CreateGraph(UNarrativeAsset* InNarrativeAsset, TSubclassOf<UNarrativeGraphSchema> NarrativeSchema);
	void RefreshGraph();

	// UEdGraph
	virtual void NotifyGraphChanged() override;
	// --

	/** Returns the NarrativeAsset that contains this graph */
	UNarrativeAsset* GetNarrativeAsset() const;
};
