// Copyright XiaoYao

#include "Asset/NarrativeAssetIndexer.h"

#include "NarrativeAsset.h"
#include "Nodes/NarrativeNode.h"

#include "Graph/Nodes/NarrativeGraphNode_Reroute.h"

#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphNode_Comment.h"
#include "Internationalization/Text.h"
#include "SearchSerializer.h"
#include "Utility/IndexerUtilities.h"

#define LOCTEXT_NAMESPACE "FNarrativeAssetIndexer"

enum class ENarrativeAssetIndexerVersion
{
	Empty,
	Initial,

	// -----<new versions can be added above this line>-------------------------------------------------
	VersionPlusOne,
	LatestVersion = VersionPlusOne - 1
};

int32 FNarrativeAssetIndexer::GetVersion() const
{
	return static_cast<int32>(ENarrativeAssetIndexerVersion::LatestVersion);
}

void FNarrativeAssetIndexer::IndexAsset(const UObject* InAssetObject, FSearchSerializer& Serializer) const
{
	const UNarrativeAsset* NarrativeAsset = CastChecked<UNarrativeAsset>(InAssetObject);

	{
		Serializer.BeginIndexingObject(NarrativeAsset, TEXT("$self"));

		FIndexerUtilities::IterateIndexableProperties(NarrativeAsset, [&Serializer](const FProperty* Property, const FString& Value)
		{
			Serializer.IndexProperty(Property, Value);
		});

		Serializer.EndIndexingObject();
	}

	IndexGraph(NarrativeAsset, Serializer);
}

void FNarrativeAssetIndexer::IndexGraph(const UNarrativeAsset* InNarrativeAsset, FSearchSerializer& Serializer) const
{
	for (UEdGraphNode* Node : InNarrativeAsset->GetGraph()->Nodes)
	{
		// Ignore Reroutes
		if (Cast<UNarrativeGraphNode_Reroute>(Node))
		{
			continue;
		}

		// Special rules for comment nodes
		if (Cast<UEdGraphNode_Comment>(Node))
		{
			Serializer.BeginIndexingObject(Node, Node->NodeComment);
			Serializer.IndexProperty(TEXT("Comment"), Node->NodeComment);
			Serializer.EndIndexingObject();
			continue;
		}

		// Indexing UEdGraphNode
		{
			const FText NodeText = Node->GetNodeTitle(ENodeTitleType::MenuTitle);
			Serializer.BeginIndexingObject(Node, NodeText);
			Serializer.IndexProperty(TEXT("Title"), NodeText);

			if (!Node->NodeComment.IsEmpty())
			{
				Serializer.IndexProperty(TEXT("Comment"), Node->NodeComment);
			}

			for (const UEdGraphPin* Pin : Node->GetAllPins())
			{
				if (Pin->Direction == EGPD_Input && Pin->LinkedTo.Num() == 0)
				{
					const FText PinText = Pin->GetDisplayName();
					if (PinText.IsEmpty())
					{
						continue;
					}

					const FText PinValue = Pin->GetDefaultAsText();
					if (PinValue.IsEmpty())
					{
						continue;
					}

					const FString PinLabel = TEXT("[Pin] ") + *FTextInspector::GetSourceString(PinText);
					Serializer.IndexProperty(PinLabel, PinValue);
				}
			}

			// This will serialize any user exposed options for the node that are editable in the Details
			FIndexerUtilities::IterateIndexableProperties(Node, [&Serializer](const FProperty* Property, const FString& Value)
			{
				Serializer.IndexProperty(Property, Value);
			});

			Serializer.EndIndexingObject();
		}

		// Indexing Narrative Node
		if (const UNarrativeGraphNode* NarrativeGraphNode = Cast<UNarrativeGraphNode>(Node))
		{
			if (const UNarrativeNode* NarrativeNode = NarrativeGraphNode->GetNarrativeNode())
			{
				const FString NodeFriendlyName = FString::Printf(TEXT("%s: %s"), *NarrativeNode->GetClass()->GetName(), *NarrativeNode->GetNodeDescription());
				Serializer.BeginIndexingObject(NarrativeNode, NodeFriendlyName);
				FIndexerUtilities::IterateIndexableProperties(NarrativeNode, [&Serializer](const FProperty* Property, const FString& Value)
				{
					Serializer.IndexProperty(Property, Value);
				});
				Serializer.EndIndexingObject();
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
