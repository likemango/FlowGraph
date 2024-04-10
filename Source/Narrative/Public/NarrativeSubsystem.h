// Copyright XiaoYao

#pragma once

#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "NarrativeComponent.h"
#include "NarrativeSubsystem.generated.h"

class UNarrativeAsset;
class UNarrativeNode_SubGraph;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSimpleNarrativeEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSimpleNarrativeComponentEvent, UNarrativeComponent*, Component);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTaggedNarrativeComponentEvent, UNarrativeComponent*, Component, const FGameplayTagContainer&, Tags);

DECLARE_DELEGATE_OneParam(FNativeNarrativeAssetEvent, class UNarrativeAsset*);

/**
 * Narrative Subsystem
 * - manages lifetime of Narrative Graphs
 * - connects Narrative Graphs with actors containing the Narrative Component
 * - convenient base for project-specific systems
 */
UCLASS()
class NARRATIVE_API UNarrativeSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UNarrativeSubsystem();

	friend class UNarrativeAsset;
	friend class UNarrativeComponent;
	friend class UNarrativeNode_SubGraph;

private:
	/* All asset templates with active instances */
	UPROPERTY()
	TArray<UNarrativeAsset*> InstancedTemplates;

	/* Assets instanced by object from another system, i.e. World Settings or Player Controller */
	UPROPERTY()
	TMap<UNarrativeAsset*, TWeakObjectPtr<UObject>> RootInstances;

	/* Assets instanced by Sub Graph nodes */
	UPROPERTY()
	TMap<UNarrativeNode_SubGraph*, UNarrativeAsset*> InstancedSubNarratives;

#if WITH_EDITOR
public:
	/* Called after creating the first instance of given Narrative Asset */
	static FNativeNarrativeAssetEvent OnInstancedTemplateAdded;

	/* Called just before removing the last instance of given Narrative Asset */
	static FNativeNarrativeAssetEvent OnInstancedTemplateRemoved;
#endif
	
protected:
	UPROPERTY()
	UNarrativeSaveGame* LoadedSaveGame;

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "NarrativeSubsystem")
	virtual void AbortActiveNarratives();

	/* Start the root Narrative, graph that will eventually instantiate next Narrative Graphs through the SubGraph node */
	UFUNCTION(BlueprintCallable, Category = "NarrativeSubsystem", meta = (DefaultToSelf = "Owner"))
	virtual void StartRootNarrative(UObject* Owner, UNarrativeAsset* NarrativeAsset, const bool bAllowMultipleInstances = true);

	virtual UNarrativeAsset* CreateRootNarrative(UObject* Owner, UNarrativeAsset* NarrativeAsset, const bool bAllowMultipleInstances = true);

	/* Finish Policy value is read by Narrative Node
	 * Nodes have opportunity to terminate themselves differently if Narrative Graph has been aborted
	 * Example: Spawn node might despawn all actors if Narrative Graph is aborted, not completed */
	UFUNCTION(BlueprintCallable, Category = "NarrativeSubsystem", meta = (DefaultToSelf = "Owner"))
	virtual void FinishRootNarrative(UObject* Owner, UNarrativeAsset* TemplateAsset, const ENarrativeFinishPolicy FinishPolicy);

	/* Finish Policy value is read by Narrative Node
	 * Nodes have opportunity to terminate themselves differently if Narrative Graph has been aborted
	 * Example: Spawn node might despawn all actors if Narrative Graph is aborted, not completed */
	UFUNCTION(BlueprintCallable, Category = "NarrativeSubsystem", meta = (DefaultToSelf = "Owner"))
	virtual void FinishAllRootNarratives(UObject* Owner, const ENarrativeFinishPolicy FinishPolicy);

