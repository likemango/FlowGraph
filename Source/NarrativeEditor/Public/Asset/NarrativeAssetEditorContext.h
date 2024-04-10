// Copyright XiaoYao

#pragma once

#include "CoreMinimal.h"

#include "NarrativeAssetEditorContext.generated.h"

class UNarrativeAsset;
class FNarrativeAssetEditor;

UCLASS()
class NARRATIVEEDITOR_API UNarrativeAssetEditorContext : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Tool Menus")
	UNarrativeAsset* GetNarrativeAsset() const;

	TWeakPtr<FNarrativeAssetEditor> NarrativeAssetEditor;
};
