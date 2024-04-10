// Copyright XiaoYao

#include "NarrativeSubsystem.h"

#include "NarrativeAsset.h"
#include "NarrativeComponent.h"
#include "NarrativeLogChannels.h"
#include "NarrativeSave.h"
#include "NarrativeSettings.h"
#include "Nodes/Route/NarrativeNode_SubGraph.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Logging/MessageLog.h"
#include "Misc/Paths.h"
#include "UObject/UObjectHash.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativeSubsystem)

#if WITH_EDITOR
FNativeNarrativeAssetEvent UNarrativeSubsystem::OnInstancedTemplateAdded;
FNativeNarrativeAssetEvent UNarrativeSubsystem::OnInstancedTemplateRemoved;
#endif

#define LOCTEXT_NAMESPACE "NarrativeSubsystem"

UNarrativeSubsystem::UNarrativeSubsystem()
	: LoadedSaveGame(nullptr)
{
}

bool UNarrativeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	// Only create an instance if there is no override implementation defined elsewhere
	TArray<UClass*> ChildClasses;
	GetDerivedClasses(GetClass(), ChildClasses, false);
	if (ChildClasses.Num() > 0)
	{
		return false;
	}

	// in this case, we simply create subsystem for every instance of the game
	if (UNarrativeSettings::Get()->bCreateNarrativeSubsystemOnClients)
	{
		return true;
	}

	return Outer->GetWorld()->GetNetMode() < NM_Client;
}

void UNarrativeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
}

void UNarrativeSubsystem::Deinitialize()
{
	AbortActiveNarratives();
}

void UNarrativeSubsystem::AbortActiveNarratives()
{
	if (InstancedTemplates.Num() > 0)
	{
		for (int32 i = InstancedTemplates.Num() - 1; i >= 0; i--)
		{
			if (InstancedTemplates.IsValidIndex(i) && InstancedTemplates[i])
			{
				InstancedTemplates[i]->ClearInstances();
			}
		}
	}

	InstancedTemplates.Empty();
	InstancedSubNarratives.Empty();

	RootInstances.Empty();
}

void UNarrativeSubsystem::StartRootNarrative(UObject* Owner, UNarrativeAsset* NarrativeAsset, const bool bAllowMultipleInstances /* = true */)
{
	if (NarrativeAsset)
	{
		if (UNarrativeAsset* NewNarrative = CreateRootNarrative(Owner, NarrativeAsset, bAllowMultipleInstances))
		{
			NewNarrative->StartNarrative();
		}
	}
#if WITH_EDITOR
	else
	{
		FMessageLog("PIE").Error(LOCTEXT("StartRootNarrativeNullAsset", "Attempted to start Root Narrative with a null asset."))
		                  ->AddToken(FUObjectToken::Create(Owner));
	}
#endif
}

UNarrativeAsset* UNarrativeSubsystem::CreateRootNarrative(UObject* Owner, UNarrativeAsset* NarrativeAsset, const bool bAllowMultipleInstances)
{
	for (const TPair<UNarrativeAsset*, TWeakObjectPtr<UObject>>& RootInstance : RootInstances)
	{
		if (Owner == RootInstance.Value.Get() && NarrativeAsset == RootInstance.Key->GetTemplateAsset())
		{
			UE_LOG(LogNarrative, Warning, TEXT("Attempted to start Root Narrative for the same Owner again. Owner: %s. Narrative Asset: %s."), *Owner->GetName(), *NarrativeAsset->GetName());
			return nullptr;
		}
	}

	if (!bAllowMultipleInstances && InstancedTemplates.Contains(NarrativeAsset))
	{
		UE_LOG(LogNarrative, Warning, TEXT("Attempted to start Root Narrative, although there can be only a single instance. Owner: %s. Narrative Asset: %s."), *Owner->GetName(), *NarrativeAsset->GetName());
		return nullptr;
	}

	UNarrativeAsset* NewNarrative = CreateNarrativeInstance(Owner, NarrativeAsset);
	if (NewNarrative)
	{
		RootInstances.Add(NewNarrative, Owner);
	}

	return NewNarrative;
}

