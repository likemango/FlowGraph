// Copyright XiaoYao

#include "Nodes/Utils/NarrativeNode_Log.h"
#include "NarrativeLogChannels.h"

#include "Engine/Engine.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativeNode_Log)

UNarrativeNode_Log::UNarrativeNode_Log(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, Message(TEXT("Log!"))
	, Verbosity(ENarrativeLogVerbosity::Warning)
	, bPrintToScreen(true)
	, Duration(5.0f)
	, TextColor(FColor::Yellow)
{
#if WITH_EDITOR
	Category = TEXT("Utils");
#endif
}

void UNarrativeNode_Log::ExecuteInput(const FName& PinName)
{
	switch (Verbosity)
	{
		case ENarrativeLogVerbosity::Error:
			UE_LOG(LogNarrative, Error, TEXT("%s"), *Message);
			break;
		case ENarrativeLogVerbosity::Warning:
			UE_LOG(LogNarrative, Warning, TEXT("%s"), *Message);
			break;
		case ENarrativeLogVerbosity::Display:
			UE_LOG(LogNarrative, Display, TEXT("%s"), *Message);
			break;
		case ENarrativeLogVerbosity::Log:
			UE_LOG(LogNarrative, Log, TEXT("%s"), *Message);
			break;
		case ENarrativeLogVerbosity::Verbose:
			UE_LOG(LogNarrative, Verbose, TEXT("%s"), *Message);
			break;
		case ENarrativeLogVerbosity::VeryVerbose:
			UE_LOG(LogNarrative, VeryVerbose, TEXT("%s"), *Message);
			break;
		default: ;
	}

	if (bPrintToScreen)
	{
		GEngine->AddOnScreenDebugMessage(-1, Duration, TextColor, Message);
	}

	TriggerFirstOutput(true);
}

#if WITH_EDITOR
FString UNarrativeNode_Log::GetNodeDescription() const
{
	return Message;
}
#endif
