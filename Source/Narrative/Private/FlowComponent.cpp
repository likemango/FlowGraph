// Copyright XiaoYao

#include "NarrativeComponent.h"

#include "NarrativeAsset.h"
#include "NarrativeLogChannels.h"
#include "NarrativeSettings.h"
#include "NarrativeSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/ViewportStatsSubsystem.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativeComponent)

UNarrativeComponent::UNarrativeComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, RootNarrative(nullptr)
	, bAutoStartRootNarrative(true)
	, RootNarrativeMode(ENarrativeNetMode::Authority)
	, bAllowMultipleInstances(true)
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	SetIsReplicatedByDefault(true);
}

void UNarrativeComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UNarrativeComponent, AddedIdentityTags);
	DOREPLIFETIME(UNarrativeComponent, RemovedIdentityTags);

	DOREPLIFETIME(UNarrativeComponent, RecentlySentNotifyTags);
	DOREPLIFETIME(UNarrativeComponent, NotifyTagsFromGraph);
	DOREPLIFETIME(UNarrativeComponent, NotifyTagsFromAnotherComponent);
}

void UNarrativeComponent::BeginPlay()
{
	Super::BeginPlay();

	RegisterWithNarrativeSubsystem();
}

void UNarrativeComponent::RegisterWithNarrativeSubsystem()
{
	if (UNarrativeSubsystem* NarrativeSubsystem = GetNarrativeSubsystem())
	{
		bool bComponentLoadedFromSaveGame = false;
		if (GetNarrativeSubsystem()->GetLoadedSaveGame())
		{
			bComponentLoadedFromSaveGame = LoadInstance();
		}

		NarrativeSubsystem->RegisterComponent(this);

		if (RootNarrative)
		{
			if (bComponentLoadedFromSaveGame)
			{
				LoadRootNarrative();
			}
			else if (bAutoStartRootNarrative)
			{
				StartRootNarrative();
			}
		}
	}
}

void UNarrativeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterWithNarrativeSubsystem();

	Super::EndPlay(EndPlayReason);
}

void UNarrativeComponent::UnregisterWithNarrativeSubsystem()
{
	if (UNarrativeSubsystem* NarrativeSubsystem = GetNarrativeSubsystem())
	{
		NarrativeSubsystem->FinishAllRootNarratives(this, ENarrativeFinishPolicy::Keep);
		NarrativeSubsystem->UnregisterComponent(this);
	}
}

void UNarrativeComponent::AddIdentityTag(const FGameplayTag Tag, const ENarrativeNetMode NetMode /* = ENarrativeNetMode::Authority*/)
{
	if (IsNarrativeNetMode(NetMode) && Tag.IsValid() && !IdentityTags.HasTagExact(Tag))
	{
		IdentityTags.AddTag(Tag);

		if (HasBegunPlay())
		{
			OnIdentityTagsAdded.Broadcast(this, FGameplayTagContainer(Tag));

			if (UNarrativeSubsystem* NarrativeSubsystem = GetNarrativeSubsystem())
			{
				NarrativeSubsystem->OnIdentityTagAdded(this, Tag);
			}

			if (IsNetMode(NM_DedicatedServer) || IsNetMode(NM_ListenServer))
			{
				AddedIdentityTags = FGameplayTagContainer(Tag);
			}
		}
	}
}

void UNarrativeComponent::AddIdentityTags(FGameplayTagContainer Tags, const ENarrativeNetMode NetMode /* = ENarrativeNetMode::Authority*/)
{
	if (IsNarrativeNetMode(NetMode) && Tags.IsValid())
	{
		FGameplayTagContainer ValidatedTags;

		for (const FGameplayTag& Tag : Tags)
		{
			if (Tag.IsValid() && !IdentityTags.HasTagExact(Tag))
			{
				IdentityTags.AddTag(Tag);
				ValidatedTags.AddTag(Tag);
			}
		}

		if (ValidatedTags.Num() > 0 && HasBegunPlay())
		{
			OnIdentityTagsAdded.Broadcast(this, ValidatedTags);

			if (UNarrativeSubsystem* NarrativeSubsystem = GetNarrativeSubsystem())
			{
				NarrativeSubsystem->OnIdentityTagsAdded(this, ValidatedTags);
			}

			if (IsNetMode(NM_DedicatedServer) || IsNetMode(NM_ListenServer))
			{
				AddedIdentityTags = ValidatedTags;
			}
		}
	}
}

