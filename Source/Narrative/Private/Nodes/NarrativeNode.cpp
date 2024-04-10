// Copyright XiaoYao

#include "Nodes/NarrativeNode.h"

#include "NarrativeAsset.h"
#include "NarrativeLogChannels.h"
#include "NarrativeOwnerInterface.h"
#include "NarrativeSettings.h"
#include "NarrativeSubsystem.h"
#include "NarrativeTypes.h"

#include "Components/ActorComponent.h"
#include "Engine/Blueprint.h"
#include "Engine/Engine.h"
#include "Engine/ViewportStatsSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativeNode)

FNarrativePin UNarrativeNode::DefaultInputPin(TEXT("In"));
FNarrativePin UNarrativeNode::DefaultOutputPin(TEXT("Out"));

FString UNarrativeNode::MissingIdentityTag = TEXT("Missing Identity Tag");
FString UNarrativeNode::MissingNotifyTag = TEXT("Missing Notify Tag");
FString UNarrativeNode::MissingClass = TEXT("Missing class");
FString UNarrativeNode::NoActorsFound = TEXT("No actors found");

UNarrativeNode::UNarrativeNode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, GraphNode(nullptr)
#if WITH_EDITOR
	, bCanDelete(true)
	, bCanDuplicate(true)
	, bNodeDeprecated(false)
#endif
	, AllowedSignalModes({ENarrativeSignalMode::Enabled, ENarrativeSignalMode::Disabled, ENarrativeSignalMode::PassThrough})
	, SignalMode(ENarrativeSignalMode::Enabled)
	, bPreloaded(false)
	, ActivationState(ENarrativeNodeState::NeverActivated)
{
#if WITH_EDITOR
	Category = TEXT("Uncategorized");
	NodeStyle = ENarrativeNodeStyle::Default;
	NodeColor = FLinearColor::Black;
#endif

	InputPins = {DefaultInputPin};
	OutputPins = {DefaultOutputPin};
}

#if WITH_EDITOR
void UNarrativeNode::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property
		&& (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UNarrativeNode, InputPins) || PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UNarrativeNode, OutputPins)
			|| PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(UNarrativeNode, InputPins) || PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(UNarrativeNode, OutputPins)))
	{
		OnReconstructionRequested.ExecuteIfBound();
	}
}

void UNarrativeNode::PostLoad()
{
	Super::PostLoad();

	// fix Class Default Object
	FixNode(nullptr);
}

void UNarrativeNode::FixNode(UEdGraphNode* NewGraphNode)
{
	// Fix any node pointers that may be out of date
	if (NewGraphNode)
	{
		GraphNode = NewGraphNode;
	}
}

void UNarrativeNode::SetGraphNode(UEdGraphNode* NewGraph)
{
	GraphNode = NewGraph;
}

FString UNarrativeNode::GetNodeCategory() const
{
	if (GetClass()->ClassGeneratedBy)
	{
		const FString& BlueprintCategory = Cast<UBlueprint>(GetClass()->ClassGeneratedBy)->BlueprintCategory;
		if (!BlueprintCategory.IsEmpty())
		{
			return BlueprintCategory;
		}
	}

	return Category;
}

FText UNarrativeNode::GetNodeTitle() const
{
	if (GetClass()->ClassGeneratedBy)
	{
		const FString& BlueprintTitle = Cast<UBlueprint>(GetClass()->ClassGeneratedBy)->BlueprintDisplayName;
		if (!BlueprintTitle.IsEmpty())
		{
			return FText::FromString(BlueprintTitle);
		}
	}

	return GetClass()->GetDisplayNameText();
}

FText UNarrativeNode::GetNodeToolTip() const
{
	if (GetClass()->ClassGeneratedBy)
	{
		const FString& BlueprintToolTip = Cast<UBlueprint>(GetClass()->ClassGeneratedBy)->BlueprintDescription;
		if (!BlueprintToolTip.IsEmpty())
		{
			return FText::FromString(BlueprintToolTip);
		}
	}

	return GetClass()->GetToolTipText();
}

