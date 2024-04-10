// Copyright XiaoYao

#pragma once

#include "Engine/DeveloperSettings.h"
#include "Templates/SubclassOf.h"
#include "UObject/SoftObjectPath.h"
#include "NarrativeSettings.generated.h"

class UNarrativeNode;

/**
 *
 */
UCLASS(Config = Game, defaultconfig, meta = (DisplayName = "Narrative"))
class NARRATIVE_API UNarrativeSettings : public UDeveloperSettings
{
	GENERATED_UCLASS_BODY()

	static UNarrativeSettings* Get() { return CastChecked<UNarrativeSettings>(UNarrativeSettings::StaticClass()->GetDefaultObject()); }

	// Set if to False, if you don't want to create client-side Narrative Graphs
	// And you don't access to the Narrative Component registry on clients
	UPROPERTY(Config, EditAnywhere, Category = "Networking")
	bool bCreateNarrativeSubsystemOnClients;

	UPROPERTY(Config, EditAnywhere, Category = "SaveSystem")
	bool bWarnAboutMissingIdentityTags;

	// If enabled, runtime logs will be added when a flow node signal mode is set to Disabled
	UPROPERTY(Config, EditAnywhere, Category = "Narrative")
	bool bLogOnSignalDisabled;

	// If enabled, runtime logs will be added when a flow node signal mode is set to Pass-through
	UPROPERTY(Config, EditAnywhere, Category = "Narrative")
	bool bLogOnSignalPassthrough;

	// Adjust the Titles for NarrativeNodes to be more expressive than default
	// by incorporating data that would otherwise go in the Description
	UPROPERTY(EditAnywhere, config, Category = "Nodes")
	bool bUseAdaptiveNodeTitles;

	// Default class to use as a NarrativeAsset's "ExpectedOwnerClass" 
	UPROPERTY(EditAnywhere, Config, Category = "Nodes", meta = (MustImplement = "/Script/Narrative.NarrativeOwnerInterface"))
	FSoftClassPath DefaultExpectedOwnerClass;

public:
	UClass* GetDefaultExpectedOwnerClass() const;

	static UClass* TryResolveOrLoadSoftClass(const FSoftClassPath& SoftClassPath);

#if WITH_EDITORONLY_DATA
	virtual FName GetCategoryName() const override { return FName("Narrative Graph"); }
	virtual FText GetSectionText() const override { return INVTEXT("Settings"); }
#endif
};