void UNarrativeSubsystem::FinishRootNarrative(UObject* Owner, UNarrativeAsset* TemplateAsset, const ENarrativeFinishPolicy FinishPolicy)
{
	UNarrativeAsset* InstanceToFinish = nullptr;

	for (TPair<UNarrativeAsset*, TWeakObjectPtr<UObject>>& RootInstance : RootInstances)
	{
		if (Owner && Owner == RootInstance.Value.Get() && RootInstance.Key && RootInstance.Key->GetTemplateAsset() == TemplateAsset)
		{
			InstanceToFinish = RootInstance.Key;
			break;
		}
	}

	if (InstanceToFinish)
	{
		RootInstances.Remove(InstanceToFinish);
		InstanceToFinish->FinishNarrative(FinishPolicy);
	}
}

void UNarrativeSubsystem::FinishAllRootNarratives(UObject* Owner, const ENarrativeFinishPolicy FinishPolicy)
{
	TArray<UNarrativeAsset*> InstancesToFinish;

	for (TPair<UNarrativeAsset*, TWeakObjectPtr<UObject>>& RootInstance : RootInstances)
	{
		if (Owner && Owner == RootInstance.Value.Get() && RootInstance.Key)
		{
			InstancesToFinish.Emplace(RootInstance.Key);
		}
	}

	for (UNarrativeAsset* InstanceToFinish : InstancesToFinish)
	{
		RootInstances.Remove(InstanceToFinish);
		InstanceToFinish->FinishNarrative(FinishPolicy);
	}
}

UNarrativeAsset* UNarrativeSubsystem::CreateSubNarrative(UNarrativeNode_SubGraph* SubGraphNode, const FString SavedInstanceName, const bool bPreloading /* = false */)
{
	UNarrativeAsset* NewInstance = nullptr;

	if (!InstancedSubNarratives.Contains(SubGraphNode))
	{
		const TWeakObjectPtr<UObject> Owner = SubGraphNode->GetNarrativeAsset() ? SubGraphNode->GetNarrativeAsset()->GetOwner() : nullptr;
		NewInstance = CreateNarrativeInstance(Owner, SubGraphNode->Asset, SavedInstanceName);

		if (NewInstance)
		{
			InstancedSubNarratives.Add(SubGraphNode, NewInstance);

			if (bPreloading)
			{
				NewInstance->PreloadNodes();
			}
		}
	}

	if (InstancedSubNarratives.Contains(SubGraphNode) && !bPreloading)
	{
		// get instanced asset from map - in case it was already instanced by calling CreateSubNarrative() with bPreloading == true
		UNarrativeAsset* AssetInstance = InstancedSubNarratives[SubGraphNode];

		AssetInstance->NodeOwningThisAssetInstance = SubGraphNode;
		SubGraphNode->GetNarrativeAsset()->ActiveSubGraphs.Add(SubGraphNode, AssetInstance);

		// don't activate Start Node if we're loading Sub Graph from SaveGame
		if (SavedInstanceName.IsEmpty())
		{
			AssetInstance->StartNarrative();
		}
	}

	return NewInstance;
}

void UNarrativeSubsystem::RemoveSubNarrative(UNarrativeNode_SubGraph* SubGraphNode, const ENarrativeFinishPolicy FinishPolicy)
{
	if (InstancedSubNarratives.Contains(SubGraphNode))
	{
		UNarrativeAsset* AssetInstance = InstancedSubNarratives[SubGraphNode];
		AssetInstance->NodeOwningThisAssetInstance = nullptr;

		SubGraphNode->GetNarrativeAsset()->ActiveSubGraphs.Remove(SubGraphNode);
		InstancedSubNarratives.Remove(SubGraphNode);

		AssetInstance->FinishNarrative(FinishPolicy);
	}
}

