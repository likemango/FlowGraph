// Copyright XiaoYao

#include "Asset/NarrativeImportUtils.h"

#include "Asset/NarrativeAssetFactory.h"
#include "NarrativeEditorDefines.h"
#include "NarrativeEditorLogChannels.h"
#include "Graph/NarrativeGraphSchema_Actions.h"
#include "Graph/NarrativeGraph.h"

#include "NarrativeAsset.h"
#include "Nodes/NarrativePin.h"
#include "Nodes/Route/NarrativeNode_Start.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "EdGraphSchema_K2.h"
#include "EditorAssetLibrary.h"
#include "Misc/ScopedSlowTask.h"

#if ENABLE_ASYNC_NODES_IMPORT
#include "K2Node_BaseAsyncTask.h"
#endif
#include "K2Node_CallFunction.h"
#include "K2Node_Event.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_Knot.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativeImportUtils)

#define LOCTEXT_NAMESPACE "NarrativeImportUtils"

TMap<FName, TSubclassOf<UNarrativeNode>> UNarrativeImportUtils::FunctionsToNarrativeNodes = TMap<FName, TSubclassOf<UNarrativeNode>>();
TMap<TSubclassOf<UNarrativeNode>, FBlueprintToNarrativePinName> UNarrativeImportUtils::PinMappings = TMap<TSubclassOf<UNarrativeNode>, FBlueprintToNarrativePinName>();

UNarrativeAsset* UNarrativeImportUtils::ImportBlueprintGraph(UObject* BlueprintAsset, const TSubclassOf<UNarrativeAsset> NarrativeAssetClass, const FString NarrativeAssetName,
													const TMap<FName, TSubclassOf<UNarrativeNode>> InFunctionsToNarrativeNodes, const TMap<TSubclassOf<UNarrativeNode>, FBlueprintToNarrativePinName> InPinMappings, const FName StartEventName)
{
	if (BlueprintAsset == nullptr || NarrativeAssetClass == nullptr || NarrativeAssetName.IsEmpty() || StartEventName.IsNone())
	{
		return nullptr;
	}

	UBlueprint* Blueprint = Cast<UBlueprint>(BlueprintAsset);
	UNarrativeAsset* NarrativeAsset = nullptr;

	// we assume that users want to have a converted asset in the same folder as the legacy blueprint
	const FString PackageFolder = FPaths::GetPath(Blueprint->GetOuter()->GetPathName());

	if (!FPackageName::DoesPackageExist(PackageFolder / NarrativeAssetName, nullptr)) // create a new asset
	{
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		UFactory* Factory = Cast<UFactory>(UNarrativeAssetFactory::StaticClass()->GetDefaultObject());

		if (UObject* NewAsset = AssetTools.CreateAsset(NarrativeAssetName, PackageFolder, NarrativeAssetClass, Factory))
		{
			NarrativeAsset = Cast<UNarrativeAsset>(NewAsset);
		}
	}
	else // load existing asset
	{
		const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

		const FString PackageName = PackageFolder / (NarrativeAssetName + TEXT(".") + NarrativeAssetName);
		const FAssetData& FoundAssetData = AssetRegistryModule.GetRegistry().GetAssetByObjectPath(FSoftObjectPath(PackageName));

		NarrativeAsset = Cast<UNarrativeAsset>(FoundAssetData.GetAsset());
	}

	// import graph
	if (NarrativeAsset)
	{
		FunctionsToNarrativeNodes = InFunctionsToNarrativeNodes;
		PinMappings = InPinMappings;

		ImportBlueprintGraph(Blueprint, NarrativeAsset, StartEventName);
		FunctionsToNarrativeNodes.Empty();
		PinMappings.Empty();

		Cast<UNarrativeGraph>(NarrativeAsset->GetGraph())->RefreshGraph();
		UEditorAssetLibrary::SaveLoadedAsset(NarrativeAsset->GetPackage());
	}

	return NarrativeAsset;
}

