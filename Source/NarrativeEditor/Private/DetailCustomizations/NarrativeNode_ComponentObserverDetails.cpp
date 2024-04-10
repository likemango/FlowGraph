// Copyright XiaoYao

#include "DetailCustomizations/NarrativeNode_ComponentObserverDetails.h"
#include "Nodes/World/NarrativeNode_ComponentObserver.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"

void FNarrativeNode_ComponentObserverDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	IDetailCategoryBuilder& SequenceCategory = DetailBuilder.EditCategory("ObservedComponent");
	SequenceCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UNarrativeNode_ComponentObserver, IdentityTags));
	SequenceCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UNarrativeNode_ComponentObserver, IdentityMatchType));
}