UNarrativeAsset* UNarrativeSubsystem::CreateNarrativeInstance(const TWeakObjectPtr<UObject> Owner, TSoftObjectPtr<UNarrativeAsset> NarrativeAsset, FString NewInstanceName)
{
	UNarrativeAsset* LoadedNarrativeAsset = NarrativeAsset.LoadSynchronous();
	if (LoadedNarrativeAsset == nullptr)
	{
		return nullptr;
	}

	AddInstancedTemplate(LoadedNarrativeAsset);

#if WITH_EDITOR
	if (GetWorld()->WorldType != EWorldType::Game)
	{
		// Fix connections - even in packaged game if assets haven't been re-saved in the editor after changing node's definition
		LoadedNarrativeAsset->HarvestNodeConnections();
	}
#endif

	// it won't be empty, if we're restoring Narrative Asset instance from the SaveGame
	if (NewInstanceName.IsEmpty())
	{
		NewInstanceName = MakeUniqueObjectName(this, UNarrativeAsset::StaticClass(), *FPaths::GetBaseFilename(LoadedNarrativeAsset->GetPathName())).ToString();
	}

	UNarrativeAsset* NewInstance = NewObject<UNarrativeAsset>(this, LoadedNarrativeAsset->GetClass(), *NewInstanceName, RF_Transient, LoadedNarrativeAsset, false, nullptr);
	NewInstance->InitializeInstance(Owner, LoadedNarrativeAsset);

	LoadedNarrativeAsset->AddInstance(NewInstance);

	return NewInstance;
}

void UNarrativeSubsystem::AddInstancedTemplate(UNarrativeAsset* Template)
{
	if (!InstancedTemplates.Contains(Template))
	{
		InstancedTemplates.Add(Template);

#if WITH_EDITOR
		Template->RuntimeLog = MakeShareable(new FNarrativeMessageLog());
		OnInstancedTemplateAdded.ExecuteIfBound(Template);
#endif
	}
}

void UNarrativeSubsystem::RemoveInstancedTemplate(UNarrativeAsset* Template)
{
#if WITH_EDITOR
	OnInstancedTemplateRemoved.ExecuteIfBound(Template);
	Template->RuntimeLog.Reset();
#endif

	InstancedTemplates.Remove(Template);
}

TMap<UObject*, UNarrativeAsset*> UNarrativeSubsystem::GetRootInstances() const
{
	TMap<UObject*, UNarrativeAsset*> Result;
	for (const TPair<UNarrativeAsset*, TWeakObjectPtr<UObject>>& RootInstance : RootInstances)
	{
		Result.Emplace(RootInstance.Value.Get(), RootInstance.Key);
	}
	return Result;
}

TSet<UNarrativeAsset*> UNarrativeSubsystem::GetRootInstancesByOwner(const UObject* Owner) const
{
	TSet<UNarrativeAsset*> Result;
	for (const TPair<UNarrativeAsset*, TWeakObjectPtr<UObject>>& RootInstance : RootInstances)
	{
		if (Owner && RootInstance.Value == Owner)
		{
			Result.Emplace(RootInstance.Key);
		}
	}
	return Result;
}

UNarrativeAsset* UNarrativeSubsystem::GetRootNarrative(const UObject* Owner) const
{
	const TSet<UNarrativeAsset*> Result = GetRootInstancesByOwner(Owner);
	if (Result.Num() > 0)
	{
		return Result.Array()[0];
	}

	return nullptr;
}

UWorld* UNarrativeSubsystem::GetWorld() const
{
	return GetGameInstance()->GetWorld();
}

void UNarrativeSubsystem::OnGameSaved(UNarrativeSaveGame* SaveGame)
{
	// clear existing data, in case we received reused SaveGame instance
	// we only remove data for the current world + global Narrative Graph instances (i.e. not bound to any world if created by UGameInstanceSubsystem)
	// we keep data bound to other worlds
	if (GetWorld())
	{
		const FString& WorldName = GetWorld()->GetName();

		for (int32 i = SaveGame->NarrativeInstances.Num() - 1; i >= 0; i--)
		{
			if (SaveGame->NarrativeInstances[i].WorldName.IsEmpty() || SaveGame->NarrativeInstances[i].WorldName == WorldName)
			{
				SaveGame->NarrativeInstances.RemoveAt(i);
			}
		}

		for (int32 i = SaveGame->NarrativeComponents.Num() - 1; i >= 0; i--)
		{
			if (SaveGame->NarrativeComponents[i].WorldName.IsEmpty() || SaveGame->NarrativeComponents[i].WorldName == WorldName)
			{
				SaveGame->NarrativeComponents.RemoveAt(i);
			}
		}
	}

	// save Narrative Graphs
	for (const TPair<UNarrativeAsset*, TWeakObjectPtr<UObject>>& RootInstance : RootInstances)
	{
		if (RootInstance.Key && RootInstance.Value.IsValid())
		{
			if (UNarrativeComponent* NarrativeComponent = Cast<UNarrativeComponent>(RootInstance.Value))
			{
				NarrativeComponent->SaveRootNarrative(SaveGame->NarrativeInstances);
			}
			else
			{
				RootInstance.Key->SaveInstance(SaveGame->NarrativeInstances);
			}
		}
	}

	// save Narrative Components
	{
		// retrieve all registered components
		TArray<TWeakObjectPtr<UNarrativeComponent>> ComponentsArray;
		NarrativeComponentRegistry.GenerateValueArray(ComponentsArray);

		// ensure uniqueness of entries
		const TSet<TWeakObjectPtr<UNarrativeComponent>> RegisteredComponents = TSet<TWeakObjectPtr<UNarrativeComponent>>(ComponentsArray);

		// write archives to SaveGame
		for (const TWeakObjectPtr<UNarrativeComponent> RegisteredComponent : RegisteredComponents)
		{
			SaveGame->NarrativeComponents.Emplace(RegisteredComponent->SaveInstance());
		}
	}
}

