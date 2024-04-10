// Copyright XiaoYao

#include "NarrativeOwnerFunctionRef.h"

#include "NarrativeLogChannels.h"
#include "NarrativeOwnerFunctionParams.h"
#include "NarrativeOwnerInterface.h"

#include "Logging/LogMacros.h"
#include "UObject/Class.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativeOwnerFunctionRef)

UFunction* FNarrativeOwnerFunctionRef::TryResolveFunction(const UClass& InClass)
{
	if (IsConfigured())
	{
		Function = InClass.FindFunctionByName(FunctionName);
	}
	else
	{
		Function = nullptr;
	}

	return Function;
}

FName FNarrativeOwnerFunctionRef::CallFunction(INarrativeOwnerInterface& InNarrativeOwnerInterface, UNarrativeOwnerFunctionParams& InParams) const
{
	if (!IsResolved())
	{
		const UObject* NarrativeOwnerObject = CastChecked<UObject>(&InNarrativeOwnerInterface);

		UE_LOG(
			LogNarrative,
			Error,
			TEXT("Could not resolve function named %s with flow owner class %s"),
			*FunctionName.ToString(),
			*NarrativeOwnerObject->GetClass()->GetName());

		return NAME_None;
	}

	UObject* NarrativeOwnerObject = CastChecked<UObject>(&InNarrativeOwnerInterface);

	struct FNarrativeOwnerFunctionRef_Parms
	{
		// Single FunctionParams object parameter
		UNarrativeOwnerFunctionParams* Params;

		// Return value
		FName OutputPinName;
	};

	FNarrativeOwnerFunctionRef_Parms Parms = {&InParams, NAME_None};

	// Call the owner function itself
	NarrativeOwnerObject->ProcessEvent(Function, &Parms);

	// Ensure the return value is valid
	if (!Parms.OutputPinName.IsNone())
	{
		const TArray<FName> OutputNames = InParams.GatherOutputNames();

		if (!OutputNames.Contains(Parms.OutputPinName))
		{
			FString OutputNamesStr = TEXT("None");
			for (const FName& OutputName : OutputNames)
			{
				OutputNamesStr += TEXT(", ") + OutputName.ToString();
			}

			UE_LOG(
				LogNarrative,
				Error,
				TEXT("Narrative Owner Function %s returned an invalid OutputPinName '%s', which is not in the valid outputs: { %s }"),
				*FunctionName.ToString(),
				*Parms.OutputPinName.ToString(),
				*OutputNamesStr);

			// Replace the invalid output pin name with None
			Parms.OutputPinName = NAME_None;
		}
	}

	return Parms.OutputPinName;
}
