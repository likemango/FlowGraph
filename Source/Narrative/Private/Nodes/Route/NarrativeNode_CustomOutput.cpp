// Copyright XiaoYao

#include "Nodes/Route/NarrativeNode_CustomOutput.h"
#include "NarrativeAsset.h"
#include "NarrativeSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativeNode_CustomOutput)

#define LOCTEXT_NAMESPACE "NarrativeNode_CustomOutput"

UNarrativeNode_CustomOutput::UNarrativeNode_CustomOutput(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	OutputPins.Empty();
}

void UNarrativeNode_CustomOutput::ExecuteInput(const FName& PinName)
{
	UNarrativeAsset* NarrativeAsset = GetNarrativeAsset();
	check(IsValid(NarrativeAsset));

	if (EventName.IsNone())
	{
		LogWarning(FString::Printf(TEXT("Attempted to trigger a CustomOutput (Node %s, Asset %s), with no EventName"),
		                           *GetName(),
		                           *NarrativeAsset->GetPathName()));
	}
	else if (!NarrativeAsset->TryFindCustomOutputNodeByEventName(EventName))
	{
		const TArray<FName> OutputNames = NarrativeAsset->GatherCustomOutputNodeEventNames();
		FString CustomOutputsString;

		for (const FName& OutputName : OutputNames)
		{
			if (!CustomOutputsString.IsEmpty())
			{
				CustomOutputsString += TEXT(", ");
			}

			CustomOutputsString += OutputName.ToString();
		}

		LogWarning(FString::Printf(TEXT("Attempted to trigger a CustomOutput (Node %s, Asset %s), with EventName %s, which is not a listed CustomOutput { %s }"),
		                           *GetName(),
		                           *NarrativeAsset->GetPathName(),
		                           *EventName.ToString(),
		                           *CustomOutputsString));
	}
	else
	{
		NarrativeAsset->TriggerCustomOutput(EventName);
	}
}

#if WITH_EDITOR
FText UNarrativeNode_CustomOutput::GetNodeTitle() const
{
	if (!EventName.IsNone() && UNarrativeSettings::Get()->bUseAdaptiveNodeTitles)
	{
		return FText::Format(LOCTEXT("CustomOutputTitle", "{0} Output"), {FText::FromString(EventName.ToString())});
	}

	return Super::GetNodeTitle();
}
#endif

#undef LOCTEXT_NAMESPACE