void UNarrativeComponent::RemoveIdentityTag(const FGameplayTag Tag, const ENarrativeNetMode NetMode /* = ENarrativeNetMode::Authority*/)
{
	if (IsNarrativeNetMode(NetMode) && Tag.IsValid() && IdentityTags.HasTagExact(Tag))
	{
		IdentityTags.RemoveTag(Tag);

		if (HasBegunPlay())
		{
			OnIdentityTagsRemoved.Broadcast(this, FGameplayTagContainer(Tag));

			if (UNarrativeSubsystem* NarrativeSubsystem = GetNarrativeSubsystem())
			{
				NarrativeSubsystem->OnIdentityTagRemoved(this, Tag);
			}

			if (IsNetMode(NM_DedicatedServer) || IsNetMode(NM_ListenServer))
			{
				RemovedIdentityTags = FGameplayTagContainer(Tag);
			}
		}
	}
}

void UNarrativeComponent::RemoveIdentityTags(FGameplayTagContainer Tags, const ENarrativeNetMode NetMode /* = ENarrativeNetMode::Authority*/)
{
	if (IsNarrativeNetMode(NetMode) && Tags.IsValid())
	{
		FGameplayTagContainer ValidatedTags;

		for (const FGameplayTag& Tag : Tags)
		{
			if (Tag.IsValid() && IdentityTags.HasTagExact(Tag))
			{
				IdentityTags.RemoveTag(Tag);
				ValidatedTags.AddTag(Tag);
			}
		}

		if (ValidatedTags.Num() > 0 && HasBegunPlay())
		{
			OnIdentityTagsRemoved.Broadcast(this, ValidatedTags);

			if (UNarrativeSubsystem* NarrativeSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UNarrativeSubsystem>())
			{
				NarrativeSubsystem->OnIdentityTagsRemoved(this, ValidatedTags);
			}

			if (IsNetMode(NM_DedicatedServer) || IsNetMode(NM_ListenServer))
			{
				RemovedIdentityTags = ValidatedTags;
			}
		}
	}
}

void UNarrativeComponent::OnRep_AddedIdentityTags()
{
	IdentityTags.AppendTags(AddedIdentityTags);
	OnIdentityTagsAdded.Broadcast(this, AddedIdentityTags);

	if (UNarrativeSubsystem* NarrativeSubsystem = GetNarrativeSubsystem())
	{
		NarrativeSubsystem->OnIdentityTagsAdded(this, AddedIdentityTags);
	}
}

void UNarrativeComponent::OnRep_RemovedIdentityTags()
{
	IdentityTags.RemoveTags(RemovedIdentityTags);
	OnIdentityTagsRemoved.Broadcast(this, RemovedIdentityTags);

	if (UNarrativeSubsystem* NarrativeSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UNarrativeSubsystem>())
	{
		NarrativeSubsystem->OnIdentityTagsRemoved(this, RemovedIdentityTags);
	}
}

void UNarrativeComponent::VerifyIdentityTags() const
{
	if (IdentityTags.IsEmpty() && UNarrativeSettings::Get()->bWarnAboutMissingIdentityTags)
	{
		FString Message = TEXT("Missing Identity Tags on the Narrative Component creating Narrative Asset instance! This gonna break loading SaveGame for this component!");
		Message.Append(LINE_TERMINATOR).Append(TEXT("If you're not using SaveSystem, you can silence this warning by unchecking bWarnAboutMissingIdentityTags flag in Narrative Settings."));
		LogError(Message);
	}
}

void UNarrativeComponent::LogError(FString Message, const ENarrativeOnScreenMessageType OnScreenMessageType) const
{
	Message += TEXT(" --- Narrative Component in actor ") + GetOwner()->GetName();

	if (OnScreenMessageType == ENarrativeOnScreenMessageType::Permanent)
	{
		if (GetWorld())
		{
			if (UViewportStatsSubsystem* StatsSubsystem = GetWorld()->GetSubsystem<UViewportStatsSubsystem>())
			{
				StatsSubsystem->AddDisplayDelegate([this, Message](FText& OutText, FLinearColor& OutColor)
				{
					OutText = FText::FromString(Message);
					OutColor = FLinearColor::Red;
					return IsValid(this);
				});
			}
		}
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, Message);
	}

	UE_LOG(LogNarrative, Error, TEXT("%s"), *Message);
}

void UNarrativeComponent::NotifyGraph(const FGameplayTag NotifyTag, const ENarrativeNetMode NetMode /* = ENarrativeNetMode::Authority*/)
{
	if (IsNarrativeNetMode(NetMode) && NotifyTag.IsValid() && HasBegunPlay())
	{
		// save recently notify, this allow for the retroactive check in nodes
		// if retroactive check wouldn't be performed, this is only used by the network replication
		RecentlySentNotifyTags = FGameplayTagContainer(NotifyTag);

		OnRep_SentNotifyTags();
	}
}

