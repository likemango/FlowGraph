// Copyright XiaoYao

#include "DetailCustomizations/NarrativeNode_PlayLevelSequenceDetails.h"
#include "Nodes/World/NarrativeNode_PlayLevelSequence.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"

void FNarrativeNode_PlayLevelSequenceDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	IDetailCategoryBuilder& SequenceCategory = DetailBuilder.EditCategory("Sequence");
	SequenceCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UNarrativeNode_PlayLevelSequence, Sequence));
	SequenceCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UNarrativeNode_PlayLevelSequence, PlaybackSettings));
	SequenceCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UNarrativeNode_PlayLevelSequence, bPlayReverse));
	SequenceCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UNarrativeNode_PlayLevelSequence, CameraSettings));
	SequenceCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UNarrativeNode_PlayLevelSequence, bUseGraphOwnerAsTransformOrigin));
	SequenceCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UNarrativeNode_PlayLevelSequence, bReplicates));
	SequenceCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UNarrativeNode_PlayLevelSequence, bAlwaysRelevant));
	SequenceCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UNarrativeNode_PlayLevelSequence, bApplyOwnerTimeDilation));
}
