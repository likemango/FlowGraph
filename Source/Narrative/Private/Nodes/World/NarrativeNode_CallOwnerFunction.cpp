// Copyright XiaoYao

#include "Nodes/World/NarrativeNode_CallOwnerFunction.h"

#include "NarrativeAsset.h"
#include "NarrativeLogChannels.h"
#include "NarrativeOwnerInterface.h"
#include "NarrativeOwnerFunctionParams.h"
#include "NarrativeSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativeNode_CallOwnerFunction)

#define LOCTEXT_NAMESPACE "NarrativeNode"

UNarrativeNode_CallOwnerFunction::UNarrativeNode_CallOwnerFunction(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, Params(nullptr)
{
#if WITH_EDITOR
	NodeStyle = ENarrativeNodeStyle::Default;
	Category = TEXT("World");
#endif
}

void UNarrativeNode_CallOwnerFunction::ExecuteInput(const FName& PinName)
{
	Super::ExecuteInput(PinName);

	if (!IsValid(Params))
	{
		UE_LOG(LogNarrative, Error, TEXT("Expected a valid Params object"));

		return;
	}

	INarrativeOwnerInterface* NarrativeOwnerInterface = GetNarrativeOwnerInterface();
	if (!NarrativeOwnerInterface)
	{
		UE_LOG(LogNarrative, Error, TEXT("Expected an owner that implements the INarrativeOwnerInterface"));

		return;
	}

	const UObject* NarrativeOwnerObject = CastChecked<UObject>(NarrativeOwnerInterface);
	const UClass* NarrativeOwnerClass = NarrativeOwnerObject->GetClass();
	check(IsValid(NarrativeOwnerClass));

	if (!FunctionRef.TryResolveFunction(*NarrativeOwnerClass))
	{
		UE_LOG(
			LogNarrative,
			Error,
			TEXT("Could not resolve function named %s with flow owner class %s"),
			*FunctionRef.GetFunctionName().ToString(),
			*NarrativeOwnerClass->GetName());

		return;
	}

	Params->PreExecute(*this, PinName);

	const FName ResultOutputName = FunctionRef.CallFunction(*NarrativeOwnerInterface, *Params);

	Params->PostExecute();

	(void)TryExecuteOutputPin(ResultOutputName);
}

bool UNarrativeNode_CallOwnerFunction::TryExecuteOutputPin(const FName& OutputName)
{
	if (OutputName.IsNone())
	{
		return false;
	}

	const bool bFinish = ShouldFinishForOutputName(OutputName);
	TriggerOutput(OutputName, bFinish);

	return true;
}

bool UNarrativeNode_CallOwnerFunction::ShouldFinishForOutputName(const FName& OutputName) const
{
	if (ensure(IsValid(Params)))
	{
		return Params->ShouldFinishForOutputName(OutputName);
	}

	return true;
}

#if WITH_EDITOR

void UNarrativeNode_CallOwnerFunction::PostLoad()
{
	Super::PostLoad();

	FObjectPropertyBase* ParamsProperty = FindFProperty<FObjectPropertyBase>(GetClass(), GET_MEMBER_NAME_CHECKED(UNarrativeNode_CallOwnerFunction, Params));
	check(ParamsProperty);

	// NOTE (gtaylor) This fixes corruption in NarrativeNodes that could have been caused with
	// a previous version of the code (which was inadvisedly calling SetPropertyClass)
	// to restore the correct PropertyClass for this node.  
	// (it could be removed in a future release, once all assets have been updated)
	if (ParamsProperty->PropertyClass != UNarrativeOwnerFunctionParams::StaticClass())
	{
		ParamsProperty->SetPropertyClass(UNarrativeOwnerFunctionParams::StaticClass());
	}
}

bool UNarrativeNode_CallOwnerFunction::CanEditChange(const FProperty* InProperty) const
{
	if (!Super::CanEditChange(InProperty))
	{
		return false;
	}

	const FName PropertyName = InProperty->GetFName();

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UNarrativeNode_CallOwnerFunction, Params))
	{
		return false;
	}

	return true;
}

void UNarrativeNode_CallOwnerFunction::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName MemberPropertyName = PropertyChangedEvent.MemberProperty->GetFName();

	if (MemberPropertyName == GET_MEMBER_NAME_CHECKED(UNarrativeNode_CallOwnerFunction, Params))
	{
		OnReconstructionRequested.ExecuteIfBound();
	}

	const FName PropertyName = PropertyChangedEvent.Property->GetFName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(FNarrativeOwnerFunctionRef, FunctionName))
	{
		if (TryAllocateParamsInstance() || FunctionRef.GetFunctionName().IsNone())
		{
			OnReconstructionRequested.ExecuteIfBound();
		}
	}
}

