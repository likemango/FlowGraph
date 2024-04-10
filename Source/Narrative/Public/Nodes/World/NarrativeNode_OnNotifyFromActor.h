// Copyright XiaoYao

#pragma once

#include "Nodes/World/NarrativeNode_ComponentObserver.h"
#include "NarrativeNode_OnNotifyFromActor.generated.h"

/**
 * Triggers output when Narrative Component with matching Identity Tag calls NotifyGraph function with matching Notify Tag
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "On Notify From Actor"))
class NARRATIVE_API UNarrativeNode_OnNotifyFromActor : public UNarrativeNode_ComponentObserver
{
	GENERATED_UCLASS_BODY()

protected:
	UPROPERTY(EditAnywhere, Category = "Notify")
	FGameplayTagContainer NotifyTags;

	// If true, node will check given Notify Tag is present in the Recently Sent Notify Tags
	// This might be helpful in multiplayer, if client-side Narrative Node started work after server sent the notify
	UPROPERTY(EditAnywhere, Category = "Notify")
	bool bRetroactive;

	virtual void ObserveActor(TWeakObjectPtr<AActor> Actor, TWeakObjectPtr<UNarrativeComponent> Component) override;
	virtual void ForgetActor(TWeakObjectPtr<AActor> Actor, TWeakObjectPtr<UNarrativeComponent> Component) override;

	virtual void OnNotifyFromComponent(UNarrativeComponent* Component, const FGameplayTag& Tag);
	
#if WITH_EDITOR
public:
	virtual FString GetNodeDescription() const override;
#endif
};
