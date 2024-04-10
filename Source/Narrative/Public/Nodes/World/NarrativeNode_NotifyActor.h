// Copyright XiaoYao

#pragma once

#include "GameplayTagContainer.h"

#include "Nodes/NarrativeNode.h"
#include "NarrativeNode_NotifyActor.generated.h"

/**
 * Finds all Narrative Components with matching Identity Tag and calls ReceiveNotify event on these components
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Notify Actor", Keywords = "event"))
class NARRATIVE_API UNarrativeNode_NotifyActor : public UNarrativeNode
{
	GENERATED_UCLASS_BODY()

protected:
	UPROPERTY(EditAnywhere, Category = "Notify")
	FGameplayTagContainer IdentityTags;
	
	UPROPERTY(EditAnywhere, Category = "Notify")
	EGameplayContainerMatchType MatchType;
	/**
	 * If true, identity tags must be an exact match.
	 * Be careful, setting this to false may be very expensive, as the
	 * search cost is proportional to the number of registered Gameplay Tags!
	 */
	UPROPERTY(EditAnywhere, Category = "Notify")
	bool bExactMatch;
	
	UPROPERTY(EditAnywhere, Category = "Notify")
	FGameplayTagContainer NotifyTags;

	UPROPERTY(EditAnywhere, Category = "Notify")
	ENarrativeNetMode NetMode;

	virtual void ExecuteInput(const FName& PinName) override;

#if WITH_EDITOR
public:
	virtual FString GetNodeDescription() const override;
	virtual EDataValidationResult ValidateNode() override;
#endif
};
