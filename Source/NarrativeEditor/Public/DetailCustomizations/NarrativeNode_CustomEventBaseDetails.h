// Copyright XiaoYao

#pragma once

#include "IDetailCustomization.h"
#include "Templates/SharedPointer.h"
#include "Types/SlateEnums.h"
#include "Widgets/Input/SComboBox.h"

class IDetailCategoryBuilder;
class UNarrativeAsset;

class FNarrativeNode_CustomEventBaseDetails : public IDetailCustomization
{
public:
	// IDetailCustomization
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;
	// --

protected:
	void CustomizeDetailsInternal(IDetailLayoutBuilder& DetailLayout, const FText& CustomRowNameText, const FText& EventNameText);

	virtual IDetailCategoryBuilder& CreateDetailCategory(IDetailLayoutBuilder& DetailLayout) const = 0;
	virtual TArray<FName> BuildEventNames(const UNarrativeAsset& NarrativeAsset) const = 0;

	void OnComboBoxOpening();
	void RebuildEventNames();
	TSharedRef<SWidget> GenerateEventWidget(TSharedPtr<FName> Item) const;
	FText GetSelectedEventText() const;
	void PinSelectionChanged(TSharedPtr<FName> Item, ESelectInfo::Type SelectInfo);
	bool IsInEventNames(const FName& EventName) const;

	TArray<TWeakObjectPtr<UObject>> ObjectsBeingEdited;
	TArray<TSharedPtr<FName>> EventNames;
	TSharedPtr<FName> CachedEventNameSelected;
	TSharedPtr<SComboBox<TSharedPtr<FName>>> EventTextListWidget;
	bool bExcludeReferencedEvents = false;
};
