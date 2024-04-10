// Copyright XiaoYao

#pragma once

#include "EdGraph/EdGraphSchema.h"
#include "Templates/SubclassOf.h"
#include "NarrativeGraphSchema.generated.h"

class UNarrativeAsset;
class UNarrativeNode;
class UNarrativeGraphNode;

DECLARE_MULTICAST_DELEGATE(FNarrativeGraphSchemaRefresh);

UCLASS()
class NARRATIVEEDITOR_API UNarrativeGraphSchema : public UEdGraphSchema
{
	GENERATED_UCLASS_BODY()

	friend class UNarrativeGraph;

private:
	static bool bInitialGatherPerformed;
	static TArray<UClass*> NativeNarrativeNodes;
	static TMap<FName, FAssetData> BlueprintNarrativeNodes;
	static TMap<UClass*, UClass*> GraphNodesByNarrativeNodes;

	static bool bBlueprintCompilationPending;

public:
	static void SubscribeToAssetChanges();
	static void GetPaletteActions(FGraphActionMenuBuilder& ActionMenuBuilder, const UNarrativeAsset* EditedNarrativeAsset, const FString& CategoryName);

	// EdGraphSchema
	virtual void GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const override;
	virtual void CreateDefaultNodesForGraph(UEdGraph& Graph) const override;
	virtual const FPinConnectionResponse CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const override;
	virtual bool TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const override;
	virtual bool ShouldHidePinDefaultValue(UEdGraphPin* Pin) const override;
	virtual FLinearColor GetPinTypeColor(const FEdGraphPinType& PinType) const override;
	virtual FText GetPinDisplayName(const UEdGraphPin* Pin) const override;
	virtual void BreakNodeLinks(UEdGraphNode& TargetNode) const override;
	virtual void BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotification) const override;
	virtual int32 GetNodeSelectionCount(const UEdGraph* Graph) const override;
	virtual TSharedPtr<FEdGraphSchemaAction> GetCreateCommentAction() const override;
	virtual void OnPinConnectionDoubleCicked(UEdGraphPin* PinA, UEdGraphPin* PinB, const FVector2D& GraphPosition) const override;
	// --

	static TArray<TSharedPtr<FString>> GetNarrativeNodeCategories();
	static UClass* GetAssignedGraphNodeClass(const UClass* NarrativeNodeClass);

protected:
	static UNarrativeGraphNode* CreateDefaultNode(UEdGraph& Graph, const UNarrativeAsset* AssetClassDefaults, const TSubclassOf<UNarrativeNode>& NodeClass, const FVector2D& Offset, bool bPlacedAsGhostNode);

private:
	static void ApplyNodeFilter(const UNarrativeAsset* AssetClassDefaults, const UClass* NarrativeNodeClass, TArray<UNarrativeNode*>& FilteredNodes);
	static void GetNarrativeNodeActions(FGraphActionMenuBuilder& ActionMenuBuilder, const UNarrativeAsset* EditedNarrativeAsset, const FString& CategoryName);
	static void GetCommentAction(FGraphActionMenuBuilder& ActionMenuBuilder, const UEdGraph* CurrentGraph = nullptr);

	static bool IsNarrativeNodePlaceable(const UClass* Class);

	static void OnBlueprintPreCompile(UBlueprint* Blueprint);
	static void OnBlueprintCompiled();
	static void OnHotReload(EReloadCompleteReason ReloadCompleteReason);

	static void GatherNativeNodes();
	static void GatherNodes();

	static void OnAssetAdded(const FAssetData& AssetData);
	static void AddAsset(const FAssetData& AssetData, const bool bBatch);
	static void OnAssetRemoved(const FAssetData& AssetData);

public:
	static FNarrativeGraphSchemaRefresh OnNodeListChanged;
	static UBlueprint* GetPlaceableNodeBlueprint(const FAssetData& AssetData);

	static const UNarrativeAsset* GetAssetClassDefaults(const UEdGraph* Graph);
};