void UNarrativeImportUtils::ImportBlueprintGraph(UBlueprint* Blueprint, UNarrativeAsset* NarrativeAsset, const FName StartEventName)
{
	ensureAlways(Blueprint && NarrativeAsset);

	UEdGraph* BlueprintGraph = Blueprint->UbergraphPages.IsValidIndex(0) ? Blueprint->UbergraphPages[0] : nullptr;
	if (BlueprintGraph == nullptr)
	{
		return;
	}

	FScopedSlowTask ExecuteAssetTask(BlueprintGraph->Nodes.Num(), FText::Format(LOCTEXT("FNarrativeGraphUtils::ImportBlueprintGraph", "Reading {0}"), FText::FromString(Blueprint->GetFriendlyName())));
	ExecuteAssetTask.MakeDialog();

	TMap<FGuid, FImportedGraphNode> SourceNodes;
	UEdGraphNode* StartNode = nullptr;

	for (UEdGraphNode* ThisNode : BlueprintGraph->Nodes)
	{
		ExecuteAssetTask.EnterProgressFrame(1, FText::Format(LOCTEXT("FNarrativeGraphUtils::ImportBlueprintGraph", "Processing blueprint node: {0}"), ThisNode->GetNodeTitle(ENodeTitleType::ListView)));

		// non-pure K2Nodes or UK2Node_Knot
		const UK2Node* K2Node = Cast<UK2Node>(ThisNode);
		if (K2Node && (!K2Node->IsNodePure() || Cast<UK2Node_Knot>(K2Node)))
		{
			FImportedGraphNode& NodeImport = SourceNodes.FindOrAdd(ThisNode->NodeGuid);
			NodeImport.SourceGraphNode = ThisNode;

			// create map of all non-pure blueprint nodes with theirs pin connections
			for (const UEdGraphPin* ThisPin : ThisNode->Pins)
			{
				for (const UEdGraphPin* LinkedPin : ThisPin->LinkedTo)
				{
					if (LinkedPin && LinkedPin->GetOwningNode())
					{
						const FConnectedPin ConnectedPin(LinkedPin->GetOwningNode()->NodeGuid, LinkedPin->PinName);

						if (ThisPin->Direction == EGPD_Input)
						{
							NodeImport.Incoming.Add(ThisPin->PinName, ConnectedPin);
						}
						else
						{
							NodeImport.Outgoing.Add(ThisPin->PinName, ConnectedPin);
						}
					}
				}
			}

			// we need to know the default entry point of blueprint graph
			const UK2Node_Event* EventNode = Cast<UK2Node_Event>(ThisNode);
			if (EventNode && (EventNode->EventReference.GetMemberName() == StartEventName || EventNode->CustomFunctionName == StartEventName))
			{
				StartNode = ThisNode;
			}
		}
	}

	// can't start import if provided graph doesn't have required start node
	if (StartNode == nullptr)
	{
		return;
	}

	// clear existing graph
	UNarrativeGraph* NarrativeGraph = Cast<UNarrativeGraph>(NarrativeAsset->GetGraph());
	for (const TPair<FGuid, UNarrativeNode*>& Node : NarrativeAsset->GetNodes())
	{
		if (UNarrativeGraphNode* NarrativeGraphNode = Cast<UNarrativeGraphNode>(Node.Value->GetGraphNode()))
		{
			NarrativeGraph->GetSchema()->BreakNodeLinks(*NarrativeGraphNode);
			NarrativeGraphNode->DestroyNode();
		}

		NarrativeAsset->UnregisterNode(Node.Key);
	}

	TMap<FGuid, UNarrativeGraphNode*> TargetNodes;

	// recreated UNarrativeNode_Start, assign it a blueprint node FGuid
	UNarrativeGraphNode* StartGraphNode = FNarrativeGraphSchemaAction_NewNode::CreateNode(NarrativeGraph, nullptr, UNarrativeNode_Start::StaticClass(), FVector2D::ZeroVector);
	NarrativeGraph->GetSchema()->SetNodeMetaData(StartGraphNode, FNodeMetadata::DefaultGraphNode);
	StartGraphNode->NodeGuid = StartNode->NodeGuid;
	StartGraphNode->GetNarrativeNode()->SetGuid(StartNode->NodeGuid);
	TargetNodes.Add(StartGraphNode->NodeGuid, StartGraphNode);

	// execute graph import
	// iterate all nodes separately, ensures we import all possible nodes and connect them together
	for (const TPair<FGuid, FImportedGraphNode>& SourceNode : SourceNodes)
	{
		ImportBlueprintFunction(NarrativeAsset, SourceNode.Value, SourceNodes, TargetNodes);
	}
}