void UNarrativeSubsystem::OnGameLoaded(UNarrativeSaveGame* SaveGame)
{
	LoadedSaveGame = SaveGame;

	// here's opportunity to apply loaded data to custom systems
	// it's recommended to do this by overriding method in the subclass
}

void UNarrativeSubsystem::LoadRootNarrative(UObject* Owner, UNarrativeAsset* NarrativeAsset, const FString& SavedAssetInstanceName)
{
	if (NarrativeAsset == nullptr || SavedAssetInstanceName.IsEmpty())
	{
		return;
	}

	for (const FNarrativeAssetSaveData& AssetRecord : LoadedSaveGame->NarrativeInstances)
	{
		if (AssetRecord.InstanceName == SavedAssetInstanceName
			&& (NarrativeAsset->IsBoundToWorld() == false || AssetRecord.WorldName == GetWorld()->GetName()))
		{
			UNarrativeAsset* LoadedInstance = CreateRootNarrative(Owner, NarrativeAsset, false);
			if (LoadedInstance)
			{
				LoadedInstance->LoadInstance(AssetRecord);
			}
			return;
		}
	}
}

void UNarrativeSubsystem::LoadSubNarrative(UNarrativeNode_SubGraph* SubGraphNode, const FString& SavedAssetInstanceName)
{
	if (SubGraphNode->Asset.IsNull())
	{
		return;
	}

	UNarrativeAsset* SubGraphAsset = SubGraphNode->Asset.LoadSynchronous();

	for (const FNarrativeAssetSaveData& AssetRecord : LoadedSaveGame->NarrativeInstances)
	{
		if (AssetRecord.InstanceName == SavedAssetInstanceName
			&& ((SubGraphAsset && SubGraphAsset->IsBoundToWorld() == false) || AssetRecord.WorldName == GetWorld()->GetName()))
		{
			UNarrativeAsset* LoadedInstance = CreateSubNarrative(SubGraphNode, SavedAssetInstanceName);
			if (LoadedInstance)
			{
				LoadedInstance->LoadInstance(AssetRecord);
			}
			return;
		}
	}
}

void UNarrativeSubsystem::RegisterComponent(UNarrativeComponent* Component)
{
	for (const FGameplayTag& Tag : Component->IdentityTags)
	{
		if (Tag.IsValid())
		{
			NarrativeComponentRegistry.Emplace(Tag, Component);
		}
	}

	OnComponentRegistered.Broadcast(Component);
}

void UNarrativeSubsystem::OnIdentityTagAdded(UNarrativeComponent* Component, const FGameplayTag& AddedTag)
{
	NarrativeComponentRegistry.Emplace(AddedTag, Component);

	// broadcast OnComponentRegistered only if this component wasn't present in the registry previously
	if (Component->IdentityTags.Num() > 1)
	{
		OnComponentTagAdded.Broadcast(Component, FGameplayTagContainer(AddedTag));
	}
	else
	{
		OnComponentRegistered.Broadcast(Component);
	}
}

