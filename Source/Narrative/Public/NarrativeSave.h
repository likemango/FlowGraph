// Copyright XiaoYao

#pragma once

#include "GameFramework/SaveGame.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "NarrativeSave.generated.h"

USTRUCT(BlueprintType)
struct NARRATIVE_API FNarrativeNodeSaveData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(SaveGame, VisibleAnywhere, Category = "Narrative")
	FGuid NodeGuid;

	UPROPERTY(SaveGame, VisibleAnywhere, Category = "Narrative")
	TArray<uint8> NodeData;

	friend FArchive& operator<<(FArchive& Ar, FNarrativeNodeSaveData& InNodeData)
	{
		return Ar;
	}
};

USTRUCT(BlueprintType)
struct NARRATIVE_API FNarrativeAssetSaveData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(SaveGame, VisibleAnywhere, Category = "Narrative")
	FString WorldName;

	UPROPERTY(SaveGame, VisibleAnywhere, Category = "Narrative")
	FString InstanceName;

	UPROPERTY(SaveGame, VisibleAnywhere, Category = "Narrative")
	TArray<uint8> AssetData;

	UPROPERTY(SaveGame, VisibleAnywhere, Category = "Narrative")
	TArray<FNarrativeNodeSaveData> NodeRecords;

	friend FArchive& operator<<(FArchive& Ar, FNarrativeAssetSaveData& InAssetData)
	{
		return Ar;
	}
};

USTRUCT(BlueprintType)
struct NARRATIVE_API FNarrativeComponentSaveData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(SaveGame, VisibleAnywhere, Category = "Narrative")
	FString WorldName;

	UPROPERTY(SaveGame, VisibleAnywhere, Category = "Narrative")
	FString ActorInstanceName;

	UPROPERTY(SaveGame)
	TArray<uint8> ComponentData;

	friend FArchive& operator<<(FArchive& Ar, FNarrativeComponentSaveData& InComponentData)
	{
		return Ar;
	}
};

struct NARRATIVE_API FNarrativeArchive : public FObjectAndNameAsStringProxyArchive
{
	FNarrativeArchive(FArchive& InInnerArchive) : FObjectAndNameAsStringProxyArchive(InInnerArchive, true)
	{
		ArIsSaveGame = true;
	}
};

UCLASS(BlueprintType)
class NARRATIVE_API UNarrativeSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UNarrativeSaveGame() {};

	UPROPERTY(VisibleAnywhere, Category = "SaveGame")
	FString SaveSlotName = TEXT("NarrativeSave");

	UPROPERTY(VisibleAnywhere, Category = "Narrative")
	TArray<FNarrativeComponentSaveData> NarrativeComponents;

	UPROPERTY(VisibleAnywhere, Category = "Narrative")
	TArray<FNarrativeAssetSaveData> NarrativeInstances;
	
	friend FArchive& operator<<(FArchive& Ar, UNarrativeSaveGame& SaveGame)
	{
		Ar << SaveGame.NarrativeComponents;
		Ar << SaveGame.NarrativeInstances;
		return Ar;
	}
};
