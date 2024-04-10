// Copyright XiaoYao

#pragma once

#include "Factories/Factory.h"
#include "NarrativeNodeBlueprintFactory.generated.h"

UCLASS(hidecategories=Object)
class NARRATIVEEDITOR_API UNarrativeNodeBlueprintFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

	// The parent class of the created blueprint
	UPROPERTY(EditAnywhere, Category = "NarrativeNodeBlueprintFactory")
	TSubclassOf<class UNarrativeNode> ParentClass;

	// UFactory
	virtual bool ConfigureProperties() override;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext) override;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	// --
};
