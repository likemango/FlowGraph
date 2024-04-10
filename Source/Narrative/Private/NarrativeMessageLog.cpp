// Copyright XiaoYao

#include "NarrativeMessageLog.h"

#if WITH_EDITOR
#include "Nodes/NarrativeNode.h"
#include "NarrativeAsset.h"

#define LOCTEXT_NAMESPACE "NarrativeMessageLog"

const FName FNarrativeMessageLog::LogName(TEXT("NarrativeGraph"));

FNarrativeGraphToken::FNarrativeGraphToken(const UNarrativeAsset* InNarrativeAsset)
{
	CachedText = FText::FromString(InNarrativeAsset->GetClass()->GetPathName());
}

FNarrativeGraphToken::FNarrativeGraphToken(const UNarrativeNode* InNarrativeNode)
	: GraphNode(InNarrativeNode->GetGraphNode())
{
	CachedText = InNarrativeNode->GetNodeTitle();
}

FNarrativeGraphToken::FNarrativeGraphToken(UEdGraphNode* InGraphNode, const UEdGraphPin* InPin)
	: GraphNode(InGraphNode)
	, GraphPin(InPin)
{
	if (InPin)
	{
		CachedText = InPin->GetDisplayName();
		if (CachedText.IsEmpty())
		{
			CachedText = LOCTEXT("UnnamedPin", "<Unnamed Pin>");
		}
	}
	else
	{
		CachedText = GraphNode->GetNodeTitle(ENodeTitleType::ListView);
	}
}

TSharedPtr<IMessageToken> FNarrativeGraphToken::Create(const UNarrativeAsset* InNarrativeAsset, FTokenizedMessage& Message)
{
	if (InNarrativeAsset)
	{
		Message.AddToken(MakeShareable(new FNarrativeGraphToken(InNarrativeAsset)));
		return Message.GetMessageTokens().Last();
	}

	return nullptr;
}

TSharedPtr<IMessageToken> FNarrativeGraphToken::Create(const UNarrativeNode* InNarrativeNode, FTokenizedMessage& Message)
{
	if (InNarrativeNode)
	{
		Message.AddToken(MakeShareable(new FNarrativeGraphToken(InNarrativeNode)));
		return Message.GetMessageTokens().Last();
	}
	
	return nullptr;
}

TSharedPtr<IMessageToken> FNarrativeGraphToken::Create(UEdGraphNode* InGraphNode, FTokenizedMessage& Message)
{
	if (InGraphNode)
	{
		Message.AddToken(MakeShareable(new FNarrativeGraphToken(InGraphNode, nullptr)));
		return Message.GetMessageTokens().Last();
	}

	return nullptr;
}

TSharedPtr<IMessageToken> FNarrativeGraphToken::Create(const UEdGraphPin* InPin, FTokenizedMessage& Message)
{
	if (InPin && InPin->GetOwningNode())
	{
		Message.AddToken(MakeShareable(new FNarrativeGraphToken(InPin->GetOwningNode(), InPin)));
		return Message.GetMessageTokens().Last();
	}
	
	return nullptr;
}

#undef LOCTEXT_NAMESPACE

#endif // WITH_EDITOR