void UNarrativeComponent::BulkNotifyGraph(const FGameplayTagContainer NotifyTags, const ENarrativeNetMode NetMode /* = ENarrativeNetMode::Authority*/)
{
	if (IsNarrativeNetMode(NetMode) && NotifyTags.IsValid() && HasBegunPlay())
	{
		FGameplayTagContainer ValidatedTags;
		for (const FGameplayTag& Tag : NotifyTags)
		{
			if (Tag.IsValid())
			{
				ValidatedTags.AddTag(Tag);
			}
		}

		if (ValidatedTags.Num() > 0)
		{
			// save recently notify, this allow for the retroactive check in nodes
			// if retroactive check wouldn't be performed, this is only used by the network replication
			RecentlySentNotifyTags = ValidatedTags;

			OnRep_SentNotifyTags();
		}
	}
}

void UNarrativeComponent::OnRep_SentNotifyTags()
{
	for (const FGameplayTag& NotifyTag : RecentlySentNotifyTags)
	{
		OnNotifyFromComponent.Broadcast(this, NotifyTag);
	}
}

void UNarrativeComponent::NotifyFromGraph(const FGameplayTagContainer& NotifyTags, const ENarrativeNetMode NetMode /* = ENarrativeNetMode::Authority*/)
{
	if (IsNarrativeNetMode(NetMode) && NotifyTags.IsValid() && HasBegunPlay())
	{
		FGameplayTagContainer ValidatedTags;
		for (const FGameplayTag& Tag : NotifyTags)
		{
			if (Tag.IsValid())
			{
				ValidatedTags.AddTag(Tag);
			}
		}

		if (ValidatedTags.Num() > 0)
		{
			for (const FGameplayTag& ValidatedTag : ValidatedTags)
			{
				ReceiveNotify.Broadcast(nullptr, ValidatedTag);
			}

			if (IsNetMode(NM_DedicatedServer) || IsNetMode(NM_ListenServer))
			{
				NotifyTagsFromGraph = ValidatedTags;
			}
		}
	}
}

void UNarrativeComponent::OnRep_NotifyTagsFromGraph()
{
	for (const FGameplayTag& NotifyTag : NotifyTagsFromGraph)
	{
		ReceiveNotify.Broadcast(nullptr, NotifyTag);
	}
}

void UNarrativeComponent::NotifyActor(const FGameplayTag ActorTag, const FGameplayTag NotifyTag, const ENarrativeNetMode NetMode /* = ENarrativeNetMode::Authority*/)
{
	if (IsNarrativeNetMode(NetMode) && NotifyTag.IsValid() && HasBegunPlay())
	{
		if (const UNarrativeSubsystem* NarrativeSubsystem = GetNarrativeSubsystem())
		{
			for (const TWeakObjectPtr<UNarrativeComponent>& Component : NarrativeSubsystem->GetComponents<UNarrativeComponent>(ActorTag))
			{
				Component->ReceiveNotify.Broadcast(this, NotifyTag);
			}
		}

		if (IsNetMode(NM_DedicatedServer) || IsNetMode(NM_ListenServer))
		{
			NotifyTagsFromAnotherComponent.Empty();
			NotifyTagsFromAnotherComponent.Add(FNotifyTagReplication(ActorTag, NotifyTag));
		}
	}
}

void UNarrativeComponent::OnRep_NotifyTagsFromAnotherComponent()
{
	if (const UNarrativeSubsystem* NarrativeSubsystem = GetNarrativeSubsystem())
	{
		for (const FNotifyTagReplication& Notify : NotifyTagsFromAnotherComponent)
		{
			for (const TWeakObjectPtr<UNarrativeComponent>& Component : NarrativeSubsystem->GetComponents<UNarrativeComponent>(Notify.ActorTag))
			{
				Component->ReceiveNotify.Broadcast(this, Notify.NotifyTag);
			}
		}
	}
}

void UNarrativeComponent::StartRootNarrative()
{
	if (RootNarrative && IsNarrativeNetMode(RootNarrativeMode))
	{
		if (UNarrativeSubsystem* NarrativeSubsystem = GetNarrativeSubsystem())
		{
			VerifyIdentityTags();

			NarrativeSubsystem->StartRootNarrative(this, RootNarrative, bAllowMultipleInstances);
		}
	}
}

void UNarrativeComponent::FinishRootNarrative(UNarrativeAsset* TemplateAsset, const ENarrativeFinishPolicy FinishPolicy)
{
	if (UNarrativeSubsystem* NarrativeSubsystem = GetNarrativeSubsystem())
	{
		NarrativeSubsystem->FinishRootNarrative(this, TemplateAsset, FinishPolicy);
	}
}

