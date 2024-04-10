// Copyright XiaoYao

#pragma once

#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"

#include "NarrativeSave.h"
#include "NarrativeTypes.h"
#include "NarrativeOwnerInterface.h"
#include "NarrativeComponent.generated.h"

class UNarrativeAsset;
class UNarrativeSubsystem;

USTRUCT()
struct FNotifyTagReplication
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag ActorTag;

	UPROPERTY()
	FGameplayTag NotifyTag;

	FNotifyTagReplication() {}

	FNotifyTagReplication(const FGameplayTag& InActorTag, const FGameplayTag& InNotifyTag)
		: ActorTag(InActorTag)
		, NotifyTag(InNotifyTag)
	{
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNarrativeComponentTagsReplicated, class UNarrativeComponent*, NarrativeComponent, const FGameplayTagContainer&, CurrentTags);

DECLARE_MULTICAST_DELEGATE_TwoParams(FNarrativeComponentNotify, class UNarrativeComponent*, const FGameplayTag&);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNarrativeComponentDynamicNotify, class UNarrativeComponent*, NarrativeComponent, const FGameplayTag&, NotifyTag);

/**
* Base component of Narrative System - makes possible to communicate between Actor, Narrative Subsystem and Narrative Graphs
*/
UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class NARRATIVE_API UNarrativeComponent : public UActorComponent, public INarrativeOwnerInterface
{
	GENERATED_UCLASS_BODY()

	friend class UNarrativeSubsystem;
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
//////////////////////////////////////////////////////////////////////////
// Identity Tags

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative")
	FGameplayTagContainer IdentityTags;

private:
	// Used to replicate tags added during gameplay
	UPROPERTY(ReplicatedUsing = OnRep_AddedIdentityTags)
	FGameplayTagContainer AddedIdentityTags;

	// Used to replicate tags removed during gameplay
	UPROPERTY(ReplicatedUsing = OnRep_RemovedIdentityTags)
	FGameplayTagContainer RemovedIdentityTags;

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "Narrative")
	void AddIdentityTag(const FGameplayTag Tag, const ENarrativeNetMode NetMode = ENarrativeNetMode::Authority);

	UFUNCTION(BlueprintCallable, Category = "Narrative")
	void AddIdentityTags(FGameplayTagContainer Tags, const ENarrativeNetMode NetMode = ENarrativeNetMode::Authority);

	UFUNCTION(BlueprintCallable, Category = "Narrative")
	void RemoveIdentityTag(const FGameplayTag Tag, const ENarrativeNetMode NetMode = ENarrativeNetMode::Authority);

	UFUNCTION(BlueprintCallable, Category = "Narrative")
	void RemoveIdentityTags(FGameplayTagContainer Tags, const ENarrativeNetMode NetMode = ENarrativeNetMode::Authority);

protected:
	void RegisterWithNarrativeSubsystem();
	void UnregisterWithNarrativeSubsystem();
	
private:
	UFUNCTION()
	void OnRep_AddedIdentityTags();

	UFUNCTION()
	void OnRep_RemovedIdentityTags();

public:
	UPROPERTY(BlueprintAssignable, Category = "Narrative")
	FNarrativeComponentTagsReplicated OnIdentityTagsAdded;

	UPROPERTY(BlueprintAssignable, Category = "Narrative")
	FNarrativeComponentTagsReplicated OnIdentityTagsRemoved;

public:
	void VerifyIdentityTags() const;
		
	UFUNCTION(BlueprintCallable, Category = "Narrative")
	void LogError(FString Message, const ENarrativeOnScreenMessageType OnScreenMessageType = ENarrativeOnScreenMessageType::Permanent) const;

//////////////////////////////////////////////////////////////////////////
// Component sending Notify Tags to Narrative Graph, or any other listener

private:
	// Stores only recently sent tags
	UPROPERTY(ReplicatedUsing = OnRep_SentNotifyTags)
	FGameplayTagContainer RecentlySentNotifyTags;

public:
	const FGameplayTagContainer& GetRecentlySentNotifyTags() const { return RecentlySentNotifyTags; }

	// Send single notification from the actor to Narrative graphs
	// If set on server, it always going to be replicated to clients
	UFUNCTION(BlueprintCallable, Category = "Narrative")
	void NotifyGraph(const FGameplayTag NotifyTag, const ENarrativeNetMode NetMode = ENarrativeNetMode::Authority);

	// Send multiple notifications at once - from the actor to Narrative graphs
	// If set on server, it always going to be replicated to clients
	UFUNCTION(BlueprintCallable, Category = "Narrative")
	void BulkNotifyGraph(const FGameplayTagContainer NotifyTags, const ENarrativeNetMode NetMode = ENarrativeNetMode::Authority);