bool UNarrativeNode::GetDynamicTitleColor(FLinearColor& OutColor) const
{
	if (NodeStyle == ENarrativeNodeStyle::Custom)
	{
		OutColor = NodeColor;
		return true;
	}

	return false;
}

FString UNarrativeNode::GetNodeDescription() const
{
	return K2_GetNodeDescription();
}
#endif

UNarrativeAsset* UNarrativeNode::GetNarrativeAsset() const
{
	return GetOuter() ? Cast<UNarrativeAsset>(GetOuter()) : nullptr;
}

AActor* UNarrativeNode::TryGetRootNarrativeActorOwner() const
{
	AActor* OwningActor = nullptr;

	UObject* RootNarrativeOwner = TryGetRootNarrativeObjectOwner();

	if (IsValid(RootNarrativeOwner))
	{
		// Check if the immediate parent is an AActor
		OwningActor = Cast<AActor>(RootNarrativeOwner);

		if (!IsValid(OwningActor))
		{
			// Check if the if the immediate parent is an UActorComponent
			//  and return that Component's Owning actor
			if (const UActorComponent* OwningComponent = Cast<UActorComponent>(RootNarrativeOwner))
			{
				OwningActor = OwningComponent->GetOwner();
			}
		}
	}

	return OwningActor;
}

UObject* UNarrativeNode::TryGetRootNarrativeObjectOwner() const
{
	const UNarrativeAsset* NarrativeAsset = GetNarrativeAsset();

	if (IsValid(NarrativeAsset))
	{
		return NarrativeAsset->GetOwner();
	}

	return nullptr;
}

INarrativeOwnerInterface* UNarrativeNode::GetNarrativeOwnerInterface() const
{
	const UNarrativeAsset* NarrativeAsset = GetNarrativeAsset();
	if (!IsValid(NarrativeAsset))
	{
		return nullptr;
	}

	const UClass* ExpectedOwnerClass = NarrativeAsset->GetExpectedOwnerClass();
	if (!IsValid(ExpectedOwnerClass))
	{
		return nullptr;
	}

	UObject* RootNarrativeOwner = NarrativeAsset->GetOwner();
	if (!IsValid(RootNarrativeOwner))
	{
		return nullptr;
	}

	if (INarrativeOwnerInterface* NarrativeOwnerInterface = TryGetNarrativeOwnerInterfaceFromRootNarrativeOwner(*RootNarrativeOwner, *ExpectedOwnerClass))
	{
		return NarrativeOwnerInterface;
	}

	if (INarrativeOwnerInterface* NarrativeOwnerInterface = TryGetNarrativeOwnerInterfaceActor(*RootNarrativeOwner, *ExpectedOwnerClass))
	{
		return NarrativeOwnerInterface;
	}

	return nullptr;
}

INarrativeOwnerInterface* UNarrativeNode::TryGetNarrativeOwnerInterfaceFromRootNarrativeOwner(UObject& RootNarrativeOwner, const UClass& ExpectedOwnerClass) const
{
	const UClass* RootNarrativeOwnerClass = RootNarrativeOwner.GetClass();
	if (!IsValid(RootNarrativeOwnerClass))
	{
		return nullptr;
	}

	if (!RootNarrativeOwnerClass->IsChildOf(&ExpectedOwnerClass))
	{
		return nullptr;
	}

	// If the immediate owner is the expected class type, return its NarrativeOwnerInterface
	return CastChecked<INarrativeOwnerInterface>(&RootNarrativeOwner);
}

