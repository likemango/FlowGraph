// Copyright XiaoYao

#include "Asset/NarrativeAssetEditorContext.h"
#include "Asset/NarrativeAssetEditor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativeAssetEditorContext)

UNarrativeAsset* UNarrativeAssetEditorContext::GetNarrativeAsset() const
{
	return NarrativeAssetEditor.IsValid() ? NarrativeAssetEditor.Pin()->GetNarrativeAsset() : nullptr;
}
