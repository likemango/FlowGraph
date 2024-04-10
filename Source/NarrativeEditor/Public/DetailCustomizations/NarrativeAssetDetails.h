// Copyright XiaoYao

#pragma once

#include "IDetailCustomization.h"
#include "Templates/SharedPointer.h"
#include "Types/SlateEnums.h"

class IDetailChildrenBuilder;
class IDetailLayoutBuilder;
class IPropertyHandle;

class FNarrativeAssetDetails final : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShareable(new FNarrativeAssetDetails());
	}

	// IDetailCustomization
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	// --

private:
	void GenerateCustomPinArray(TSharedRef<IPropertyHandle> PropertyHandle, int32 ArrayIndex, IDetailChildrenBuilder& ChildrenBuilder);

	FText GetCustomPinText(TSharedRef<IPropertyHandle> PropertyHandle) const;
	static void OnCustomPinTextCommitted(const FText& InText, ETextCommit::Type InCommitType, TSharedRef<IPropertyHandle> PropertyHandle);
	static bool VerifyNewCustomPinText(const FText& InNewText, FText& OutErrorMessage);
};
