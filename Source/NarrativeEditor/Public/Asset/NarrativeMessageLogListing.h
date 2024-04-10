// Copyright XiaoYao

#pragma once

#include "IMessageLogListing.h"

#include "NarrativeAsset.h"

UENUM()
enum class ENarrativeLogType : uint8
{
	Runtime,
	Validation
};

/**
 * Scope wrapper for the message log. Ensures we don't leak logs that we dont need (i.e. those that have no messages)
 * Replicated after FScopedBlueprintMessageLog
 */
class NARRATIVEEDITOR_API FNarrativeMessageLogListing
{
public:
	FNarrativeMessageLogListing(const UNarrativeAsset* InNarrativeAsset, const ENarrativeLogType Type);
	~FNarrativeMessageLogListing();
	
public:
	TSharedRef<IMessageLogListing> Log;
	FName LogName;

private:
	static TSharedRef<IMessageLogListing> RegisterLogListing(const UNarrativeAsset* InNarrativeAsset, const ENarrativeLogType Type);
	static FName GetListingName(const UNarrativeAsset* InNarrativeAsset, const ENarrativeLogType Type);

public:
	static TSharedRef<IMessageLogListing> GetLogListing(const UNarrativeAsset* InNarrativeAsset, const ENarrativeLogType Type);
	static FString GetLogLabel(const ENarrativeLogType Type);
};