protected:
	UNarrativeAsset* CreateSubNarrative(UNarrativeNode_SubGraph* SubGraphNode, const FString SavedInstanceName = FString(), const bool bPreloading = false);
	void RemoveSubNarrative(UNarrativeNode_SubGraph* SubGraphNode, const ENarrativeFinishPolicy FinishPolicy);

	UNarrativeAsset* CreateNarrativeInstance(const TWeakObjectPtr<UObject> Owner, TSoftObjectPtr<UNarrativeAsset> NarrativeAsset, FString NewInstanceName = FString());

	virtual void AddInstancedTemplate(UNarrativeAsset* Template);
	virtual void RemoveInstancedTemplate(UNarrativeAsset* Template);

public:
	/* Returns all assets instanced by object from another system like World Settings */
	UFUNCTION(BlueprintPure, Category = "NarrativeSubsystem")
	TMap<UObject*, UNarrativeAsset*> GetRootInstances() const;
	
	/* Returns asset instanced by specific object */
	UFUNCTION(BlueprintPure, Category = "NarrativeSubsystem")
	TSet<UNarrativeAsset*> GetRootInstancesByOwner(const UObject* Owner) const;

	UFUNCTION(BlueprintPure, Category = "NarrativeSubsystem", meta = (DeprecatedFunction, DeprecationMessage="Use GetRootInstancesByOwner() instead."))
	UNarrativeAsset* GetRootNarrative(const UObject* Owner) const;

	/* Returns assets instanced by Sub Graph nodes */
	UFUNCTION(BlueprintPure, Category = "NarrativeSubsystem")
	const TMap<UNarrativeNode_SubGraph*, UNarrativeAsset*>& GetInstancedSubNarratives() const { return InstancedSubNarratives; }

	virtual UWorld* GetWorld() const override;

//////////////////////////////////////////////////////////////////////////
// SaveGame support

	UPROPERTY(BlueprintAssignable, Category = "NarrativeSubsystem")
	FSimpleNarrativeEvent OnSaveGame;

	UFUNCTION(BlueprintCallable, Category = "NarrativeSubsystem")
	virtual void OnGameSaved(UNarrativeSaveGame* SaveGame);

	UFUNCTION(BlueprintCallable, Category = "NarrativeSubsystem")
	virtual void OnGameLoaded(UNarrativeSaveGame* SaveGame);

	UFUNCTION(BlueprintCallable, Category = "NarrativeSubsystem")
	virtual void LoadRootNarrative(UObject* Owner, UNarrativeAsset* NarrativeAsset, const FString& SavedAssetInstanceName);

	UFUNCTION(BlueprintCallable, Category = "NarrativeSubsystem")
	virtual void LoadSubNarrative(UNarrativeNode_SubGraph* SubGraphNode, const FString& SavedAssetInstanceName);

	UFUNCTION(BlueprintPure, Category = "NarrativeSubsystem")
	UNarrativeSaveGame* GetLoadedSaveGame() const { return LoadedSaveGame; }

//////////////////////////////////////////////////////////////////////////
// Component Registry

protected:
	/* All the Narrative Components currently existing in the world */
	TMultiMap<FGameplayTag, TWeakObjectPtr<UNarrativeComponent>> NarrativeComponentRegistry;

protected:
	virtual void RegisterComponent(UNarrativeComponent* Component);
	virtual void OnIdentityTagAdded(UNarrativeComponent* Component, const FGameplayTag& AddedTag);
	virtual void OnIdentityTagsAdded(UNarrativeComponent* Component, const FGameplayTagContainer& AddedTags);

	virtual void UnregisterComponent(UNarrativeComponent* Component);
	virtual void OnIdentityTagRemoved(UNarrativeComponent* Component, const FGameplayTag& RemovedTag);
	virtual void OnIdentityTagsRemoved(UNarrativeComponent* Component, const FGameplayTagContainer& RemovedTags);

