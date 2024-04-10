// Copyright XiaoYao

#pragma once

#include "Templates/SubclassOf.h"

#include "NarrativeOwnerFunctionRef.generated.h"

class UNarrativeOwnerFunctionParams;
class INarrativeOwnerInterface;

// Similar to FAnimNodeFunctionRef, providing a FName-based function binding
//  that is resolved at runtime
USTRUCT(BlueprintType)
struct FNarrativeOwnerFunctionRef
{
	GENERATED_BODY()

	// For GET_MEMBER_NAME_CHECKED access
	friend class UNarrativeNode_CallOwnerFunction;
	friend class FNarrativeOwnerFunctionRefCustomization;

public:

	// Resolves the function and returns the UFunction
	UFunction* TryResolveFunction(const UClass& InClass);

	// Returns a the resolved function
	//  (assumes TryResolveFunction was called previously)
	UFunction* GetResolvedFunction() const { return Function; }

	// Call the function and return the Output Pin Name result
	FName CallFunction(INarrativeOwnerInterface& InNarrativeOwnerInterface, UNarrativeOwnerFunctionParams& InParams) const;

	// Accessors
	FName GetFunctionName() const { return FunctionName; }
	bool IsConfigured() const { return !FunctionName.IsNone(); }
	bool IsResolved() const { return ::IsValid(Function); }

protected:

	// The name of the function to call
	UPROPERTY(VisibleAnywhere, Category = "NarrativeOwnerFunction")
	FName FunctionName = NAME_None;	

	// The function to call
	//  (resolved by looking for a function named FunctionName on the ExpectedOwnerClass)
	UPROPERTY(Transient)
	TObjectPtr<UFunction> Function = nullptr;

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Category = "NarrativeOwnerFunction", meta = (DisplayName = "Function Parameters Class"))
	TSubclassOf<UNarrativeOwnerFunctionParams> ParamsClass;
#endif // WITH_EDITORONLY_DATA
};