void UNarrativeSubsystem::OnIdentityTagsAdded(UNarrativeComponent* Component, const FGameplayTagContainer& AddedTags)
{
	for (const FGameplayTag& Tag : AddedTags)
	{
		NarrativeComponentRegistry.Emplace(Tag, Component);
	}

	// broadcast OnComponentRegistered only if this component wasn't present in the registry previously
	if (Component->IdentityTags.Num() > AddedTags.Num())
	{
		OnComponentTagAdded.Broadcast(Component, AddedTags);
	}
	else
	{
		OnComponentRegistered.Broadcast(Component);
	}
}

void UNarrativeSubsystem::UnregisterComponent(UNarrativeComponent* Component)
{
	for (const FGameplayTag& Tag : Component->IdentityTags)
	{
		if (Tag.IsValid())
		{
			NarrativeComponentRegistry.Remove(Tag, Component);
		}
	}

	OnComponentUnregistered.Broadcast(Component);
}

void UNarrativeSubsystem::OnIdentityTagRemoved(UNarrativeComponent* Component, const FGameplayTag& RemovedTag)
{
	NarrativeComponentRegistry.Remove(RemovedTag, Component);

	// broadcast OnComponentUnregistered only if this component isn't present in the registry anymore
	if (Component->IdentityTags.Num() > 0)
	{
		OnComponentTagRemoved.Broadcast(Component, FGameplayTagContainer(RemovedTag));
	}
	else
	{
		OnComponentUnregistered.Broadcast(Component);
	}
}

void UNarrativeSubsystem::OnIdentityTagsRemoved(UNarrativeComponent* Component, const FGameplayTagContainer& RemovedTags)
{
	for (const FGameplayTag& Tag : RemovedTags)
	{
		NarrativeComponentRegistry.Remove(Tag, Component);
	}

	// broadcast OnComponentUnregistered only if this component isn't present in the registry anymore
	if (Component->IdentityTags.Num() > 0)
	{
		OnComponentTagRemoved.Broadcast(Component, RemovedTags);
	}
	else
	{
		OnComponentUnregistered.Broadcast(Component);
	}
}

TSet<UNarrativeComponent*> UNarrativeSubsystem::GetNarrativeComponentsByTag(const FGameplayTag Tag, const TSubclassOf<UNarrativeComponent> ComponentClass, const bool bExactMatch) const
{
	TArray<TWeakObjectPtr<UNarrativeComponent>> FoundComponents;
	FindComponents(Tag, bExactMatch, FoundComponents);

	TSet<UNarrativeComponent*> Result;
	for (const TWeakObjectPtr<UNarrativeComponent>& Component : FoundComponents)
	{
		if (Component.IsValid() && Component->GetClass()->IsChildOf(ComponentClass))
		{
			Result.Emplace(Component.Get());
		}
	}

	return Result;
}

TSet<UNarrativeComponent*> UNarrativeSubsystem::GetNarrativeComponentsByTags(const FGameplayTagContainer Tags, const EGameplayContainerMatchType MatchType, const TSubclassOf<UNarrativeComponent> ComponentClass, const bool bExactMatch) const
{
	TSet<TWeakObjectPtr<UNarrativeComponent>> FoundComponents;
	FindComponents(Tags, MatchType, bExactMatch, FoundComponents);

	TSet<UNarrativeComponent*> Result;
	for (const TWeakObjectPtr<UNarrativeComponent>& Component : FoundComponents)
	{
		if (Component.IsValid() && Component->GetClass()->IsChildOf(ComponentClass))
		{
			Result.Emplace(Component.Get());
		}
	}

	return Result;
}

TSet<AActor*> UNarrativeSubsystem::GetNarrativeActorsByTag(const FGameplayTag Tag, const TSubclassOf<AActor> ActorClass, const bool bExactMatch) const
{
	TArray<TWeakObjectPtr<UNarrativeComponent>> FoundComponents;
	FindComponents(Tag, bExactMatch, FoundComponents);

	TSet<AActor*> Result;
	for (const TWeakObjectPtr<UNarrativeComponent>& Component : FoundComponents)
	{
		if (Component.IsValid() && Component->GetOwner()->GetClass()->IsChildOf(ActorClass))
		{
			Result.Emplace(Component->GetOwner());
		}
	}

	return Result;
}