void UNarrativeImportUtils::ImportBlueprintFunction(const UNarrativeAsset* NarrativeAsset, const FImportedGraphNode& NodeImport, const TMap<FGuid, FImportedGraphNode>& SourceNodes, TMap<FGuid, class UNarrativeGraphNode*>& TargetNodes)
{
	ensureAlways(NodeImport.SourceGraphNode);
	TSubclassOf<UNarrativeNode> MatchingNarrativeNodeClass = nullptr;

	// find NarrativeNode class matching provided UFunction name
	FName FunctionName = NAME_None;
	if (const UK2Node_CallFunction* FunctionNode = Cast<UK2Node_CallFunction>(NodeImport.SourceGraphNode))
	{
		FunctionName = FunctionNode->GetFunctionName();
	}
#if ENABLE_ASYNC_NODES_IMPORT
	else if (const UK2Node_BaseAsyncTask* AsyncTaskNode = Cast<UK2Node_BaseAsyncTask>(NodeImport.SourceGraphNode))
	{
		FunctionName = AsyncTaskNode->GetProxyFactoryFunctionName();
	}
#endif
	else if (Cast<UK2Node_Knot>(NodeImport.SourceGraphNode))
	{
		FunctionName = TEXT("Reroute");
	}
	else if (Cast<UK2Node_IfThenElse>(NodeImport.SourceGraphNode))
	{
		FunctionName = TEXT("Branch");
	}

	if (!FunctionName.IsNone())
	{
		// find NarrativeNode class matching provided UFunction name
		MatchingNarrativeNodeClass = FunctionsToNarrativeNodes.FindRef(FunctionName);
	}

	if (MatchingNarrativeNodeClass == nullptr)
	{
		UE_LOG(LogNarrativeEditor, Error, TEXT("Can't find Narrative Node class for K2Node, function name %s"), *FunctionName.ToString());
		return;
	}

	const FGuid& NodeGuid = NodeImport.SourceGraphNode->NodeGuid;

	// create a new Narrative Graph node
	const FVector2d Location = FVector2D(NodeImport.SourceGraphNode->NodePosX, NodeImport.SourceGraphNode->NodePosY);
	UNarrativeGraphNode* NarrativeGraphNode = FNarrativeGraphSchemaAction_NewNode::ImportNode(NarrativeAsset->GetGraph(), nullptr, MatchingNarrativeNodeClass, NodeGuid, Location);

	if (NarrativeGraphNode == nullptr)
	{
		return;
	}
	TargetNodes.Add(NodeGuid, NarrativeGraphNode);

	// transfer properties from UFunction input parameters to Narrative Node properties
	{
		TMap<const FName, const UEdGraphPin*> InputPins;
		GetValidInputPins(NodeImport.SourceGraphNode, InputPins);

		UClass* NarrativeNodeClass = NarrativeGraphNode->GetNarrativeNode()->GetClass();
		for (TFieldIterator<FProperty> PropIt(NarrativeNodeClass, EFieldIteratorFlags::IncludeSuper); PropIt && (PropIt->PropertyFlags & CPF_Edit); ++PropIt)
		{
			const FProperty* Param = *PropIt;
			const bool bIsEditable = !Param->HasAnyPropertyFlags(CPF_Deprecated);
			if (bIsEditable)
			{
				if (const UEdGraphPin* MatchingInputPin = FindPinMatchingToProperty(NarrativeNodeClass, Param, InputPins))
				{
					if (MatchingInputPin->LinkedTo.Num() == 0) // nothing connected to pin, so user can set value directly on this pin
					{
						FString const PinValue = MatchingInputPin->GetDefaultAsString();
						uint8* Offset = Param->ContainerPtrToValuePtr<uint8>(NarrativeGraphNode->GetNarrativeNode());
						Param->ImportText_Direct(*PinValue, Offset, NarrativeGraphNode->GetNarrativeNode(), PPF_Copy, GLog);
					}
				}
				else // try to find matching Pin in connected pure nodes
				{
					bool bPinFound = false;
					for (const TPair<const FName, const UEdGraphPin*> InputPin : InputPins)
					{
						for (const UEdGraphPin* LinkedPin : InputPin.Value->LinkedTo)
						{
							if (LinkedPin && LinkedPin->GetOwningNode()) // try to read value from the first pure node connected to the pin
							{
								// in theory, we could put this part in recursive loop, iterating pure nodes until we find one with matching Pin Name
								// in practice, iterating blueprint graph isn't that easy as might encounter Make/Break nodes, array builders
								// if someone is willing put work to it, you're welcome to make a pull request

								UK2Node* LinkedK2Node = Cast<UK2Node>(LinkedPin->GetOwningNode());
								if (LinkedK2Node && LinkedK2Node->IsNodePure())
								{
									TMap<const FName, const UEdGraphPin*> PureNodePins;
									GetValidInputPins(LinkedK2Node, PureNodePins);

									if (const UEdGraphPin* PureInputPin = FindPinMatchingToProperty(NarrativeNodeClass, Param, PureNodePins))
									{
										if (PureInputPin->LinkedTo.Num() == 0) // nothing connected to pin, so user can set value directly on this pin
										{
											FString const PinValue = PureInputPin->GetDefaultAsString();
											uint8* Offset = Param->ContainerPtrToValuePtr<uint8>(NarrativeGraphNode->GetNarrativeNode());
											Param->ImportText_Direct(*PinValue, Offset, NarrativeGraphNode->GetNarrativeNode(), PPF_Copy, GLog);

											bPinFound = true;
										}
									}
								}

								// there can be only single valid connection on input parameter pin
								break;
							}
						}

						if (bPinFound)
						{
							break;
						}
					}
				}
			}
		}
	}

	// Narrative Nodes with Context Pins needs to update related data and call OnReconstructionRequested.ExecuteIfBound() in order to fully construct a graph node
	NarrativeGraphNode->GetNarrativeNode()->PostImport();

	// connect new node to all already recreated nodes
	for (const TPair<FName, FConnectedPin>& Connection : NodeImport.Incoming)
	{
		UEdGraphPin* ThisPin = nullptr;
		for (UEdGraphPin* NarrativeInputPin : NarrativeGraphNode->InputPins)
		{
			if (NarrativeGraphNode->InputPins.Num() == 1 || Connection.Key == NarrativeInputPin->PinName)
			{
				ThisPin = NarrativeInputPin;
				break;
			}
		}
		if (ThisPin == nullptr)
		{
			continue;
		}

		UEdGraphPin* ConnectedPin = nullptr;
		if (UNarrativeGraphNode* ConnectedNode = TargetNodes.FindRef(Connection.Value.NodeGuid))
		{
			for (UEdGraphPin* NarrativeOutputPin : ConnectedNode->OutputPins)
			{
				if (ConnectedNode->OutputPins.Num() == 1 || Connection.Value.PinName == NarrativeOutputPin->PinName
					|| (Connection.Value.PinName == UEdGraphSchema_K2::PN_Then && NarrativeOutputPin->PinName == FName("TRUE"))
					|| (Connection.Value.PinName == UEdGraphSchema_K2::PN_Else && NarrativeOutputPin->PinName == FName("FALSE")))
				{
					ConnectedPin = NarrativeOutputPin;
					break;
				}
			}
		}

		// link the pin to existing node
		if (ConnectedPin)
		{
			NarrativeAsset->GetGraph()->GetSchema()->TryCreateConnection(ThisPin, ConnectedPin);
		}
	}
	for (const TPair<FName, FConnectedPin>& Connection : NodeImport.Outgoing)
	{
		UEdGraphPin* ThisPin = nullptr;
		for (UEdGraphPin* NarrativeOutputPin : NarrativeGraphNode->OutputPins)
		{
			if (NarrativeGraphNode->OutputPins.Num() == 1 || Connection.Key == NarrativeOutputPin->PinName
				|| (Connection.Key == UEdGraphSchema_K2::PN_Then && NarrativeOutputPin->PinName == FName("TRUE"))
				|| (Connection.Key == UEdGraphSchema_K2::PN_Else && NarrativeOutputPin->PinName == FName("FALSE")))
			{
				ThisPin = NarrativeOutputPin;
				break;
			}
		}
		if (ThisPin == nullptr)
		{
			continue;
		}

		UEdGraphPin* ConnectedPin = nullptr;
		if (UNarrativeGraphNode* ConnectedNode = TargetNodes.FindRef(Connection.Value.NodeGuid))
		{
			for (UEdGraphPin* NarrativeInputPin : ConnectedNode->InputPins)
			{
				if (ConnectedNode->InputPins.Num() == 1 || Connection.Value.PinName == NarrativeInputPin->PinName)
				{
					ConnectedPin = NarrativeInputPin;
					break;
				}
			}
		}

		// link the pin to existing node
		if (ConnectedPin)
		{
			NarrativeAsset->GetGraph()->GetSchema()->TryCreateConnection(ThisPin, ConnectedPin);
		}
	}
}

