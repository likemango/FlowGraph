// Copyright XiaoYao

#include "Utils/SLevelEditorNarrative.h"
#include "NarrativeAsset.h"
#include "NarrativeComponent.h"

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 3
#include "NarrativeWorldSettings.h"
#endif

#include "Graph/NarrativeGraphSettings.h"

#include "Editor.h"
#include "PropertyCustomizationHelpers.h"
#include "Runtime/Launch/Resources/Version.h"

#define LOCTEXT_NAMESPACE "SLevelEditorNarrative"

void SLevelEditorNarrative::Construct(const FArguments& InArgs)
{
	CreateNarrativeWidget();
	FEditorDelegates::OnMapOpened.AddRaw(this, &SLevelEditorNarrative::OnMapOpened);
}

void SLevelEditorNarrative::OnMapOpened(const FString& Filename, bool bAsTemplate)
{
	CreateNarrativeWidget();
}

void SLevelEditorNarrative::CreateNarrativeWidget()
{
	if (const UNarrativeComponent* NarrativeComponent = FindNarrativeComponent(); NarrativeComponent && NarrativeComponent->RootNarrative)
	{
		NarrativeAssetPath = NarrativeComponent->RootNarrative->GetPathName();
	}
	else
	{
		NarrativeAssetPath = FString();
	}

	ChildSlot
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SObjectPropertyEntryBox)
					.AllowedClass(UNarrativeGraphSettings::Get()->WorldAssetClass)
					.DisplayThumbnail(false)
					.OnObjectChanged(this, &SLevelEditorNarrative::OnNarrativeChanged)
					.ObjectPath(this, &SLevelEditorNarrative::GetNarrativeAssetPath) // needs function to automatically refresh view upon data change
			]
	];
}

FString SLevelEditorNarrative::GetNarrativeAssetPath() const
{
	return NarrativeAssetPath;
}

void SLevelEditorNarrative::OnNarrativeChanged(const FAssetData& NewAsset)
{
	NarrativeAssetPath = NewAsset.GetObjectPathString();

	if (UNarrativeComponent* NarrativeComponent = FindNarrativeComponent())
	{
		if (UObject* NewObject = NewAsset.GetAsset())
		{
			NarrativeComponent->RootNarrative = Cast<UNarrativeAsset>(NewObject);
		}
		else
		{
			NarrativeComponent->RootNarrative = nullptr;
		}

		const bool bSuccess = NarrativeComponent->MarkPackageDirty();
		ensureMsgf(bSuccess, TEXT("World Settings couldn't be marked dirty while changing the assigned Narrative Asset."));
	}
}

UNarrativeComponent* SLevelEditorNarrative::FindNarrativeComponent()
{
	if (const UWorld* World = GEditor->GetEditorWorldContext().World())
	{
		if (const AWorldSettings* WorldSettings = World->GetWorldSettings())
		{
			if (UActorComponent* FoundComponent = WorldSettings->GetComponentByClass(UNarrativeComponent::StaticClass()))
			{
				return Cast<UNarrativeComponent>(FoundComponent);
			}
		}
	}

	return nullptr;
}

#undef LOCTEXT_NAMESPACE