INarrativeOwnerInterface* UNarrativeNode::TryGetNarrativeOwnerInterfaceActor(UObject& RootNarrativeOwner, const UClass& ExpectedOwnerClass) const
{
	// Special case if the immediate owner is a component, also consider the component's owning actor
	const UActorComponent* NarrativeComponent = Cast<UActorComponent>(&RootNarrativeOwner);
	if (!IsValid(NarrativeComponent))
	{
		return nullptr;
	}

	AActor* ActorOwner = NarrativeComponent->GetOwner();
	if (!IsValid(ActorOwner))
	{
		return nullptr;
	}

	const UClass* ActorOwnerClass = ActorOwner->GetClass();
	if (!ActorOwnerClass->IsChildOf(&ExpectedOwnerClass))
	{
		return nullptr;
	}

	return CastChecked<INarrativeOwnerInterface>(ActorOwner);
}

void UNarrativeNode::AddInputPins(TArray<FNarrativePin> Pins)
{
	for (const FNarrativePin& Pin : Pins)
	{
		InputPins.Emplace(Pin);
	}
}

void UNarrativeNode::AddOutputPins(TArray<FNarrativePin> Pins)
{
	for (const FNarrativePin& Pin : Pins)
	{
		OutputPins.Emplace(Pin);
	}
}

void UNarrativeNode::SetNumberedInputPins(const uint8 FirstNumber, const uint8 LastNumber)
{
	InputPins.Empty();

	for (uint8 i = FirstNumber; i <= LastNumber; i++)
	{
		InputPins.Emplace(i);
	}
}

void UNarrativeNode::SetNumberedOutputPins(const uint8 FirstNumber /*= 0*/, const uint8 LastNumber /*= 1*/)
{
	OutputPins.Empty();

	for (uint8 i = FirstNumber; i <= LastNumber; i++)
	{
		OutputPins.Emplace(i);
	}
}

uint8 UNarrativeNode::CountNumberedInputs() const
{
	uint8 Result = 0;
	for (const FNarrativePin& Pin : InputPins)
	{
		if (Pin.PinName.ToString().IsNumeric())
		{
			Result++;
		}
	}
	return Result;
}

uint8 UNarrativeNode::CountNumberedOutputs() const
{
	uint8 Result = 0;
	for (const FNarrativePin& Pin : OutputPins)
	{
		if (Pin.PinName.ToString().IsNumeric())
		{
			Result++;
		}
	}
	return Result;
}

TArray<FName> UNarrativeNode::GetInputNames() const
{
	TArray<FName> Result;
	for (const FNarrativePin& Pin : InputPins)
	{
		if (!Pin.PinName.IsNone())
		{
			Result.Emplace(Pin.PinName);
		}
	}
	return Result;
}

TArray<FName> UNarrativeNode::GetOutputNames() const
{
	TArray<FName> Result;
	for (const FNarrativePin& Pin : OutputPins)
	{
		if (!Pin.PinName.IsNone())
		{
			Result.Emplace(Pin.PinName);
		}
	}
	return Result;
}

#if WITH_EDITOR
bool UNarrativeNode::CanUserAddInput() const
{
	return K2_CanUserAddInput();
}

bool UNarrativeNode::CanUserAddOutput() const
{
	return K2_CanUserAddOutput();
}

void UNarrativeNode::RemoveUserInput(const FName& PinName)
{
	Modify();

	int32 RemovedPinIndex = INDEX_NONE;
	for (int32 i = 0; i < InputPins.Num(); i++)
	{
		if (InputPins[i].PinName == PinName)
		{
			InputPins.RemoveAt(i);
			RemovedPinIndex = i;
			break;
		}
	}

	// update remaining pins
	if (RemovedPinIndex > INDEX_NONE)
	{
		for (int32 i = RemovedPinIndex; i < InputPins.Num(); ++i)
		{
			if (InputPins[i].PinName.ToString().IsNumeric())
			{
				InputPins[i].PinName = *FString::FromInt(i);
			}
		}
	}
}

