// Copyright XiaoYao

#include "Pins/SNarrativeInputPinHandle.h"
#include "Nodes/NarrativeNode.h"
#include "Pins/SNarrativePinHandle.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"

void SNarrativeInputPinHandle::RefreshNameList()
{
	PinNames.Empty();

	if (Blueprint && Blueprint->GeneratedClass)
	{
		if (UNarrativeNode* NarrativeNode = Blueprint->GeneratedClass->GetDefaultObject<UNarrativeNode>())
		{
			for (const FNarrativePin& InputPin : NarrativeNode->InputPins)
			{
				PinNames.Add(MakeShareable(new FName(InputPin.PinName)));
			}
		}
	}
}

TSharedPtr<SGraphPin> FNarrativeInputPinHandleFactory::CreatePin(UEdGraphPin* InPin) const
{
	if (InPin->PinType.PinCategory == GetDefault<UEdGraphSchema_K2>()->PC_Struct && InPin->PinType.PinSubCategoryObject == FNarrativeInputPinHandle::StaticStruct() && InPin->LinkedTo.Num() == 0)
	{
		if (const UEdGraphNode* GraphNode = InPin->GetOuter())
		{
			if (const UBlueprint* Blueprint = GraphNode->GetGraph()->GetTypedOuter<UBlueprint>())
			{
				return SNew(SNarrativeInputPinHandle, InPin, Blueprint);
			}
		}
	}

	return nullptr;
}