public:
	/* Called when actor with Narrative Component appears in the world */
	UPROPERTY(BlueprintAssignable, Category = "NarrativeSubsystem")
	FSimpleNarrativeComponentEvent OnComponentRegistered;

	/* Called after adding Identity Tags to already registered Narrative Component
	 * This can happen only after Begin Play occured in the component */
	UPROPERTY(BlueprintAssignable, Category = "NarrativeSubsystem")
	FTaggedNarrativeComponentEvent OnComponentTagAdded;

	/* Called when actor with Narrative Component disappears from the world */
	UPROPERTY(BlueprintAssignable, Category = "NarrativeSubsystem")
	FSimpleNarrativeComponentEvent OnComponentUnregistered;

	/* Called after removing Identity Tags from the Narrative Component, if component still has some Identity Tags
	 * This can happen only after Begin Play occured in the component */
	UPROPERTY(BlueprintAssignable, Category = "NarrativeSubsystem")
	FTaggedNarrativeComponentEvent OnComponentTagRemoved;

	/**
	 * Returns all registered Narrative Components identified by given tag
	 * 
	 * @param Tag Tag to check if it matches Identity Tags of registered Narrative Components
	 * @param ComponentClass Only components matching this class we'll be returned
	 * @param bExactMatch If true, the tag has to be exactly present, if false then TagContainer will include it's parent tags while matching. Be careful, using latter option may be very expensive, as the search cost is proportional to the number of registered Gameplay Tags!
	 */
	UFUNCTION(BlueprintPure, Category = "NarrativeSubsystem", meta = (DeterminesOutputType = "ComponentClass"))
	TSet<UNarrativeComponent*> GetNarrativeComponentsByTag(const FGameplayTag Tag, const TSubclassOf<UNarrativeComponent> ComponentClass, const bool bExactMatch = true) const;

	/**
	 * Returns all registered Narrative Components identified by Any or All provided tags
	 * 
	 * @param Tags Container to check if it matches Identity Tags of registered Narrative Components
	 * @param MatchType If Any, returned component needs to have only one of given tags. If All, component needs to have all given Identity Tags
	 * @param ComponentClass Only components matching this class we'll be returned
	* @param bExactMatch If true, the tag has to be exactly present, if false then TagContainer will include it's parent tags while matching. Be careful, using latter option may be very expensive, as the search cost is proportional to the number of registered Gameplay Tags!
	 */
	UFUNCTION(BlueprintPure, Category = "NarrativeSubsystem", meta = (DeterminesOutputType = "ComponentClass"))
	TSet<UNarrativeComponent*> GetNarrativeComponentsByTags(const FGameplayTagContainer Tags, const EGameplayContainerMatchType MatchType, const TSubclassOf<UNarrativeComponent> ComponentClass, const bool bExactMatch = true) const;

	/**
	 * Returns all registered actors with Narrative Component identified by given tag
	 * 
	 * @param Tag Tag to check if it matches Identity Tags of registered Narrative Components
	 * @param ActorClass Only actors matching this class we'll be returned
	 * @param bExactMatch If true, the tag has to be exactly present, if false then TagContainer will include it's parent tags while matching. Be careful, using latter option may be very expensive, as the search cost is proportional to the number of registered Gameplay Tags!
	 */
	UFUNCTION(BlueprintPure, Category = "NarrativeSubsystem", meta = (DeterminesOutputType = "ActorClass"))
	TSet<AActor*> GetNarrativeActorsByTag(const FGameplayTag Tag, const TSubclassOf<AActor> ActorClass, const bool bExactMatch = true) const;

	/**
	 * Returns all registered actors with Narrative Component identified by Any or All provided tags
	 * 
	 * @param Tags Container to check if it matches Identity Tags of registered Narrative Components
	 * @param MatchType If Any, returned component needs to have only one of given tags. If All, component needs to have all given Identity Tags
	 * @param ActorClass Only actors matching this class we'll be returned
	 * @param bExactMatch If true, the tag has to be exactly present, if false then TagContainer will include it's parent tags while matching. Be careful, using latter option may be very expensive, as the search cost is proportional to the number of registered Gameplay Tags!
	 */
	UFUNCTION(BlueprintPure, Category = "NarrativeSubsystem", meta = (DeterminesOutputType = "ActorClass"))
	TSet<AActor*> GetNarrativeActorsByTags(const FGameplayTagContainer Tags, const EGameplayContainerMatchType MatchType, const TSubclassOf<AActor> ActorClass, const bool bExactMatch = true) const;

	/**
	 * Returns all registered actors as pairs: Actor as key, its Narrative Component as value
	 * 
	 * @param Tag Tag to check if it matches Identity Tags of registered Narrative Components
	 * @param ActorClass Only actors matching this class we'll be returned
	 * @param bExactMatch If true, the tag has to be exactly present, if false then TagContainer will include it's parent tags while matching. Be careful, using latter option may be very expensive, as the search cost is proportional to the number of registered Gameplay Tags!
	 */
	UFUNCTION(BlueprintPure, Category = "NarrativeSubsystem", meta = (DeterminesOutputType = "ActorClass"))
	TMap<AActor*, UNarrativeComponent*> GetNarrativeActorsAndComponentsByTag(const FGameplayTag Tag, const TSubclassOf<AActor> ActorClass, const bool bExactMatch = true) const;

	/**
	 * Returns all registered actors as pairs: Actor as key, its Narrative Component as value
	 * 
	 * @param Tags Container to check if it matches Identity Tags of registered Narrative Components
	 * @param MatchType If Any, returned component needs to have only one of given tags. If All, component needs to have all given Identity Tags
	 * @param ActorClass Only actors matching this class we'll be returned
	 * @param bExactMatch If true, the tag has to be exactly present, if false then TagContainer will include it's parent tags while matching. Be careful, using latter option may be very expensive, as the search cost is proportional to the number of registered Gameplay Tags!
	 */
	UFUNCTION(BlueprintPure, Category = "NarrativeSubsystem", meta = (DeterminesOutputType = "ActorClass"))
	TMap<AActor*, UNarrativeComponent*> GetNarrativeActorsAndComponentsByTags(const FGameplayTagContainer Tags, const EGameplayContainerMatchType MatchType, const TSubclassOf<AActor> ActorClass, const bool bExactMatch = true) const;

	/**
	 * Returns all registered Narrative Components identified by given tag
	 * 
	 * @tparam T Only components matching this class we'll be returned
	 * @param Tag Tag to check if it matches Identity Tags of registered Narrative Components
	 * @param bExactMatch If true, the tag has to be exactly present, if false then TagContainer will include it's parent tags while matching. Be careful, using latter option may be very expensive, as the search cost is proportional to the number of registered Gameplay Tags!
	 */
	template <class T>
	TSet<TWeakObjectPtr<T>> GetComponents(const FGameplayTag& Tag, const bool bExactMatch = true) const
	{
		static_assert(TPointerIsConvertibleFromTo<T, const UActorComponent>::Value, "'T' template parameter to GetComponents must be derived from UActorComponent");

		TArray<TWeakObjectPtr<UNarrativeComponent>> FoundComponents;
		FindComponents(Tag, bExactMatch, FoundComponents);

		TSet<TWeakObjectPtr<T>> Result;
		for (const TWeakObjectPtr<UNarrativeComponent>& Component : FoundComponents)
		{
			if (Component.IsValid())
			{
				if (T* ComponentOfClass = Cast<T>(Component))
				{
					Result.Emplace(ComponentOfClass);
				}
			}
		}

		return Result;
	}

	/**
	 * Returns all registered Narrative Components identified by Any or All provided tags
	 * 
	 * @tparam T Only components matching this class we'll be returned
	 * @param Tags Container to check if it matches Identity Tags of registered Narrative Components
	 * @param MatchType If Any, returned component needs to have only one of given tags. If All, component needs to have all given Identity Tags
	 * @param bExactMatch If true, the tag has to be exactly present, if false then TagContainer will include it's parent tags while matching. Be careful, using latter option may be very expensive, as the search cost is proportional to the number of registered Gameplay Tags!
	 */
	template <class T>
	TSet<TWeakObjectPtr<T>> GetComponents(const FGameplayTagContainer& Tags, const EGameplayContainerMatchType MatchType, const bool bExactMatch = true) const
	{
		static_assert(TPointerIsConvertibleFromTo<T, const UActorComponent>::Value, "'T' template parameter to GetComponents must be derived from UActorComponent");

		TSet<TWeakObjectPtr<UNarrativeComponent>> FoundComponents;
		FindComponents(Tags, MatchType, bExactMatch, FoundComponents);

		TSet<TWeakObjectPtr<T>> Result;
		for (const TWeakObjectPtr<UNarrativeComponent>& Component : FoundComponents)
		{
			if (Component.IsValid())
			{
				if (T* ComponentOfClass = Cast<T>(Component))
				{
					Result.Emplace(ComponentOfClass);
				}
			}
		}

		return Result;
	}

	/**
	 * Returns all registered Narrative Components identified by given tag
	 * 
	 * @tparam T Only components matching this class we'll be returned
	 * @param Tag Tag to check if it matches Identity Tags of registered Narrative Components
	 * @param bExactMatch If true, the tag has to be exactly present, if false then TagContainer will include it's parent tags while matching. Be careful, using latter option may be very expensive, as the search cost is proportional to the number of registered Gameplay Tags!
	 */
	template <class T>
	TSet<TWeakObjectPtr<T>> GetActors(const FGameplayTag& Tag, const bool bExactMatch = true) const
	{
		static_assert(TPointerIsConvertibleFromTo<T, const AActor>::Value, "'T' template parameter to GetActors must be derived from AActor");

		TArray<TWeakObjectPtr<UNarrativeComponent>> FoundComponents;
		FindComponents(Tag, bExactMatch, FoundComponents);

		TSet<TWeakObjectPtr<T>> Result;
		for (const TWeakObjectPtr<UNarrativeComponent>& Component : FoundComponents)
		{
			if (Component.IsValid())
			{
				if (T* ActorOfClass = Cast<T>(Component->GetOwner()))
				{
					Result.Emplace(ActorOfClass);
				}
			}
		}

		return Result;
	}

	/**
	 * Returns all registered Narrative Components identified by Any or All provided tags
	 * 
	 * @tparam T Only actors matching this class we'll be returned
	 * @param Tags Container to check if it matches Identity Tags of registered Narrative Components
	 * @param MatchType If Any, returned component needs to have only one of given tags. If All, component needs to have all given Identity Tags
	 * @param bExactMatch If true, the tag has to be exactly present, if false then TagContainer will include it's parent tags while matching. Be careful, using latter option may be very expensive, as the search cost is proportional to the number of registered Gameplay Tags!
	 */
	template <class T>
	TSet<TWeakObjectPtr<T>> GetActors(const FGameplayTagContainer& Tags, const EGameplayContainerMatchType MatchType, const bool bExactMatch = true) const
	{
		static_assert(TPointerIsConvertibleFromTo<T, const AActor>::Value, "'T' template parameter to GetActors must be derived from AActor");

		TSet<TWeakObjectPtr<UNarrativeComponent>> FoundComponents;
		FindComponents(Tags, MatchType, bExactMatch, FoundComponents);

		TSet<TWeakObjectPtr<T>> Result;
		for (const TWeakObjectPtr<UNarrativeComponent>& Component : FoundComponents)
		{
			if (Component.IsValid())
			{
				if (T* ActorOfClass = Cast<T>(Component->GetOwner()))
				{
					Result.Emplace(ActorOfClass);
				}
			}
		}

		return Result;
	}

	/**
	 * Returns all registered actors with Narrative Component identified by given tag
	 * 
	 * @tparam ActorT Only actors matching this class we'll be returned
	 * @tparam ComponentT Only components matching this class we'll be returned
	 * @param Tag Tag to check if it matches Identity Tags of registered Narrative Components
	 * @param bExactMatch If true, the tag has to be exactly present, if false then TagContainer will include it's parent tags while matching. Be careful, using latter option may be very expensive, as the search cost is proportional to the number of registered Gameplay Tags!
	 */
	template <class ActorT, class ComponentT>
	TMap<TWeakObjectPtr<ActorT>, TWeakObjectPtr<ComponentT>> GetActorsAndComponents(const FGameplayTag& Tag, const bool bExactMatch = true) const
	{
		static_assert(TPointerIsConvertibleFromTo<ActorT, const AActor>::Value, "'ActorT' template parameter to GetActorsAndComponents must be derived from AActor");
		static_assert(TPointerIsConvertibleFromTo<ComponentT, const UActorComponent>::Value, "'ComponentT' template parameter to GetActorsAndComponents must be derived from UActorComponent");

		TArray<TWeakObjectPtr<UNarrativeComponent>> FoundComponents;
		FindComponents(Tag, bExactMatch, FoundComponents);

		TMap<TWeakObjectPtr<ActorT>, TWeakObjectPtr<ComponentT>> Result;
		for (const TWeakObjectPtr<UNarrativeComponent>& Component : FoundComponents)
		{
			if (Component.IsValid())
			{
				ComponentT* ComponentOfClass = Cast<ComponentT>(Component);
				ActorT* ActorOfClass = Cast<ActorT>(Component->GetOwner());
				if (ComponentOfClass && ActorOfClass)
				{
					Result.Emplace(ActorOfClass, ComponentOfClass);
				}
			}
		}

		return Result;
	}

	/**
	 * Returns all registered actors with Narrative Component identified by Any or All provided tags
	 * 
	 * @tparam ActorT Only actors matching this class we'll be returned
	 * @tparam ComponentT Only components matching this class we'll be returned
	 * @param Tags Container to check if it matches Identity Tags of registered Narrative Components
	 * @param MatchType If Any, returned component needs to have only one of given tags. If All, component needs to have all given Identity Tags
	 * @param bExactMatch If true, the tag has to be exactly present, if false then TagContainer will include it's parent tags while matching. Be careful, using latter option may be very expensive, as the search cost is proportional to the number of registered Gameplay Tags!
	 */
	template <class ActorT, class ComponentT>
	TMap<TWeakObjectPtr<ActorT>, TWeakObjectPtr<ComponentT>> GetActorsAndComponents(const FGameplayTagContainer& Tags, const EGameplayContainerMatchType MatchType, const bool bExactMatch = true) const
	{
		static_assert(TPointerIsConvertibleFromTo<ActorT, const AActor>::Value, "'ActorT' template parameter to GetActorsAndComponents must be derived from AActor");
		static_assert(TPointerIsConvertibleFromTo<ComponentT, const UActorComponent>::Value, "'ComponentT' template parameter to GetActorsAndComponents must be derived from UActorComponent");

		TSet<TWeakObjectPtr<UNarrativeComponent>> FoundComponents;
		FindComponents(Tags, MatchType, bExactMatch, FoundComponents);

		TMap<TWeakObjectPtr<ActorT>, TWeakObjectPtr<ComponentT>> Result;
		for (const TWeakObjectPtr<UNarrativeComponent>& Component : FoundComponents)
		{
			if (Component.IsValid())
			{
				ComponentT* ComponentOfClass = Cast<ComponentT>(Component);
				ActorT* ActorOfClass = Cast<ActorT>(Component->GetOwner());
				if (ComponentOfClass && ActorOfClass)
				{
					Result.Emplace(ActorOfClass, ComponentOfClass);
				}
			}
		}

		return Result;
	}

private:
	void FindComponents(const FGameplayTag& Tag, const bool bExactMatch, TArray<TWeakObjectPtr<UNarrativeComponent>>& OutComponents) const;
	void FindComponents(const FGameplayTagContainer& Tags, const EGameplayContainerMatchType MatchType, const bool bExactMatch, TSet<TWeakObjectPtr<UNarrativeComponent>>& OutComponents) const;
};
