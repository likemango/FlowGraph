// Copyright XiaoYao

#pragma once

#include "NarrativeOwnerFunctionRef.h"
#include "Nodes/NarrativeNode.h"

#include "NarrativeNode_CallOwnerFunction.generated.h"

class UNarrativeOwnerFunctionParams;
class INarrativeOwnerInterface;

// Example signature for valid Narrative Owner Functions
typedef TFunction<FName(UNarrativeOwnerFunctionParams* Params)> FNarrativeOwnerFunctionSignature;

/**
 * NarrativeNode to call an owner function
 * - Owner must implement INarrativeOwnerInterface
 * - Callable functions must take a single input parameter deriving from UNarrativeOwnerFunctionParams
 *   and return FName for the Output event to trigger (or "None" to trigger none of the outputs)
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Call Owner Function"))
class NARRATIVE_API UNarrativeNode_CallOwnerFunction : public UNarrativeNode
{
	GENERATED_UCLASS_BODY()

protected:
	// Function reference on the expected owner to call
	UPROPERTY(EditAnywhere, Category = "Call Owner", meta = (DisplayName = "Function"))
	FNarrativeOwnerFunctionRef FunctionRef;

	// Parameter object to pass to the function when called
	UPROPERTY(EditAnywhere, Category = "Call Owner", Instanced)
	UNarrativeOwnerFunctionParams* Params;

protected:
	// UNarrativeNode
	virtual void ExecuteInput(const FName& PinName) override;
	// ---

	bool TryExecuteOutputPin(const FName& OutputName);
	bool ShouldFinishForOutputName(const FName& OutputName) const;

#if WITH_EDITOR

public:
	// UNarrativeNode
	virtual FText GetNodeTitle() const override;
	virtual FString GetNodeDescription() const override;
	virtual FString GetStatusString() const override;
	virtual EDataValidationResult ValidateNode() override;

	virtual bool SupportsContextPins() const override { return true; };
	virtual TArray<FNarrativePin> GetContextInputs() override;
	virtual TArray<FNarrativePin> GetContextOutputs() override;
	// ---

	// UObject
	virtual void PostLoad() override;
	virtual bool CanEditChange(const FProperty* InProperty) const override;
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	// ---

protected:
	bool TryAllocateParamsInstance();

	UClass* GetRequiredParamsClass() const;
	UClass* GetExistingParamsClass() const;

	static UClass* GetParamsClassForFunctionName(const UClass& ExpectedOwnerClass, const FName& FunctionName);
	static UClass* GetParamsClassForFunction(const UFunction& Function);

public:
	bool IsAcceptableParamsPropertyClass(const UClass* ParamsClass) const;

	UClass* TryGetExpectedOwnerClass() const;
	static bool DoesFunctionHaveValidNarrativeOwnerFunctionSignature(const UFunction& Function);

protected:
	// Helper function for DoesFunctionHaveValidNarrativeOwnerFunctionSignature()
	static bool DoesFunctionHaveNameReturnType(const UFunction& Function);
#endif // WITH_EDITOR
};