bool UNarrativeNode_CallOwnerFunction::TryAllocateParamsInstance()
{
	if (FunctionRef.GetFunctionName().IsNone())
	{
		// Throw out the old params object (if any)
		Params = nullptr;

		return false;
	}

	const UClass* ExistingParamsClass = GetExistingParamsClass();
	const UClass* RequiredParamsClass = GetRequiredParamsClass();

	const bool bNeedsAllocateParams =
		!IsValid(ExistingParamsClass) ||
		ExistingParamsClass != RequiredParamsClass;

	if (!bNeedsAllocateParams)
	{
		return false;
	}

	// Throw out the old params object (if any)
	Params = nullptr;

	// Create the new params object
	Params = NewObject<UNarrativeOwnerFunctionParams>(this, RequiredParamsClass);

	return true;
}

UClass* UNarrativeNode_CallOwnerFunction::GetRequiredParamsClass() const
{
	const UClass* ExpectedOwnerClass = TryGetExpectedOwnerClass();
	if (!IsValid(ExpectedOwnerClass))
	{
		return UNarrativeOwnerFunctionParams::StaticClass();
	}

	const FName FunctionNameAsName = FunctionRef.GetFunctionName();

	if (FunctionNameAsName.IsNone())
	{
		return UNarrativeOwnerFunctionParams::StaticClass();
	}
	
	UClass* RequiredParamsClass = GetParamsClassForFunctionName(*ExpectedOwnerClass, FunctionNameAsName);
	return RequiredParamsClass;
}

UClass* UNarrativeNode_CallOwnerFunction::GetExistingParamsClass() const
{
	if (!IsValid(Params))
	{
		return nullptr;
	}

	UClass* ExistingParamsClass = Params->GetClass();
	return ExistingParamsClass;
}

UClass* UNarrativeNode_CallOwnerFunction::GetParamsClassForFunctionName(const UClass& ExpectedOwnerClass, const FName& FunctionName)
{
	const UFunction* Function = ExpectedOwnerClass.FindFunctionByName(FunctionName);
	if (IsValid(Function))
	{
		return GetParamsClassForFunction(*Function);
	}

	return nullptr;
}

FText UNarrativeNode_CallOwnerFunction::GetNodeTitle() const
{
	const bool bUseAdaptiveNodeTitles = UNarrativeSettings::Get()->bUseAdaptiveNodeTitles;

	if (bUseAdaptiveNodeTitles && !FunctionRef.GetFunctionName().IsNone())
	{
		const FText FunctionNameText = FText::FromName(FunctionRef.FunctionName);

		return FText::Format(LOCTEXT("CallOwnerFunction", "Call {0}"), {FunctionNameText});
	}
	else
	{
		return Super::GetNodeTitle();
	}
}

FString UNarrativeNode_CallOwnerFunction::GetNodeDescription() const
{
	if (UNarrativeSettings::Get()->bUseAdaptiveNodeTitles)
	{
		return Super::GetNodeDescription();
	}

	return FunctionRef.FunctionName.ToString();
}

bool UNarrativeNode_CallOwnerFunction::IsAcceptableParamsPropertyClass(const UClass* ParamsClass) const
{
	if (!IsValid(ParamsClass))
	{
		return false;
	}

	if (!ParamsClass->IsChildOf<UNarrativeOwnerFunctionParams>())
	{
		return false;
	}

	const UClass* ExistingParamsClass = GetExistingParamsClass();

	if (IsValid(ExistingParamsClass) && ParamsClass != ExistingParamsClass)
	{
		return false;
	}

	return true;
}

UClass* UNarrativeNode_CallOwnerFunction::TryGetExpectedOwnerClass() const
{
	const UNarrativeAsset* NarrativeAsset = GetNarrativeAsset();
	if (IsValid(NarrativeAsset))
	{
		return NarrativeAsset->GetExpectedOwnerClass();
	}

	return nullptr;
}

bool UNarrativeNode_CallOwnerFunction::DoesFunctionHaveValidNarrativeOwnerFunctionSignature(const UFunction& Function)
{
	if (GetParamsClassForFunction(Function) == nullptr)
	{
		return false;
	}

	checkf(Function.NumParms == 2, TEXT("This should be checked in GetParamsClassForFunction()"));

	if (!DoesFunctionHaveNameReturnType(Function))
	{
		return false;
	}

	return true;
}

bool UNarrativeNode_CallOwnerFunction::DoesFunctionHaveNameReturnType(const UFunction& Function)
{
	checkf(Function.NumParms == 2, TEXT("This should have already been checked in DoesFunctionHaveValidNarrativeOwnerFunctionSignature()"));

	TFieldIterator<FNameProperty> Iterator(&Function);

	while (Iterator)
	{
		return EnumHasAllFlags(Iterator->PropertyFlags, CPF_Parm | CPF_OutParm);
	}

	return false;
}

