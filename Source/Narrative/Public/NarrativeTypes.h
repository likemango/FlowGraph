// Copyright XiaoYao

#pragma once

#include "GameplayTagContainer.h"
#include "NarrativeTypes.generated.h"

#if WITH_EDITORONLY_DATA
UENUM(BlueprintType)
enum class ENarrativeNodeStyle : uint8
{
	Condition,
	Default,
	InOut UMETA(Hidden),
	Latent,
	Logic,
	SubGraph UMETA(Hidden),
	Custom
};
#endif

UENUM(BlueprintType)
enum class ENarrativeNodeState : uint8
{
	NeverActivated,
	Active,
	Completed,
	Aborted
};

// Finish Policy value is read by Narrative Node
// Nodes have opportunity to terminate themselves differently if Narrative Graph has been aborted
// Example: Spawn node might despawn all actors if Narrative Graph is aborted, not completed
UENUM(BlueprintType)
enum class ENarrativeFinishPolicy : uint8
{
	Keep,
	Abort
};

UENUM(BlueprintType)
enum class ENarrativeSignalMode : uint8
{
	Enabled		UMETA(ToolTip = "Default state, node is fully executed."),
	Disabled	UMETA(ToolTip = "No logic executed, any Input Pin activation is ignored. Node instantly enters a deactivated state."),
	PassThrough UMETA(ToolTip = "Internal node logic not executed. All connected outputs are triggered, node finishes its work.")
};

UENUM(BlueprintType)
enum class ENarrativeNetMode : uint8
{
	Any					UMETA(ToolTip = "Any networking mode."),
	Authority			UMETA(ToolTip = "Executed on the server or in the single-player (standalone)."),
	ClientOnly			UMETA(ToolTip = "Executed locally, on the single client."),
	ServerOnly			UMETA(ToolTip = "Executed on the server."),
	SinglePlayerOnly	UMETA(ToolTip = "Executed only in the single player, not available in multiplayer.")
};

UENUM(BlueprintType)
enum class ENarrativeTagContainerMatchType : uint8
{
	HasAny				UMETA(ToolTip = "Check if container A contains ANY of the tags in the specified container B."),
	HasAnyExact			UMETA(ToolTip = "Check if container A contains ANY of the tags in the specified container B, only allowing exact matches."),
	HasAll				UMETA(ToolTip = "Check if container A contains ALL of the tags in the specified container B."),
	HasAllExact			UMETA(ToolTip = "Check if container A contains ALL of the tags in the specified container B, only allowing exact matches")
};

namespace NarrativeTypes
{
	FORCEINLINE_DEBUGGABLE bool HasMatchingTags(const FGameplayTagContainer& Container, const FGameplayTagContainer& OtherContainer, const ENarrativeTagContainerMatchType MatchType)
	{
		switch (MatchType)
		{
			case ENarrativeTagContainerMatchType::HasAny:
				return Container.HasAny(OtherContainer);
			case ENarrativeTagContainerMatchType::HasAnyExact:
				return Container.HasAnyExact(OtherContainer);
			case ENarrativeTagContainerMatchType::HasAll:
				return Container.HasAll(OtherContainer);
			case ENarrativeTagContainerMatchType::HasAllExact:
				return Container.HasAllExact(OtherContainer);
			default:
				return false;
		}
	}
}

UENUM(BlueprintType)
enum class ENarrativeOnScreenMessageType : uint8
{
	Temporary,
	Permanent
};
