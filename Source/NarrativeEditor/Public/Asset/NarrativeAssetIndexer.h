// Copyright XiaoYao

#pragma once

#include "CoreMinimal.h"

#include "IAssetIndexer.h"

class UNarrativeAsset;
class FSearchSerializer;

/**
 * Documentation: https://github.com/MothCocoon/NarrativeGraph/wiki/Asset-Search
 */
class NARRATIVEEDITOR_API FNarrativeAssetIndexer : public IAssetIndexer
{
public:
	virtual FString GetName() const override { return TEXT("NarrativeAsset"); }
	virtual int32 GetVersion() const override;
	virtual void IndexAsset(const UObject* InAssetObject, FSearchSerializer& Serializer) const override;

private:
	// Variant of FBlueprintIndexer::IndexGraphs
	void IndexGraph(const UNarrativeAsset* InNarrativeAsset, FSearchSerializer& Serializer) const;
};