private:
	UFUNCTION()
	void OnRep_SentNotifyTags();

public:
	FNarrativeComponentNotify OnNotifyFromComponent;

//////////////////////////////////////////////////////////////////////////
// Component receiving Notify Tags from Narrative Graph

private:
	// Stores only recently replicated tags
	UPROPERTY(ReplicatedUsing = OnRep_NotifyTagsFromGraph)
	FGameplayTagContainer NotifyTagsFromGraph;

public:
	virtual void NotifyFromGraph(const FGameplayTagContainer& NotifyTags, const ENarrativeNetMode NetMode = ENarrativeNetMode::Authority);

private:
	UFUNCTION()
	void OnRep_NotifyTagsFromGraph();

public:
	// Receive notification from Narrative graph or another Narrative Component
	UPROPERTY(BlueprintAssignable, Category = "Narrative")
	FNarrativeComponentDynamicNotify ReceiveNotify;

//////////////////////////////////////////////////////////////////////////
// Sending Notify Tags between Narrative components

private:
	// Stores only recently replicated tags
	UPROPERTY(ReplicatedUsing = OnRep_NotifyTagsFromAnotherComponent)
	TArray<FNotifyTagReplication> NotifyTagsFromAnotherComponent;

public:
	// Send notification to another actor containing Narrative Component
	UFUNCTION(BlueprintCallable, Category = "Narrative")
	virtual void NotifyActor(const FGameplayTag ActorTag, const FGameplayTag NotifyTag, const ENarrativeNetMode NetMode = ENarrativeNetMode::Authority);

private:
	UFUNCTION()
	void OnRep_NotifyTagsFromAnotherComponent();

//////////////////////////////////////////////////////////////////////////
// Root Narrative

public:
	// Asset that might instantiated as "Root Narrative" 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RootNarrative")
	UNarrativeAsset* RootNarrative;

	// If true, component will start Root Narrative on Begin Play
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RootNarrative")
	bool bAutoStartRootNarrative;

	// Networking mode for creating this Root Narrative
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RootNarrative")
	ENarrativeNetMode RootNarrativeMode;

	// If false, another Root Narrative instance won't be created from this component, if this Narrative Asset is already instantiated
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RootNarrative")
	bool bAllowMultipleInstances;

	UPROPERTY(SaveGame)
	FString SavedAssetInstanceName;
	
	// This will instantiate Narrative Asset assigned on this component.
	// Created Narrative Asset instance will be a "root flow", as additional Narrative Assets can be instantiated via Sub Graph node
	UFUNCTION(BlueprintCallable, Category = "RootNarrative")
	void StartRootNarrative();

	// This will destroy instantiated Narrative Asset - created from asset assigned on this component.
	UFUNCTION(BlueprintCallable, Category = "RootNarrative")
	void FinishRootNarrative(UNarrativeAsset* TemplateAsset, const ENarrativeFinishPolicy FinishPolicy);

	UFUNCTION(BlueprintPure, Category = "NarrativeSubsystem")
	TSet<UNarrativeAsset*> GetRootInstances(const UObject* Owner) const;

	UFUNCTION(BlueprintPure, Category = "RootNarrative", meta = (DeprecatedFunction, DeprecationMessage="Use GetRootInstances() instead."))
	UNarrativeAsset* GetRootNarrativeInstance() const;

//////////////////////////////////////////////////////////////////////////
// UNarrativeComponent overrideable events

public:
	// Called when a Root flow asset triggers a CustomOutput
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "OnTriggerRootNarrativeOutputEvent")
	void BP_OnTriggerRootNarrativeOutputEvent(UNarrativeAsset* RootNarrativeInstance, const FName& EventName);

	virtual void OnTriggerRootNarrativeOutputEvent(UNarrativeAsset* RootNarrativeInstance, const FName& EventName) {}

	// UNarrativeAsset-only access
	void OnTriggerRootNarrativeOutputEventDispatcher(UNarrativeAsset* RootNarrativeInstance, const FName& EventName);
	// ---

//////////////////////////////////////////////////////////////////////////
// SaveGame

public:
	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	virtual void SaveRootNarrative(TArray<FNarrativeAssetSaveData>& SavedNarrativeInstances);

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	virtual void LoadRootNarrative();

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	FNarrativeComponentSaveData SaveInstance();

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	bool LoadInstance();

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "SaveGame")
	void OnSave();
	
	UFUNCTION(BlueprintNativeEvent, Category = "SaveGame")
	void OnLoad();
	
//////////////////////////////////////////////////////////////////////////
// Helpers

public:
	UNarrativeSubsystem* GetNarrativeSubsystem() const;
	bool IsNarrativeNetMode(const ENarrativeNetMode NetMode) const;
};
