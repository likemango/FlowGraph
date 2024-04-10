// Copyright XiaoYao

#include "Pins/SNarrativeOutputPinHandle.h"
#include "Nodes/NarrativeNode.h"
#include "Pins/SNarrativePinHandle.h"

#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"

void SNarrativeOutputPinHandle::RefreshNameList()
{
	PinNames.Empty();

	if (Blueprint && Blueprint->GeneratedClass)
	{
		if (UNarrativeNode* NarrativeNode = Blueprint->GeneratedClass->GetDefaultObject<UNarrativeNode>())
		{
			for (const FNarrativePin& OutputPin : NarrativeNode->OutputPins)
			{
				PinNames.Add(MakeShareable(new FName(OutputPin.PinName)));
			}
		}
	}
}

TSharedPtr<SGraphPin> FNarrativeOutputPinHandleFactory::CreatePin(UEdGraphPin* InPin) const
{
	if (InPin->PinType.PinCategory == GetDefault<UEdGraphSchema_K2>()->PC_Struct && InPin->PinType.PinSubCategoryObject == FNarrativeOutputPinHandle::StaticStruct() && InPin->LinkedTo.Num() == 0)
	{
		if (const UEdGraphNode* GraphNode = InPin->GetOuter())
		{
			if (const UBlueprint* Blueprint = GraphNode->GetGraph()->GetTypedOuter<UBlueprint>())
			{
				return SNew(SNarrativeOutputPinHandle, InPin, Blueprint);
			}
		}
	}

	return nullptr;
}
