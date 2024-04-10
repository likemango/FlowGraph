// Copyright XiaoYao

#pragma once

#include "CoreMinimal.h"

#include "NarrativeOwnerFunctionParams.generated.h"

class UNarrativeNode_CallOwnerFunction;

UCLASS(BlueprintType, Blueprintable, EditInlineNew)
class NARRATIVE_API UNarrativeOwnerFunctionParams : public UObject
{
	GENERATED_BODY()

public:
	UNarrativeOwnerFunctionParams();

	void PreExecute(UNarrativeNode_CallOwnerFunction& InSourceNode, const FName& InputPinName);
	void PostExecute();

	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "NarrativeOwnerFunction")
	bool ShouldFinishForOutputName(const FName& OutputName) const;

#if WITH_EDITORONLY_DATA
	const TArray<FName>& GetInputNames() const { return InputNames; }
	const TArray<FName>& GetOutputNames() const { return OutputNames; }
#endif // WITH_EDITORONLY_DATA

	// Get the input/output pin names for the SourceNode.
	//  Slightly slower than the editor-only counterparts owing to the deep copy of the array.
	//  Valid only if called between PreExecute() and PostExecute(), inclusive
	TArray<FName> GatherInputNames() const { return BP_GetInputNames(); }
	TArray<FName> GatherOutputNames() const { return BP_GetOutputNames(); }

protected:
	// Called prior to the owner executing the function described by this object.
	//  Can be overridden to prepare the stateful data before execution.
	UFUNCTION(BlueprintImplementableEvent, Category = "NarrativeOwnerFunction", DisplayName = "PreExecute")
	void BP_PreExecute();

	// Cleans up the stateful data in this Params struct.
	//  Can be overridden to cleanup the stateful data after execution.
	UFUNCTION(BlueprintImplementableEvent, Category = "NarrativeOwnerFunction", DisplayName = "PostExecute")
	void BP_PostExecute();

	// Get the input pin names for the SourceNode
	//  Valid only if called between PreExecute() and PostExecute(), inclusive
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "NarrativeOwnerFunction", DisplayName = "GetInputNames")
	TArray<FName> BP_GetInputNames() const;

	// Get the output pin names for the SourceNode
	//  Valid only if called between PreExecute() and PostExecute(), inclusive
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "NarrativeOwnerFunction", DisplayName = "GetOutputNames")
	TArray<FName> BP_GetOutputNames() const;

protected:
	// CallOwnerObjectFunction node that is executing this set of function params.
	//  Valid only if called between PreExecute() and PostExecute(), inclusive
	UPROPERTY(Transient, BlueprintReadOnly, Category = "NarrativeOwnerFunction")
	UNarrativeNode_CallOwnerFunction* SourceNode = nullptr;

	// This is the Name from the Input Pin that caused this node to Execute.
	//  Valid only if called between PreExecute() and PostExecute(), inclusive
	UPROPERTY(Transient, BlueprintReadOnly, Category = "NarrativeOwnerFunction")
	FName ExecutedInputPinName;

#if WITH_EDITORONLY_DATA
	// Input pin names for this function
	UPROPERTY(EditDefaultsOnly, Category = "NarrativeOwnerFunction")
	TArray<FName> InputNames;

	// Output pin names for this function
	UPROPERTY(EditDefaultsOnly, Category = "NarrativeOwnerFunction")
	TArray<FName> OutputNames;
#endif // WITH_EDITORONLY_DATA
};