void UNarrativeNode::RemoveUserOutput(const FName& PinName)
{
	Modify();

	int32 RemovedPinIndex = INDEX_NONE;
	for (int32 i = 0; i < OutputPins.Num(); i++)
	{
		if (OutputPins[i].PinName == PinName)
		{
			OutputPins.RemoveAt(i);
			RemovedPinIndex = i;
			break;
		}
	}

	// update remaining pins
	if (RemovedPinIndex > INDEX_NONE)
	{
		for (int32 i = RemovedPinIndex; i < OutputPins.Num(); ++i)
		{
			if (OutputPins[i].PinName.ToString().IsNumeric())
			{
				OutputPins[i].PinName = *FString::FromInt(i);
			}
		}
	}
}
#endif

TSet<UNarrativeNode*> UNarrativeNode::GetConnectedNodes() const
{
	TSet<UNarrativeNode*> Result;
	for (const TPair<FName, FConnectedPin>& Connection : Connections)
	{
		Result.Emplace(GetNarrativeAsset()->GetNode(Connection.Value.NodeGuid));
	}
	return Result;
}

FName UNarrativeNode::GetPinConnectedToNode(const FGuid& OtherNodeGuid)
{
	for (const TPair<FName, FConnectedPin>& Connection : Connections)
	{
		if (Connection.Value.NodeGuid == OtherNodeGuid)
		{
			return Connection.Key;
		}
	}

	return NAME_None;
}

