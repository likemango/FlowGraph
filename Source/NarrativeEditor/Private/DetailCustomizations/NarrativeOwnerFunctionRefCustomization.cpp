// Copyright XiaoYao

#include "DetailCustomizations/NarrativeOwnerFunctionRefCustomization.h"

#include "Nodes/NarrativeNode.h"
#include "Nodes/World/NarrativeNode_CallOwnerFunction.h"

#include "UObject/UnrealType.h"

void FNarrativeOwnerFunctionRefCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	// Do not include children properties (the header is all we need to show for this struct)
}

TSharedPtr<IPropertyHandle> FNarrativeOwnerFunctionRefCustomization::GetCuratedNamePropertyHandle() const
{
	check(StructPropertyHandle->IsValidHandle());

	TSharedPtr<IPropertyHandle> FoundHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FNarrativeOwnerFunctionRef, FunctionName));
	check(FoundHandle);

	return FoundHandle;
}

TArray<FName> FNarrativeOwnerFunctionRefCustomization::GetCuratedNameOptions() const
{
	TArray<FName> Results;

	const UClass* ExpectedOwnerClass = TryGetExpectedOwnerClass();
	if (!IsValid(ExpectedOwnerClass))
	{
		return Results;
	}

	const UNarrativeNode_CallOwnerFunction* NarrativeNodeOwner = Cast<UNarrativeNode_CallOwnerFunction>(TryGetNarrativeNodeOuter());
	if (!IsValid(NarrativeNodeOwner))
	{
		return Results;
	}

	Results = GetNarrativeOwnerFunctionRefs(*NarrativeNodeOwner, *ExpectedOwnerClass);

	return Results;
}

const UClass* FNarrativeOwnerFunctionRefCustomization::TryGetExpectedOwnerClass() const
{
	const UNarrativeNode* NodeOwner = TryGetNarrativeNodeOuter();
	const UNarrativeNode_CallOwnerFunction* CallOwnerFunctionNode = Cast<UNarrativeNode_CallOwnerFunction>(NodeOwner);

	if (IsValid(CallOwnerFunctionNode))
	{
		return CallOwnerFunctionNode->TryGetExpectedOwnerClass();
	}

	return nullptr;
}

TArray<FName> FNarrativeOwnerFunctionRefCustomization::GetNarrativeOwnerFunctionRefs(
	const UNarrativeNode_CallOwnerFunction& NarrativeNodeOwner,
	const UClass& ExpectedOwnerClass)
{
	TArray<FName> ValidFunctionNames;

	// Gather a list of potential functions
	TSet<FName> PotentialFunctionNames;

	const UClass* CurClass = &ExpectedOwnerClass;
	while (IsValid(CurClass))
	{
		TArray<FName> CurClassFunctionNames;
		CurClass->GenerateFunctionList(CurClassFunctionNames);

		PotentialFunctionNames.Append(CurClassFunctionNames);

		// Recurse to include all of the Super(s) names
		CurClass = CurClass->GetSuperClass();
	}

	if (PotentialFunctionNames.Num() == 0)
	{
		return ValidFunctionNames;
	}

	ValidFunctionNames.Reserve(PotentialFunctionNames.Num());

	// Filter out any unusable functions (that do not match the expected signature)
	for (const FName& PotentialFunctionName : PotentialFunctionNames)
	{
		const UFunction* PotentialFunction = ExpectedOwnerClass.FindFunctionByName(PotentialFunctionName);
		check(IsValid(PotentialFunction));

		if (IsFunctionUsable(*PotentialFunction, NarrativeNodeOwner))
		{
			ValidFunctionNames.Add(PotentialFunctionName);
		}
	}

	return ValidFunctionNames;
}

bool FNarrativeOwnerFunctionRefCustomization::IsFunctionUsable(const UFunction& Function, const UNarrativeNode_CallOwnerFunction& NarrativeNodeOwner)
{
	if (!UNarrativeNode_CallOwnerFunction::DoesFunctionHaveValidNarrativeOwnerFunctionSignature(Function))
	{
		return false;
	}

	return true;
}

void FNarrativeOwnerFunctionRefCustomization::SetCuratedName(const FName& NewFunctionName)
{
	TSharedPtr<IPropertyHandle> FunctionNameHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FNarrativeOwnerFunctionRef, FunctionName));

	check(FunctionNameHandle);

	FunctionNameHandle->SetPerObjectValue(0, NewFunctionName.ToString());
}

FName FNarrativeOwnerFunctionRefCustomization::GetCuratedName() const
{
	const FNarrativeOwnerFunctionRef* NarrativeOwnerFunction = GetNarrativeOwnerFunctionRef();
	if (NarrativeOwnerFunction)
	{
		return NarrativeOwnerFunction->FunctionName;
	}
	else
	{
		return NAME_None;
	}
}

UNarrativeNode* FNarrativeOwnerFunctionRefCustomization::TryGetNarrativeNodeOuter() const
{
	check(StructPropertyHandle->IsValidHandle());

	TArray<UObject*> OuterObjects;
	StructPropertyHandle->GetOuterObjects(OuterObjects);

	for (UObject* OuterObject : OuterObjects)
	{
		UNarrativeNode* NarrativeNodeOuter = Cast<UNarrativeNode>(OuterObject);
		if (IsValid(NarrativeNodeOuter))
		{
			return NarrativeNodeOuter;
		}
	}

	return nullptr;
}