TSet<AActor*> UNarrativeSubsystem::GetNarrativeActorsByTags(const FGameplayTagContainer Tags, const EGameplayContainerMatchType MatchType, const TSubclassOf<AActor> ActorClass, const bool bExactMatch) const
{
	TSet<TWeakObjectPtr<UNarrativeComponent>> FoundComponents;
	FindComponents(Tags, MatchType, bExactMatch, FoundComponents);

	TSet<AActor*> Result;
	for (const TWeakObjectPtr<UNarrativeComponent>& Component : FoundComponents)
	{
		if (Component.IsValid() && Component->GetOwner()->GetClass()->IsChildOf(ActorClass))
		{
			Result.Emplace(Component->GetOwner());
		}
	}

	return Result;
}

TMap<AActor*, UNarrativeComponent*> UNarrativeSubsystem::GetNarrativeActorsAndComponentsByTag(const FGameplayTag Tag, const TSubclassOf<AActor> ActorClass, const bool bExactMatch) const
{
	TArray<TWeakObjectPtr<UNarrativeComponent>> FoundComponents;
	FindComponents(Tag, bExactMatch, FoundComponents);

	TMap<AActor*, UNarrativeComponent*> Result;
	for (const TWeakObjectPtr<UNarrativeComponent>& Component : FoundComponents)
	{
		if (Component.IsValid() && Component->GetOwner()->GetClass()->IsChildOf(ActorClass))
		{
			Result.Emplace(Component->GetOwner(), Component.Get());
		}
	}

	return Result;
}

TMap<AActor*, UNarrativeComponent*> UNarrativeSubsystem::GetNarrativeActorsAndComponentsByTags(const FGameplayTagContainer Tags, const EGameplayContainerMatchType MatchType, const TSubclassOf<AActor> ActorClass, const bool bExactMatch) const
{
	TSet<TWeakObjectPtr<UNarrativeComponent>> FoundComponents;
	FindComponents(Tags, MatchType, bExactMatch, FoundComponents);

	TMap<AActor*, UNarrativeComponent*> Result;
	for (const TWeakObjectPtr<UNarrativeComponent>& Component : FoundComponents)
	{
		if (Component.IsValid() && Component->GetOwner()->GetClass()->IsChildOf(ActorClass))
		{
			Result.Emplace(Component->GetOwner(), Component.Get());
		}
	}

	return Result;
}

void UNarrativeSubsystem::FindComponents(const FGameplayTag& Tag, const bool bExactMatch, TArray<TWeakObjectPtr<UNarrativeComponent>>& OutComponents) const
{
	if (bExactMatch)
	{
		NarrativeComponentRegistry.MultiFind(Tag, OutComponents);
	}
	else
	{
		for (TMultiMap<FGameplayTag, TWeakObjectPtr<UNarrativeComponent>>::TConstIterator It(NarrativeComponentRegistry); It; ++It)
		{
			if (It.Key().MatchesTag(Tag))
			{
				OutComponents.Emplace(It.Value());
			}
		}
	}
}

void UNarrativeSubsystem::FindComponents(const FGameplayTagContainer& Tags, const EGameplayContainerMatchType MatchType, const bool bExactMatch, TSet<TWeakObjectPtr<UNarrativeComponent>>& OutComponents) const
{
	if (MatchType == EGameplayContainerMatchType::Any)
	{
		for (const FGameplayTag& Tag : Tags)
		{
			TArray<TWeakObjectPtr<UNarrativeComponent>> ComponentsPerTag;
			FindComponents(Tag, bExactMatch, ComponentsPerTag);
			OutComponents.Append(ComponentsPerTag);
		}
	}
	else // EGameplayContainerMatchType::All
	{
		TSet<TWeakObjectPtr<UNarrativeComponent>> ComponentsWithAnyTag;
		for (const FGameplayTag& Tag : Tags)
		{
			TArray<TWeakObjectPtr<UNarrativeComponent>> ComponentsPerTag;
			FindComponents(Tag, bExactMatch, ComponentsPerTag);
			ComponentsWithAnyTag.Append(ComponentsPerTag);
		}

		for (const TWeakObjectPtr<UNarrativeComponent>& Component : ComponentsWithAnyTag)
		{
			if (Component.IsValid() && Component->IdentityTags.HasAllExact(Tags))
			{
				OutComponents.Emplace(Component);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
