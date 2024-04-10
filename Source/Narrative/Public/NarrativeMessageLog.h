// Copyright XiaoYao

#pragma once

#if WITH_EDITOR
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "Logging/TokenizedMessage.h"
#include "Misc/UObjectToken.h"

class UNarrativeAsset;
class UNarrativeNode;

/**
 * Message Log token that links to an element in Narrative Graph
 */
class NARRATIVE_API FNarrativeGraphToken : public IMessageToken
{
private:
	const TWeakObjectPtr<UEdGraphNode> GraphNode;
	const FEdGraphPinReference GraphPin;

	explicit FNarrativeGraphToken(const UNarrativeAsset* InNarrativeAsset);
	explicit FNarrativeGraphToken(const UNarrativeNode* InNarrativeNode);
	explicit FNarrativeGraphToken(UEdGraphNode* InGraphNode, const UEdGraphPin* InPin);

public:
	/** Factory method, tokens can only be constructed as shared refs */
	static TSharedPtr<IMessageToken> Create(const UNarrativeAsset* InNarrativeAsset, FTokenizedMessage& Message);
	static TSharedPtr<IMessageToken> Create(const UNarrativeNode* InNarrativeNode, FTokenizedMessage& Message);
	static TSharedPtr<IMessageToken> Create(UEdGraphNode* InGraphNode, FTokenizedMessage& Message);
	static TSharedPtr<IMessageToken> Create(const UEdGraphPin* InPin, FTokenizedMessage& Message);

	const UEdGraphNode* GetGraphNode() const { return GraphNode.Get(); }
	const UEdGraphPin* GetPin() const { return GraphPin.Get(); }

	// IMessageToken
	virtual EMessageToken::Type GetType() const override
	{
		return EMessageToken::EdGraph;
	}
};

/**
 * List of Message Log lines
 */
class NARRATIVE_API FNarrativeMessageLog
{
public:
	static const FName LogName;
	TArray<TSharedRef<FTokenizedMessage>> Messages;

public:
	FNarrativeMessageLog()
	{
	}

	template <typename T>
	TSharedRef<FTokenizedMessage> Error(const TCHAR* Format, T* Object)
	{
		TSharedRef<FTokenizedMessage> Message = FTokenizedMessage::Create(EMessageSeverity::Error);
		AddMessage<T>(NAME_None, Format, Message, Object);
		return Message;
	}

	template <typename T>
	TSharedRef<FTokenizedMessage> Warning(const TCHAR* Format, T* Object)
	{
		TSharedRef<FTokenizedMessage> Message = FTokenizedMessage::Create(EMessageSeverity::Warning);
		AddMessage<T>(NAME_None, Format, Message, Object);
		return Message;
	}

	template <typename T>
	TSharedRef<FTokenizedMessage> Note(const TCHAR* Format, T* Object)
	{
		TSharedRef<FTokenizedMessage> Message = FTokenizedMessage::Create(EMessageSeverity::Info);
		AddMessage<T>(NAME_None, Format, Message, Object);
		return Message;
	}

protected:
	template <typename T>
	void AddMessage(FName MessageID, const TCHAR* Format, TSharedRef<FTokenizedMessage>& Message, T* Object)
	{
		Message->SetIdentifier(MessageID);

		if (Object)
		{
			if (const TSharedPtr<IMessageToken> Token = FNarrativeGraphToken::Create(Object, Message.Get()))
			{
				Message->SetMessageLink(FUObjectToken::Create(Object));
			}
		}

		Message.Get().AddToken(FTextToken::Create(FText::FromString(Format)));
		Messages.Add(Message);
	}
};

#endif // WITH_EDITOR