TSet<UNarrativeAsset*> UNarrativeComponent::GetRootInstances(const UObject* Owner) const
{
	const UObject* OwnerToCheck = IsValid(Owner) ? Owner : this;

	if (const UNarrativeSubsystem* NarrativeSubsystem = GetNarrativeSubsystem())
	{
		return NarrativeSubsystem->GetRootInstancesByOwner(OwnerToCheck);
	}

	return TSet<UNarrativeAsset*>();
}

UNarrativeAsset* UNarrativeComponent::GetRootNarrativeInstance() const
{
	if (const UNarrativeSubsystem* NarrativeSubsystem = GetNarrativeSubsystem())
	{
		const TSet<UNarrativeAsset*> Result = NarrativeSubsystem->GetRootInstancesByOwner(this);
		if (Result.Num() > 0)
		{
			return Result.Array()[0];
		}
	}

	return nullptr;
}

void UNarrativeComponent::OnTriggerRootNarrativeOutputEventDispatcher(UNarrativeAsset* RootNarrativeInstance, const FName& EventName)
{
	BP_OnTriggerRootNarrativeOutputEvent(RootNarrativeInstance, EventName);
	OnTriggerRootNarrativeOutputEvent(RootNarrativeInstance, EventName);
}

void UNarrativeComponent::SaveRootNarrative(TArray<FNarrativeAssetSaveData>& SavedNarrativeInstances)
{
	if (UNarrativeAsset* NarrativeAssetInstance = GetRootNarrativeInstance())
	{
		const FNarrativeAssetSaveData AssetRecord = NarrativeAssetInstance->SaveInstance(SavedNarrativeInstances);
		SavedAssetInstanceName = AssetRecord.InstanceName;
		return;
	}

	SavedAssetInstanceName = FString();
}

void UNarrativeComponent::LoadRootNarrative()
{
	if (RootNarrative && !SavedAssetInstanceName.IsEmpty() && GetNarrativeSubsystem())
	{
		VerifyIdentityTags();

		GetNarrativeSubsystem()->LoadRootNarrative(this, RootNarrative, SavedAssetInstanceName);
		SavedAssetInstanceName = FString();
	}
}

FNarrativeComponentSaveData UNarrativeComponent::SaveInstance()
{
	FNarrativeComponentSaveData ComponentRecord;
	ComponentRecord.WorldName = GetWorld()->GetName();
	ComponentRecord.ActorInstanceName = GetOwner()->GetName();

	// opportunity to collect data before serializing component
	OnSave();

	// serialize component
	FMemoryWriter MemoryWriter(ComponentRecord.ComponentData, true);
	FNarrativeArchive Ar(MemoryWriter);
	Serialize(Ar);

	return ComponentRecord;
}

bool UNarrativeComponent::LoadInstance()
{
	const UNarrativeSaveGame* SaveGame = GetNarrativeSubsystem()->GetLoadedSaveGame();
	if (SaveGame->NarrativeComponents.Num() > 0)
	{
		for (const FNarrativeComponentSaveData& ComponentRecord : SaveGame->NarrativeComponents)
		{
			if (ComponentRecord.WorldName == GetWorld()->GetName() && ComponentRecord.ActorInstanceName == GetOwner()->GetName())
			{
				FMemoryReader MemoryReader(ComponentRecord.ComponentData, true);
				FNarrativeArchive Ar(MemoryReader);
				Serialize(Ar);

				OnLoad();
				return true;
			}
		}
	}

	return false;
}

void UNarrativeComponent::OnSave_Implementation()
{
}

void UNarrativeComponent::OnLoad_Implementation()
{
}

UNarrativeSubsystem* UNarrativeComponent::GetNarrativeSubsystem() const
{
	if (GetWorld() && GetWorld()->GetGameInstance())
	{
		return GetWorld()->GetGameInstance()->GetSubsystem<UNarrativeSubsystem>();
	}

	return nullptr;
}

bool UNarrativeComponent::IsNarrativeNetMode(const ENarrativeNetMode NetMode) const
{
	switch (NetMode)
	{
		case ENarrativeNetMode::Any:
			return true;
		case ENarrativeNetMode::Authority:
			return GetOwner()->HasAuthority();
		case ENarrativeNetMode::ClientOnly:
			return IsNetMode(NM_Client) && UNarrativeSettings::Get()->bCreateNarrativeSubsystemOnClients;
		case ENarrativeNetMode::ServerOnly:
			return IsNetMode(NM_DedicatedServer) || IsNetMode(NM_ListenServer);
		case ENarrativeNetMode::SinglePlayerOnly:
			return IsNetMode(NM_Standalone);
		default:
			return false;
	}
}
