// Copyright XiaoYao

#pragma once

#include "UnrealExtensions/INarrativeCuratedNamePropertyCustomization.h"

#include "NarrativeOwnerFunctionRef.h"

class UNarrativeAsset;
class UNarrativeNode;
class UObject;
class UClass;
class UFunction;
class UNarrativeNode_CallOwnerFunction;

class FNarrativeOwnerFunctionRefCustomization : public INarrativeCuratedNamePropertyCustomization
{
private:
	typedef INarrativeCuratedNamePropertyCustomization Super;

public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance() { return MakeShareable(new FNarrativeOwnerFunctionRefCustomization()); }

protected:
	// IPropertyTypeCustomization
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	// ---

	// ICuratedNamePropertyCustomization
	virtual TSharedPtr<IPropertyHandle> GetCuratedNamePropertyHandle() const override;
	virtual void SetCuratedName(const FName& NewName) override;
	virtual FName GetCuratedName() const override;
	virtual TArray<FName> GetCuratedNameOptions() const override;
	// ---

	// Accessor to return the actual struct being edited
	FORCEINLINE FNarrativeOwnerFunctionRef* GetNarrativeOwnerFunctionRef() const
	{
		return INarrativeExtendedPropertyTypeCustomization::TryGetTypedStructValue<FNarrativeOwnerFunctionRef>(StructPropertyHandle);
	}

	const UClass* TryGetExpectedOwnerClass() const;
	UNarrativeNode* TryGetNarrativeNodeOuter() const;

	static TArray<FName> GetNarrativeOwnerFunctionRefs(const UNarrativeNode_CallOwnerFunction& NarrativeNodeOwner, const UClass& ExpectedOwnerClass);

	static bool IsFunctionUsable(const UFunction& Function, const UNarrativeNode_CallOwnerFunction& NarrativeNodeOwner);
};
