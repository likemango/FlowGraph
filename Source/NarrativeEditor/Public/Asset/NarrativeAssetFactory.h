// Copyright XiaoYao

#pragma once

#include "Factories/Factory.h"
#include "NarrativeAssetFactory.generated.h"

UCLASS(HideCategories = Object)
class NARRATIVEEDITOR_API UNarrativeAssetFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = Asset)
	TSubclassOf<class UNarrativeAsset> AssetClass;

	virtual bool ConfigureProperties() override;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;

protected:
	// Parameterized guts of ConfigureProperties()
	bool ConfigurePropertiesInternal(const FText& TitleText);
};