bool UNarrativeNode::IsInputConnected(const FName& PinName) const
{
	if (GetNarrativeAsset())
	{
		for (const TPair<FGuid, UNarrativeNode*>& Pair : GetNarrativeAsset()->Nodes)
		{
			if (Pair.Value)
			{
				for (const TPair<FName, FConnectedPin>& Connection : Pair.Value->Connections)
				{
					if (Connection.Value.NodeGuid == NodeGuid && Connection.Value.PinName == PinName)
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}

bool UNarrativeNode::IsOutputConnected(const FName& PinName) const
{
	return OutputPins.Contains(PinName) && Connections.Contains(PinName);
}

void UNarrativeNode::RecursiveFindNodesByClass(UNarrativeNode* Node, const TSubclassOf<UNarrativeNode> Class, uint8 Depth, TArray<UNarrativeNode*>& OutNodes)
{
	if (Node)
	{
		// Record the node if it is the desired type
		if (Node->GetClass() == Class)
		{
			OutNodes.AddUnique(Node);
		}

		if (OutNodes.Num() == Depth)
		{
			return;
		}

		// Recurse
		for (UNarrativeNode* ConnectedNode : Node->GetConnectedNodes())
		{
			RecursiveFindNodesByClass(ConnectedNode, Class, Depth, OutNodes);
		}
	}
}

UNarrativeSubsystem* UNarrativeNode::GetNarrativeSubsystem() const
{
	return GetNarrativeAsset() ? GetNarrativeAsset()->GetNarrativeSubsystem() : nullptr;
}

UWorld* UNarrativeNode::GetWorld() const
{
	if (GetNarrativeAsset() && GetNarrativeAsset()->GetNarrativeSubsystem())
	{
		return GetNarrativeAsset()->GetNarrativeSubsystem()->GetWorld();
	}

	return nullptr;
}

void UNarrativeNode::InitializeInstance()
{
	K2_InitializeInstance();
}

void UNarrativeNode::TriggerPreload()
{
	bPreloaded = true;
	PreloadContent();
}

void UNarrativeNode::TriggerFlush()
{
	bPreloaded = false;
	FlushContent();
}

void UNarrativeNode::PreloadContent()
{
	K2_PreloadContent();
}

void UNarrativeNode::FlushContent()
{
	K2_FlushContent();
}

void UNarrativeNode::OnActivate()
{
	K2_OnActivate();
}

void UNarrativeNode::TriggerInput(const FName& PinName, const ENarrativePinActivationType ActivationType /*= Default*/)
{
	if (SignalMode == ENarrativeSignalMode::Disabled)
	{
		// entirely ignore any Input activation
	}

	if (InputPins.Contains(PinName))
	{
		if (SignalMode == ENarrativeSignalMode::Enabled)
		{
			const ENarrativeNodeState PreviousActivationState = ActivationState;
			if (PreviousActivationState != ENarrativeNodeState::Active)
			{
				OnActivate();
			}

			ActivationState = ENarrativeNodeState::Active;
		}

#if !UE_BUILD_SHIPPING
		// record for debugging
		TArray<FPinRecord>& Records = InputRecords.FindOrAdd(PinName);
		Records.Add(FPinRecord(FApp::GetCurrentTime(), ActivationType));
#endif // UE_BUILD_SHIPPING

#if WITH_EDITOR
		if (GEditor && UNarrativeAsset::GetNarrativeGraphInterface().IsValid())
		{
			UNarrativeAsset::GetNarrativeGraphInterface()->OnInputTriggered(GraphNode, InputPins.IndexOfByKey(PinName));
		}
#endif // WITH_EDITOR
	}
	else
	{
#if !UE_BUILD_SHIPPING
		LogError(FString::Printf(TEXT("Input Pin name %s invalid"), *PinName.ToString()));
#endif // UE_BUILD_SHIPPING
		return;
	}

	switch (SignalMode)
	{
		case ENarrativeSignalMode::Enabled:
			ExecuteInput(PinName);
			break;
		case ENarrativeSignalMode::Disabled:
			if (UNarrativeSettings::Get()->bLogOnSignalDisabled)
			{
				LogNote(FString::Printf(TEXT("Node disabled while triggering input %s"), *PinName.ToString()));
			}
			break;
		case ENarrativeSignalMode::PassThrough:
			if (UNarrativeSettings::Get()->bLogOnSignalPassthrough)
			{
				LogNote(FString::Printf(TEXT("Signal pass-through on triggering input %s"), *PinName.ToString()));
			}
			OnPassThrough();
			break;
		default: ;
	}
}

void UNarrativeNode::ExecuteInput(const FName& PinName)
{
	K2_ExecuteInput(PinName);
}

void UNarrativeNode::TriggerFirstOutput(const bool bFinish)
{
	if (OutputPins.Num() > 0)
	{
		TriggerOutput(OutputPins[0].PinName, bFinish);
	}
}

void UNarrativeNode::TriggerOutput(const FName& PinName, const bool bFinish /*= false*/, const ENarrativePinActivationType ActivationType /*= Default*/)
{
	// clean up node, if needed
	if (bFinish)
	{
		Finish();
	}

#if !UE_BUILD_SHIPPING
	if (OutputPins.Contains(PinName))
	{
		// record for debugging, even if nothing is connected to this pin
		TArray<FPinRecord>& Records = OutputRecords.FindOrAdd(PinName);
		Records.Add(FPinRecord(FApp::GetCurrentTime(), ActivationType));

#if WITH_EDITOR
		if (GEditor && UNarrativeAsset::GetNarrativeGraphInterface().IsValid())
		{
			UNarrativeAsset::GetNarrativeGraphInterface()->OnOutputTriggered(GraphNode, OutputPins.IndexOfByKey(PinName));
		}
#endif // WITH_EDITOR
	}
	else
	{
		LogError(FString::Printf(TEXT("Output Pin name %s invalid"), *PinName.ToString()));
	}
#endif // UE_BUILD_SHIPPING

	// call the next node
	if (OutputPins.Contains(PinName) && Connections.Contains(PinName))
	{
		const FConnectedPin NarrativePin = GetConnection(PinName);
		GetNarrativeAsset()->TriggerInput(NarrativePin.NodeGuid, NarrativePin.PinName);
	}
}

void UNarrativeNode::TriggerOutputPin(const FNarrativeOutputPinHandle Pin, const bool bFinish, const ENarrativePinActivationType ActivationType /*= Default*/)
{
	TriggerOutput(Pin.PinName, bFinish, ActivationType);
}

void UNarrativeNode::TriggerOutput(const FString& PinName, const bool bFinish)
{
	TriggerOutput(*PinName, bFinish);
}

void UNarrativeNode::TriggerOutput(const FText& PinName, const bool bFinish)
{
	TriggerOutput(*PinName.ToString(), bFinish);
}

void UNarrativeNode::TriggerOutput(const TCHAR* PinName, const bool bFinish)
{
	TriggerOutput(FName(PinName), bFinish);
}

void UNarrativeNode::Finish()
{
	Deactivate();
	GetNarrativeAsset()->FinishNode(this);
}

void UNarrativeNode::Deactivate()
{
	if (GetNarrativeAsset()->FinishPolicy == ENarrativeFinishPolicy::Abort)
	{
		ActivationState = ENarrativeNodeState::Aborted;
	}
	else
	{
		ActivationState = ENarrativeNodeState::Completed;
	}

	Cleanup();
}

void UNarrativeNode::Cleanup()
{
	K2_Cleanup();
}

void UNarrativeNode::DeinitializeInstance()
{
	K2_DeinitializeInstance();
}

void UNarrativeNode::ForceFinishNode()
{
	K2_ForceFinishNode();
}

void UNarrativeNode::ResetRecords()
{
	ActivationState = ENarrativeNodeState::NeverActivated;

#if !UE_BUILD_SHIPPING
	InputRecords.Empty();
	OutputRecords.Empty();
#endif
}

void UNarrativeNode::SaveInstance(FNarrativeNodeSaveData& NodeRecord)
{
	NodeRecord.NodeGuid = NodeGuid;
	OnSave();

	FMemoryWriter MemoryWriter(NodeRecord.NodeData, true);
	FNarrativeArchive Ar(MemoryWriter);
	Serialize(Ar);
}

void UNarrativeNode::LoadInstance(const FNarrativeNodeSaveData& NodeRecord)
{
	FMemoryReader MemoryReader(NodeRecord.NodeData, true);
	FNarrativeArchive Ar(MemoryReader);
	Serialize(Ar);

	if (UNarrativeAsset* NarrativeAsset = GetNarrativeAsset())
	{
		NarrativeAsset->OnActivationStateLoaded(this);
	}

	switch (SignalMode)
	{
		case ENarrativeSignalMode::Enabled:
			OnLoad();
			break;
		case ENarrativeSignalMode::Disabled:
			// designer doesn't want to execute this node's logic at all, so we kill it
			LogNote(TEXT("Signal disabled while loading Narrative Node from SaveGame"));
			Finish();
			break;
		case ENarrativeSignalMode::PassThrough:
			LogNote(TEXT("Signal pass-through on loading Narrative Node from SaveGame"));
			OnPassThrough();
			break;
		default: ;
	}
}

void UNarrativeNode::OnSave_Implementation()
{
}

void UNarrativeNode::OnLoad_Implementation()
{
}

void UNarrativeNode::OnPassThrough_Implementation()
{
	// trigger all connected outputs
	// pin connections aren't serialized to the SaveGame, so users can safely change connections post game release
	for (const FNarrativePin& OutputPin : OutputPins)
	{
		if (Connections.Contains(OutputPin.PinName))
		{
			TriggerOutput(OutputPin.PinName, false, ENarrativePinActivationType::PassThrough);
		}
	}

	// deactivate node, so it doesn't get saved to a new SaveGame
	Finish();
}

#if WITH_EDITOR
UNarrativeNode* UNarrativeNode::GetInspectedInstance() const
{
	if (const UNarrativeAsset* NarrativeInstance = GetNarrativeAsset()->GetInspectedInstance())
	{
		return NarrativeInstance->GetNode(GetGuid());
	}

	return nullptr;
}

TMap<uint8, FPinRecord> UNarrativeNode::GetWireRecords() const
{
	TMap<uint8, FPinRecord> Result;
	for (const TPair<FName, TArray<FPinRecord>>& Record : OutputRecords)
	{
		Result.Emplace(OutputPins.IndexOfByKey(Record.Key), Record.Value.Last());
	}
	return Result;
}

TArray<FPinRecord> UNarrativeNode::GetPinRecords(const FName& PinName, const EEdGraphPinDirection PinDirection) const
{
	switch (PinDirection)
	{
		case EGPD_Input:
			return InputRecords.FindRef(PinName);
		case EGPD_Output:
			return OutputRecords.FindRef(PinName);
		default:
			return TArray<FPinRecord>();
	}
}

FString UNarrativeNode::GetStatusString() const
{
	return K2_GetStatusString();
}

bool UNarrativeNode::GetStatusBackgroundColor(FLinearColor& OutColor) const
{
	return K2_GetStatusBackgroundColor(OutColor);
}

FString UNarrativeNode::GetAssetPath()
{
	return K2_GetAssetPath();
}

UObject* UNarrativeNode::GetAssetToEdit()
{
	return K2_GetAssetToEdit();
}

AActor* UNarrativeNode::GetActorToFocus()
{
	return K2_GetActorToFocus();
}
#endif

FString UNarrativeNode::GetIdentityTagDescription(const FGameplayTag& Tag)
{
	return Tag.IsValid() ? Tag.ToString() : MissingIdentityTag;
}

FString UNarrativeNode::GetIdentityTagsDescription(const FGameplayTagContainer& Tags)
{
	return Tags.IsEmpty() ? MissingIdentityTag : FString::JoinBy(Tags, LINE_TERMINATOR, [](const FGameplayTag& Tag) { return Tag.ToString(); });
}

FString UNarrativeNode::GetNotifyTagsDescription(const FGameplayTagContainer& Tags)
{
	return Tags.IsEmpty() ? MissingNotifyTag : FString::JoinBy(Tags, LINE_TERMINATOR, [](const FGameplayTag& Tag) { return Tag.ToString(); });
}

FString UNarrativeNode::GetClassDescription(const TSubclassOf<UObject> Class)
{
	return Class ? Class->GetName() : MissingClass;
}

FString UNarrativeNode::GetProgressAsString(const float Value)
{
	return FString::Printf(TEXT("%.*f"), 2, Value);
}

void UNarrativeNode::LogError(FString Message, const ENarrativeOnScreenMessageType OnScreenMessageType)
{
#if !UE_BUILD_SHIPPING
	if (BuildMessage(Message))
	{
		// OnScreen Message
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
						return IsValid(this) && ActivationState != ENarrativeNodeState::NeverActivated;
					});
				}
			}
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, Message);
		}

		// Output Log
		UE_LOG(LogNarrative, Error, TEXT("%s"), *Message);

		// Message Log
