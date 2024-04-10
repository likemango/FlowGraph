// Copyright XiaoYao

#include "DetailCustomizations/NarrativeNode_SubGraphDetails.h"

#include "DetailLayoutBuilder.h"
#include "NarrativeAsset.h"
#include "Nodes/Route/NarrativeNode_SubGraph.h"

void FNarrativeNode_SubGraphDetails::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	TArray<TWeakObjectPtr<UObject>> ObjectsBeingEdited;
	DetailLayout.GetObjectsBeingCustomized(ObjectsBeingEdited);

	if (ObjectsBeingEdited[0].IsValid())
	{
		const UNarrativeNode_SubGraph* SubGraphNode = CastChecked<UNarrativeNode_SubGraph>(ObjectsBeingEdited[0]);

		// Generate the list of asset classes allowed or disallowed
		TArray<UClass*> NarrativeAssetClasses;
		GetDerivedClasses(UNarrativeAsset::StaticClass(), NarrativeAssetClasses, true);
		NarrativeAssetClasses.Add(UNarrativeAsset::StaticClass());

		TArray<UClass*> DisallowedClasses;
		TArray<UClass*> AllowedClasses;
		for (auto NarrativeAssetClass : NarrativeAssetClasses)
		{
			if (const UNarrativeAsset* DefaultAsset = Cast<UNarrativeAsset>(NarrativeAssetClass->GetDefaultObject()))
			{
				if (DefaultAsset->DeniedInSubgraphNodeClasses.Contains(SubGraphNode->GetClass()))
				{
					DisallowedClasses.Add(NarrativeAssetClass);
				}
	
				if (DefaultAsset->AllowedInSubgraphNodeClasses.Contains(SubGraphNode->GetClass()))
				{
					AllowedClasses.Add(NarrativeAssetClass);
				}
			}
		}
	
		DisallowedClasses.Append(SubGraphNode->DeniedAssignedAssetClasses);
	
		for (auto NarrativeAssetClass : SubGraphNode->AllowedAssignedAssetClasses)
		{
			if (!DisallowedClasses.Contains(NarrativeAssetClass))
			{
				AllowedClasses.AddUnique(NarrativeAssetClass);
			}
		}
	
		FString AllowedClassesString;
		for (UClass* Class : AllowedClasses)
		{
			AllowedClassesString.Append(FString::Printf(TEXT("%s,"), *Class->GetClassPathName().ToString()));
		}
	
		FString DisallowedClassesString;
		for (UClass* Class : DisallowedClasses)
		{
			DisallowedClassesString.Append(FString::Printf(TEXT("%s,"), *Class->GetClassPathName().ToString()));
		}
	
		const auto AssetProperty = DetailLayout.GetProperty(FName("Asset"), UNarrativeNode_SubGraph::StaticClass());
		if (FProperty* MetaDataProperty = AssetProperty->GetMetaDataProperty())
		{
			MetaDataProperty->SetMetaData(TEXT("AllowedClasses"), *AllowedClassesString);
			MetaDataProperty->SetMetaData(TEXT("DisallowedClasses"), *DisallowedClassesString);
		}
	}
}
