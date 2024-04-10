// Copyright XiaoYao

#include "DetailCustomizations/NarrativeNode_Details.h"
#include "PropertyEditing.h"

void FNarrativeNode_Details::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	// hide class properties while editing node instance placed in the graph
	if (DetailLayout.HasClassDefaultObject() == false)
	{
		DetailLayout.HideCategory(TEXT("NarrativeNode"));
	}
}