#if WITH_EDITOR
		GetNarrativeAsset()->GetTemplateAsset()->LogError(Message, this);
#endif
	}
#endif
}

void UNarrativeNode::LogWarning(FString Message)
{
#if !UE_BUILD_SHIPPING
	if (BuildMessage(Message))
	{
		// Output Log
		UE_LOG(LogNarrative, Warning, TEXT("%s"), *Message);

		// Message Log
#if WITH_EDITOR
		GetNarrativeAsset()->GetTemplateAsset()->LogWarning(Message, this);
#endif
	}
#endif
}

void UNarrativeNode::LogNote(FString Message)
{
#if !UE_BUILD_SHIPPING
	if (BuildMessage(Message))
	{
		// Output Log
		UE_LOG(LogNarrative, Log, TEXT("%s"), *Message);

		// Message Log
#if WITH_EDITOR
		GetNarrativeAsset()->GetTemplateAsset()->LogNote(Message, this);
#endif
	}
#endif
}

#if !UE_BUILD_SHIPPING
bool UNarrativeNode::BuildMessage(FString& Message) const
{
	if (GetNarrativeAsset()->TemplateAsset) // this is runtime log which is should be only called on runtime instances of asset
	{
		const FString TemplatePath = GetNarrativeAsset()->TemplateAsset->GetPathName();
		Message.Append(TEXT(" --- node ")).Append(GetName()).Append(TEXT(", asset ")).Append(FPaths::GetPath(TemplatePath) / FPaths::GetBaseFilename(TemplatePath));
		return true;
	}

	return false;
}
#endif