void UNarrativeImportUtils::GetValidInputPins(const UEdGraphNode* GraphNode, TMap<const FName, const UEdGraphPin*>& Result)
{
	for (const UEdGraphPin* Pin : GraphNode->Pins)
	{
		if (Pin->Direction == EGPD_Input && !Pin->bHidden && !Pin->bOrphanedPin)
		{
			Result.Add(Pin->PinName, Pin);
		}
	}
}

const UEdGraphPin* UNarrativeImportUtils::FindPinMatchingToProperty(UClass* NarrativeNodeClass, const FProperty* Property, const TMap<const FName, const UEdGraphPin*> Pins)
{
	const FName& PropertyAuthoredName = *Property->GetAuthoredName();

	// if Pin Name is exactly the same as Narrative Node property name
	if (const UEdGraphPin* Pin = Pins.FindRef(PropertyAuthoredName))
	{
		return Pin;
	}

	// if not, check if appropriate Pin Mapping has been provided
	if (const FBlueprintToNarrativePinName* PinMapping = PinMappings.Find(NarrativeNodeClass))
	{
		if (const FName* MappedPinName = PinMapping->NodePropertiesToFunctionPins.Find(PropertyAuthoredName))
		{
			if (const UEdGraphPin* Pin = Pins.FindRef(*MappedPinName))
			{
				return Pin;
			}
		}
	}

	return nullptr;
}

#undef LOCTEXT_NAMESPACE
