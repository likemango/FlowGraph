// Copyright XiaoYao

#pragma once

#include "Nodes/NarrativeNode.h"
#include "NarrativeNode_SubGraph.generated.h"

/**
 * Creates instance of provided Narrative Asset and starts its execution
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Sub Graph"))
class NARRATIVE_API UNarrativeNode_SubGraph : public UNarrativeNode
{
	GENERATED_UCLASS_BODY()

	friend class UNarrativeAsset;
	friend class FNarrativeNode_SubGraphDetails;
	friend class UNarrativeSubsystem;

	static FNarrativePin StartPin;
	static FNarrativePin FinishPin;
	
private:
	UPROPERTY(EditAnywhere, Category = "Graph")
	TSoftObjectPtr<UNarrativeAsset> Asset;

	/*
	 * Allow to create instance of the same Narrative Asset as the asset containing this node
	 * Enabling it may cause an infinite loop, if graph would keep creating copies of itself
	 */
	UPROPERTY(EditAnywhere, Category = "Graph")
	bool bCanInstanceIdenticalAsset;
	
	UPROPERTY(SaveGame)
	FString SavedAssetInstanceName;

protected:
	virtual bool CanBeAssetInstanced() const;
	
	virtual void PreloadContent() override;
	virtual void FlushContent() override;

	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;

public:
	virtual void ForceFinishNode() override;

protected:
	virtual void OnLoad_Implementation() override;


#if WITH_EDITORONLY_DATA
protected:
	// All the classes allowed to be used as assets on this subgraph node
	UPROPERTY()
	TArray<TSubclassOf<UNarrativeAsset>> AllowedAssignedAssetClasses;

	// All the classes disallowed to be used as assets on this subgraph node
	UPROPERTY()
	TArray<TSubclassOf<UNarrativeAsset>> DeniedAssignedAssetClasses;
#endif

#if WITH_EDITOR
public:
	virtual FString GetNodeDescription() const override;
	virtual UObject* GetAssetToEdit() override;
	virtual EDataValidationResult ValidateNode() override;
	
	virtual bool SupportsContextPins() const override { return true; }

	virtual TArray<FNarrativePin> GetContextInputs() override;
	virtual TArray<FNarrativePin> GetContextOutputs() override;

	// UObject
	virtual void PostLoad() override;
	virtual void PreEditChange(FProperty* PropertyAboutToChange) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	// --

private:
	void SubscribeToAssetChanges();
#endif
};
