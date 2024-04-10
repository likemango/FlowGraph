// Copyright XiaoYao

#pragma once

#include "NarrativeGraphConnectionDrawingPolicy.h"
#include "Engine/DeveloperSettings.h"

#include "NarrativeTypes.h"
#include "NarrativeGraphSettings.generated.h"

/**
 *
 */
UCLASS(Config = Editor, defaultconfig, meta = (DisplayName = "Narrative Graph"))
class NARRATIVEEDITOR_API UNarrativeGraphSettings : public UDeveloperSettings
{
	GENERATED_UCLASS_BODY()
	static UNarrativeGraphSettings* Get() { return StaticClass()->GetDefaultObject<UNarrativeGraphSettings>(); }

	/** Show Narrative Asset in Narrative category of "Create Asset" menu?
	* Requires restart after making a change. */
	UPROPERTY(EditAnywhere, config, Category = "Default UI", meta = (ConfigRestartRequired = true))
	bool bExposeNarrativeAssetCreation;

	/** Show Narrative Node blueprint in Narrative category of "Create Asset" menu?
	* Requires restart after making a change. */
	UPROPERTY(EditAnywhere, config, Category = "Default UI", meta = (ConfigRestartRequired = true))
	bool bExposeNarrativeNodeCreation;

	/** Show Narrative Asset toolbar?
	* Requires restart after making a change. */
	UPROPERTY(EditAnywhere, config, Category = "Default UI", meta = (ConfigRestartRequired = true))
	bool bShowAssetToolbarAboveLevelEditor;

	UPROPERTY(EditAnywhere, config, Category = "Default UI", meta = (ConfigRestartRequired = true))
	FText NarrativeAssetCategoryName;

	/** Use this class to create new assets. Class picker will show up if None */
	UPROPERTY(EditAnywhere, config, Category = "Default UI")
	TSubclassOf<class UNarrativeAsset> DefaultNarrativeAssetClass;

	/** Narrative Asset class allowed to be assigned via Level Editor toolbar*/
	UPROPERTY(EditAnywhere, config, Category = "Default UI", meta = (EditCondition = "bShowAssetToolbarAboveLevelEditor"))
	TSubclassOf<class UNarrativeAsset> WorldAssetClass;

	/** Hide specific nodes from the Narrative Palette without changing the source code.
	* Requires restart after making a change. */
	UPROPERTY(EditAnywhere, config, Category = "Nodes")
	TArray<TSubclassOf<class UNarrativeNode>> NodesHiddenFromPalette;

	/** Hide default pin names on simple nodes, reduces UI clutter */
	UPROPERTY(EditAnywhere, config, Category = "Nodes")
	bool bShowDefaultPinNames;

	UPROPERTY(EditAnywhere, config, Category = "Nodes")
	TMap<ENarrativeNodeStyle, FLinearColor> NodeTitleColors;

	UPROPERTY(Config, EditAnywhere, Category = "Nodes")
	TMap<TSubclassOf<UNarrativeNode>, FLinearColor> NodeSpecificColors;

	UPROPERTY(EditAnywhere, config, Category = "Nodes")
	FLinearColor ExecPinColorModifier;

	UPROPERTY(EditAnywhere, config, Category = "NodePopups")
	FLinearColor NodeDescriptionBackground;

	UPROPERTY(EditAnywhere, config, Category = "NodePopups")
	FLinearColor NodeStatusBackground;

	UPROPERTY(EditAnywhere, config, Category = "NodePopups")
	FLinearColor NodePreloadedBackground;

	UPROPERTY(config, EditAnywhere, Category = "Wires")
	ENarrativeConnectionDrawType ConnectionDrawType;

	UPROPERTY(config, EditAnywhere, Category = "Wires", meta = (EditCondition = "ConnectionDrawType == ENarrativeConnectionDrawType::Circuit"))
	float CircuitConnectionAngle;

	UPROPERTY(config, EditAnywhere, Category = "Wires", meta = (EditCondition = "ConnectionDrawType == ENarrativeConnectionDrawType::Circuit"))
	FVector2D CircuitConnectionSpacing;

	UPROPERTY(EditAnywhere, config, Category = "Wires")
	FLinearColor InactiveWireColor;

	UPROPERTY(EditAnywhere, config, Category = "Wires", meta = (ClampMin = 0.0f))
	float InactiveWireThickness;

	UPROPERTY(EditAnywhere, config, Category = "Wires", meta = (ClampMin = 1.0f))
	float RecentWireDuration;

	/** The color to display execution wires that were just executed */
	UPROPERTY(EditAnywhere, config, Category = "Wires")
	FLinearColor RecentWireColor;

	UPROPERTY(EditAnywhere, config, Category = "Wires", meta = (ClampMin = 0.0f))
	float RecentWireThickness;

	UPROPERTY(EditAnywhere, config, Category = "Wires")
	FLinearColor RecordedWireColor;

	UPROPERTY(EditAnywhere, config, Category = "Wires", meta = (ClampMin = 0.0f))
	float RecordedWireThickness;

	UPROPERTY(EditAnywhere, config, Category = "Wires")
	FLinearColor SelectedWireColor;

	UPROPERTY(EditAnywhere, config, Category = "Wires", meta = (ClampMin = 0.0f))
	float SelectedWireThickness;

public:
	virtual FName GetCategoryName() const override { return FName("Narrative Graph"); }
	virtual FText GetSectionText() const override { return INVTEXT("Graph Settings"); }
};
