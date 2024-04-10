// Copyright XiaoYao

#pragma once

#include "Nodes/NarrativeNode.h"
#include "NarrativeNode_Log.generated.h"

// Variant of ELogVerbosity
UENUM(BlueprintType)
enum class ENarrativeLogVerbosity : uint8
{
	Error		UMETA(ToolTip = "Prints a message to console (and log file)"),
	Warning		UMETA(ToolTip = "Prints a message to console (and log file)"),
	Display		UMETA(ToolTip = "Prints a message to console (and log file)"),
	Log			UMETA(ToolTip = "Prints a message to a log file (does not print to console)"),
	Verbose		UMETA(ToolTip = "Prints a verbose message to a log file (if Verbose logging is enabled for the given category, usually used for detailed logging)"),
	VeryVerbose	UMETA(ToolTip = "Prints a verbose message to a log file (if VeryVerbose logging is enabled, usually used for detailed logging that would otherwise spam output)"),
};

/**
 * Adds message to log
 * Optionally shows message on screen
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Log", Keywords = "print"))
class NARRATIVE_API UNarrativeNode_Log : public UNarrativeNode
{
	GENERATED_UCLASS_BODY()
	
private:
	UPROPERTY(EditAnywhere, Category = "Narrative")
	FString Message;

	UPROPERTY(EditAnywhere, Category = "Narrative")
	ENarrativeLogVerbosity Verbosity;

	UPROPERTY(EditAnywhere, Category = "Narrative")
	bool bPrintToScreen;

	UPROPERTY(EditAnywhere, Category = "Narrative", meta = (EditCondition = "bPrintToScreen"))
	float Duration;

	UPROPERTY(EditAnywhere, Category = "Narrative", meta = (EditCondition = "bPrintToScreen"))
	FColor TextColor;

protected:
	virtual void ExecuteInput(const FName& PinName) override;

#if WITH_EDITOR
public:
	virtual FString GetNodeDescription() const override;
#endif
};
