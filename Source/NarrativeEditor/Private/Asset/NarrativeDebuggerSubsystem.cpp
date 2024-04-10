// Copyright XiaoYao

#include "Asset/NarrativeDebuggerSubsystem.h"
#include "Asset/NarrativeAssetEditor.h"
#include "Asset/NarrativeMessageLogListing.h"

#include "NarrativeSubsystem.h"

#include "Editor/UnrealEdEngine.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Templates/Function.h"
#include "UnrealEdGlobals.h"
#include "Widgets/Notifications/SNotificationList.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativeDebuggerSubsystem)

#define LOCTEXT_NAMESPACE "NarrativeDebuggerSubsystem"

UNarrativeDebuggerSubsystem::UNarrativeDebuggerSubsystem()
{
	FEditorDelegates::BeginPIE.AddUObject(this, &UNarrativeDebuggerSubsystem::OnBeginPIE);
	FEditorDelegates::EndPIE.AddUObject(this, &UNarrativeDebuggerSubsystem::OnEndPIE);

	UNarrativeSubsystem::OnInstancedTemplateAdded.BindUObject(this, &UNarrativeDebuggerSubsystem::OnInstancedTemplateAdded);
	UNarrativeSubsystem::OnInstancedTemplateRemoved.BindUObject(this, &UNarrativeDebuggerSubsystem::OnInstancedTemplateRemoved);
}

void UNarrativeDebuggerSubsystem::OnInstancedTemplateAdded(UNarrativeAsset* NarrativeAsset)
{
	if (!RuntimeLogs.Contains(NarrativeAsset))
	{
		RuntimeLogs.Add(NarrativeAsset, FNarrativeMessageLogListing::GetLogListing(NarrativeAsset, ENarrativeLogType::Runtime));
		NarrativeAsset->OnRuntimeMessageAdded().AddUObject(this, &UNarrativeDebuggerSubsystem::OnRuntimeMessageAdded);
	}
}

void UNarrativeDebuggerSubsystem::OnInstancedTemplateRemoved(UNarrativeAsset* NarrativeAsset) const
{
	NarrativeAsset->OnRuntimeMessageAdded().RemoveAll(this);
}

void UNarrativeDebuggerSubsystem::OnRuntimeMessageAdded(UNarrativeAsset* NarrativeAsset, const TSharedRef<FTokenizedMessage>& Message) const
{
	const TSharedPtr<class IMessageLogListing> Log = RuntimeLogs.FindRef(NarrativeAsset);
	if (Log.IsValid())
	{
		Log->AddMessage(Message);
		Log->OnDataChanged().Broadcast();
	}
}

void UNarrativeDebuggerSubsystem::OnBeginPIE(const bool bIsSimulating)
{
	// clear all logs from a previous session
	RuntimeLogs.Empty();
}

void UNarrativeDebuggerSubsystem::OnEndPIE(const bool bIsSimulating)
{
	for (const TPair<TWeakObjectPtr<UNarrativeAsset>, TSharedPtr<class IMessageLogListing>>& Log : RuntimeLogs)
	{
		if (Log.Key.IsValid() && Log.Value->NumMessages(EMessageSeverity::Warning) > 0)
		{
			FNotificationInfo Info{FText::FromString(TEXT("Narrative Graph reported in-game issues"))};
			Info.ExpireDuration = 15.0;
			
			Info.HyperlinkText = FText::Format(LOCTEXT("OpenNarrativeAssetHyperlink", "Open {0}"), FText::FromString(Log.Key->GetName()));
			Info.Hyperlink = FSimpleDelegate::CreateLambda([this, Log]()
			{
				UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
				if (AssetEditorSubsystem->OpenEditorForAsset(Log.Key.Get()))
				{
					AssetEditorSubsystem->FindEditorForAsset(Log.Key.Get(), true)->InvokeTab(FNarrativeAssetEditor::RuntimeLogTab);
				}
			});

			const TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
			if (Notification.IsValid())
			{
				Notification->SetCompletionState(SNotificationItem::CS_Fail);
			}
		}
	}
}

void ForEachGameWorld(const TFunction<void(UWorld*)>& Func)
{
	for (const FWorldContext& PieContext : GUnrealEd->GetWorldContexts())
	{
		UWorld* PlayWorld = PieContext.World();
		if (PlayWorld && PlayWorld->IsGameWorld())
		{
			Func(PlayWorld);
		}
	}
}

bool AreAllGameWorldPaused()
{
	bool bPaused = true;
	ForEachGameWorld([&](const UWorld* World)
	{
		bPaused = bPaused && World->bDebugPauseExecution;
	});
	return bPaused;
}

void UNarrativeDebuggerSubsystem::PausePlaySession()
{
	bool bPaused = false;
	ForEachGameWorld([&](UWorld* World)
	{
		if (!World->bDebugPauseExecution)
		{
			World->bDebugPauseExecution = true;
			bPaused = true;
		}
	});
	if (bPaused)
	{
		GUnrealEd->PlaySessionPaused();
	}
}

bool UNarrativeDebuggerSubsystem::IsPlaySessionPaused()
{
	return AreAllGameWorldPaused();
}

#undef LOCTEXT_NAMESPACE