UClass* UNarrativeNode_CallOwnerFunction::GetParamsClassForFunction(const UFunction& Function)
{
	if (Function.NumParms != 2)
	{
		// Narrative Owner Functions expect exactly two parameters:
		//  - FNarrativeOwnerFunctionParams* 
		//  - FName (return)
		// See FNarrativeOwnerFunctionSignature

		return nullptr;
	}

	TFieldIterator<FObjectPropertyBase> Iterator(&Function);

	while (Iterator && (Iterator->PropertyFlags & CPF_Parm))
	{
		const FObjectPropertyBase* Prop = *Iterator;
		check(Prop);

		UClass* PropertyClass = Prop->PropertyClass;

		if (!IsValid(PropertyClass))
		{
			return nullptr;
		}

		if (!PropertyClass->IsChildOf<UNarrativeOwnerFunctionParams>())
		{
			return nullptr;
		}

		return PropertyClass;
	}

	return nullptr;
}

FString UNarrativeNode_CallOwnerFunction::GetStatusString() const
{
	if (ActivationState != ENarrativeNodeState::NeverActivated)
	{
		return UEnum::GetDisplayValueAsText(ActivationState).ToString();
	}

	return Super::GetStatusString();
}

EDataValidationResult UNarrativeNode_CallOwnerFunction::ValidateNode()
{
	const bool bHasFunction = FunctionRef.IsConfigured();
	if (!bHasFunction)
	{
		ValidationLog.Error<UNarrativeNode>(TEXT("CallOwnerFunction requires a valid Function reference"), this);

		return EDataValidationResult::Invalid;
	}

	const bool bHasParams = IsValid(Params);
	if (!bHasParams)
	{
		ValidationLog.Error<UNarrativeNode>(TEXT("CallOwnerFunction requires a valid Params object"), this);

		return EDataValidationResult::Invalid;
	}

	checkf(bHasParams && bHasFunction, TEXT("This should be assured by the preceding logic"));

	const UClass* ExpectedOwnerClass = TryGetExpectedOwnerClass();
	if (!IsValid(ExpectedOwnerClass))
	{
		ValidationLog.Error<UNarrativeNode>(TEXT("Invalid or null Expected Owner Class for this Narrative Asset"), this);

		return EDataValidationResult::Invalid;
	}

	// Check if the function can be found on the expected owner
	const UFunction* Function = FunctionRef.TryResolveFunction(*ExpectedOwnerClass);
	if (!IsValid(Function))
	{
		ValidationLog.Error<UNarrativeNode>(TEXT("Could not resolve function for flow owner"), this);

		return EDataValidationResult::Invalid;
	}

	// Check the function signature
	if (!DoesFunctionHaveValidNarrativeOwnerFunctionSignature(*Function))
	{
		ValidationLog.Error<UNarrativeNode>(TEXT("Narrative Owner Function has an invalid signature"), this);

		return EDataValidationResult::Invalid;
	}

	const UClass* RequiredParamsClass = GetRequiredParamsClass();
	checkf(IsValid(RequiredParamsClass), TEXT("GetRequiredParamsClass() cannot return null if DoesFunctionHaveValidNarrativeOwnerFunctionSignature() is true"));

	const UClass* ExistingParamsClass = GetExistingParamsClass();
	checkf(IsValid(ExistingParamsClass), TEXT("This should be assured, if bHasParams == true"));

	// Check if the params (existing) are compatible with the function's expected (required) params
	if (!ExistingParamsClass->IsChildOf(RequiredParamsClass))
	{
		ValidationLog.Error<UNarrativeNode>(TEXT("Params object is not of the correct type for the flow owner function"), this);

		return EDataValidationResult::Invalid;
	}

	return EDataValidationResult::Valid;
}

TArray<FNarrativePin> UNarrativeNode_CallOwnerFunction::GetContextInputs()
{
	// refresh Params, just in case function argument type was changed
	TryAllocateParamsInstance();
	
	TArray<FNarrativePin> Pins = {};

	if (Params)
	{
		for (const FName& Name : Params->GetInputNames())
		{
			if (InputPins.Contains(Name))
			{
				continue;
			}
			
			Pins.Emplace(Name);
		}
	}
	return Pins;
}

TArray<FNarrativePin> UNarrativeNode_CallOwnerFunction::GetContextOutputs()
{
	// refresh Params, just in case function argument type was changed
	TryAllocateParamsInstance();
	
	TArray<FNarrativePin> Pins = {};

	if (Params)
	{
		for (const FName& Name : Params->GetOutputNames())
		{
			if (OutputPins.Contains(Name))
			{
				continue;
			}
			
			Pins.Emplace(Name);
		}
	}
	return Pins;
}

#endif // WITH_EDITOR

#undef LOCTEXT_NAMESPACE
