// Copyright XiaoYao

#include "Nodes/Utils/NarrativeNode_Checkpoint.h"
#include "NarrativeSubsystem.h"

#include "Kismet/GameplayStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativeNode_Checkpoint)

UNarrativeNode_Checkpoint::UNarrativeNode_Checkpoint(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Utils");
#endif
}

void UNarrativeNode_Checkpoint::ExecuteInput(const FName& PinName)
{
	if (GetNarrativeSubsystem())
	{
		UNarrativeSaveGame* NewSaveGame = Cast<UNarrativeSaveGame>(UGameplayStatics::CreateSaveGameObject(UNarrativeSaveGame::StaticClass()));
		GetNarrativeSubsystem()->OnGameSaved(NewSaveGame);

		UGameplayStatics::SaveGameToSlot(NewSaveGame, NewSaveGame->SaveSlotName, 0);
	}

	TriggerFirstOutput(true);
}

void UNarrativeNode_Checkpoint::OnLoad_Implementation()
{
	TriggerFirstOutput(true);
}
