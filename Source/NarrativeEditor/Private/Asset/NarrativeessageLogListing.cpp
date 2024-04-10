// Copyright XiaoYao

#include "Asset/NarrativeMessageLogListing.h"

#include "MessageLogModule.h"
#include "Modules/ModuleManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativeMessageLogListing)

#define LOCTEXT_NAMESPACE "NarrativeMessageLogListing"

FNarrativeMessageLogListing::FNarrativeMessageLogListing(const UNarrativeAsset* InNarrativeAsset, const ENarrativeLogType Type)
	: Log(RegisterLogListing(InNarrativeAsset, Type))
{
}

FNarrativeMessageLogListing::~FNarrativeMessageLogListing()
{
	// Unregister the log so it will be ref-counted to zero if it has no messages
	if (Log->NumMessages(EMessageSeverity::Info) == 0)
	{
		FMessageLogModule& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
		MessageLogModule.UnregisterLogListing(Log->GetName());
	}
}

TSharedRef<IMessageLogListing> FNarrativeMessageLogListing::RegisterLogListing(const UNarrativeAsset* InNarrativeAsset, const ENarrativeLogType Type)
{
	FMessageLogModule& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");

	const FName LogName = GetListingName(InNarrativeAsset, Type);

	// Register the log (this will return an existing log if it has been used before)
	FMessageLogInitializationOptions LogInitOptions;
	LogInitOptions.bShowInLogWindow = false;
	MessageLogModule.RegisterLogListing(LogName, LOCTEXT("NarrativeGraphLogLabel", "NarrativeGraph"), LogInitOptions);
	return MessageLogModule.GetLogListing(LogName);
}

TSharedRef<IMessageLogListing> FNarrativeMessageLogListing::GetLogListing(const UNarrativeAsset* InNarrativeAsset, const ENarrativeLogType Type)
{
	FMessageLogModule& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
	const FName LogName = GetListingName(InNarrativeAsset, Type);

	// Create a new message log
	if (!MessageLogModule.IsRegisteredLogListing(LogName))
	{
		MessageLogModule.RegisterLogListing(LogName, FText::FromString(GetLogLabel(Type)));
	}

	return MessageLogModule.GetLogListing(LogName);
}

FString FNarrativeMessageLogListing::GetLogLabel(const ENarrativeLogType Type)
{
	const FString TypeAsString = StaticEnum<ENarrativeLogType>()->GetNameStringByIndex(static_cast<int32>(Type));
	return FString::Printf(TEXT("Narrative%sLog"), *TypeAsString);
}

FName FNarrativeMessageLogListing::GetListingName(const UNarrativeAsset* InNarrativeAsset, const ENarrativeLogType Type)
{
	FName LogListingName;
	if (InNarrativeAsset)
	{
		LogListingName = *FString::Printf(TEXT("%s::%s::%s"), *GetLogLabel(Type), *InNarrativeAsset->GetName(), *InNarrativeAsset->AssetGuid.ToString());
	}
	else
	{
		LogListingName = "NarrativeGraph";
	}
	return LogListingName;
}

#undef LOCTEXT_NAMESPACE
