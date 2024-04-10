// Copyright XiaoYao

#include "Nodes/Route/NarrativeNode_SubGraph.h"

#include "NarrativeAsset.h"
#include "NarrativeMessageLog.h"
#include "NarrativeSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativeNode_SubGraph)

FNarrativePin UNarrativeNode_SubGraph::StartPin(TEXT("Start"));
FNarrativePin UNarrativeNode_SubGraph::FinishPin(TEXT("Finish"));

UNarrativeNode_SubGraph::UNarrativeNode_SubGraph(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bCanInstanceIdenticalAsset(false)
{
#if WITH_EDITOR
	Category = TEXT("Route");
	NodeStyle = ENarrativeNodeStyle::SubGraph;

	AllowedAssignedAssetClasses = {UNarrativeAsset::StaticClass()};
#endif

	InputPins = {StartPin};
	OutputPins = {FinishPin};
}

bool UNarrativeNode_SubGraph::CanBeAssetInstanced() const
{
	return !Asset.IsNull() && (bCanInstanceIdenticalAsset || Asset.ToString() != GetNarrativeAsset()->GetTemplateAsset()->GetPathName());
}

void UNarrativeNode_SubGraph::PreloadContent()
{
	if (CanBeAssetInstanced() && GetNarrativeSubsystem())
	{
		GetNarrativeSubsystem()->CreateSubNarrative(this, FString(), true);
	}
}

void UNarrativeNode_SubGraph::FlushContent()
{
	if (CanBeAssetInstanced() && GetNarrativeSubsystem())
	{
		GetNarrativeSubsystem()->RemoveSubNarrative(this, ENarrativeFinishPolicy::Abort);
	}
}

void UNarrativeNode_SubGraph::ExecuteInput(const FName& PinName)
{
	if (CanBeAssetInstanced() == false)
	{
		if (Asset.IsNull())
		{
			LogError(TEXT("Missing Narrative Asset"));
		}
		else
		{
			LogError(FString::Printf(TEXT("Asset %s cannot be instance, probably is the same as the asset owning this SubGraph node."), *Asset.ToString()));
		}
		
		Finish();
		return;
	}
	
	if (PinName == TEXT("Start"))
	{
		if (GetNarrativeSubsystem())
		{
			GetNarrativeSubsystem()->CreateSubNarrative(this);
		}
	}
	else if (!PinName.IsNone())
	{
		GetNarrativeAsset()->TriggerCustomInput_FromSubGraph(this, PinName);
	}
}

void UNarrativeNode_SubGraph::Cleanup()
{
	if (CanBeAssetInstanced() && GetNarrativeSubsystem())
	{
		GetNarrativeSubsystem()->RemoveSubNarrative(this, ENarrativeFinishPolicy::Keep);
	}
}

void UNarrativeNode_SubGraph::ForceFinishNode()
{
	TriggerFirstOutput(true);
}

void UNarrativeNode_SubGraph::OnLoad_Implementation()
{
	if (!SavedAssetInstanceName.IsEmpty() && !Asset.IsNull())
	{
		GetNarrativeSubsystem()->LoadSubNarrative(this, SavedAssetInstanceName);
		SavedAssetInstanceName = FString();
	}
}

#if WITH_EDITOR
FString UNarrativeNode_SubGraph::GetNodeDescription() const
{
	return Asset.IsNull() ? FString() : Asset.ToSoftObjectPath().GetAssetName();
}

UObject* UNarrativeNode_SubGraph::GetAssetToEdit()
{
	return Asset.IsNull() ? nullptr : Asset.LoadSynchronous();
}

EDataValidationResult UNarrativeNode_SubGraph::ValidateNode()
{
	if (Asset.IsNull())
	{
		ValidationLog.Error<UNarrativeNode>(TEXT("Narrative Asset not assigned or invalid!"), this);
		return EDataValidationResult::Invalid;
	}

	return EDataValidationResult::Valid;
}

TArray<FNarrativePin> UNarrativeNode_SubGraph::GetContextInputs()
{
	TArray<FNarrativePin> EventNames;

	if (!Asset.IsNull())
	{
		Asset.LoadSynchronous();
		for (const FName& PinName : Asset.Get()->GetCustomInputs())
		{
			if (!PinName.IsNone())
			{
				EventNames.Emplace(PinName);
			}
		}
	}

	return EventNames;
}

TArray<FNarrativePin> UNarrativeNode_SubGraph::GetContextOutputs()
{
	TArray<FNarrativePin> Pins;

	if (!Asset.IsNull())
	{
		Asset.LoadSynchronous();
		for (const FName& PinName : Asset.Get()->GetCustomOutputs())
		{
			if (!PinName.IsNone())
			{
				Pins.Emplace(PinName);
			}
		}
	}

	return Pins;
}

void UNarrativeNode_SubGraph::PostLoad()
{
	Super::PostLoad();

	SubscribeToAssetChanges();
}

void UNarrativeNode_SubGraph::PreEditChange(FProperty* PropertyAboutToChange)
{
	Super::PreEditChange(PropertyAboutToChange);

	if (PropertyAboutToChange->GetFName() == GET_MEMBER_NAME_CHECKED(UNarrativeNode_SubGraph, Asset))
	{
		if (Asset)
		{
			Asset->OnSubGraphReconstructionRequested.Unbind();
		}
	}
}

void UNarrativeNode_SubGraph::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property && PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UNarrativeNode_SubGraph, Asset))
	{
		OnReconstructionRequested.ExecuteIfBound();
		SubscribeToAssetChanges();
	}
}

void UNarrativeNode_SubGraph::SubscribeToAssetChanges()
{
	if (Asset)
	{
		TWeakObjectPtr<UNarrativeNode_SubGraph> SelfWeakPtr(this);
		Asset->OnSubGraphReconstructionRequested.BindLambda([SelfWeakPtr]()
		{
			if (SelfWeakPtr.IsValid())
			{
				SelfWeakPtr->OnReconstructionRequested.ExecuteIfBound();
			}
		});
	}
}
#endif
