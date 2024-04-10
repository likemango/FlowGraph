// Copyright XiaoYao

#pragma once

#include "EditorSubsystem.h"
#include "Logging/TokenizedMessage.h"
#include "NarrativeDebuggerSubsystem.generated.h"

class UNarrativeAsset;
class FNarrativeMessageLog;

/**
** Persistent subsystem supporting Narrative Graph debugging
 */
UCLASS()
class NARRATIVEEDITOR_API UNarrativeDebuggerSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()
	
public:
	UNarrativeDebuggerSubsystem();

protected:	
	TMap<TWeakObjectPtr<UNarrativeAsset>, TSharedPtr<class IMessageLogListing>> RuntimeLogs;

	void OnInstancedTemplateAdded(UNarrativeAsset* NarrativeAsset);
	void OnInstancedTemplateRemoved(UNarrativeAsset* NarrativeAsset) const;
	
	void OnRuntimeMessageAdded(UNarrativeAsset* NarrativeAsset, const TSharedRef<FTokenizedMessage>& Message) const;
	
	void OnBeginPIE(const bool bIsSimulating);
	void OnEndPIE(const bool bIsSimulating);

public:	
	static void PausePlaySession();
	static bool IsPlaySessionPaused();
};
