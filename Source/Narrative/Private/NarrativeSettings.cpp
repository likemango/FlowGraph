// Copyright XiaoYao

#include "NarrativeSettings.h"
#include "NarrativeComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativeSettings)

UNarrativeSettings::UNarrativeSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bCreateNarrativeSubsystemOnClients(true)
	, bWarnAboutMissingIdentityTags(true)
	, bLogOnSignalDisabled(true)
	, bLogOnSignalPassthrough(true)
	, bUseAdaptiveNodeTitles(false)
	, DefaultExpectedOwnerClass(UNarrativeComponent::StaticClass())
{
}

UClass* UNarrativeSettings::GetDefaultExpectedOwnerClass() const
{
	return CastChecked<UClass>(TryResolveOrLoadSoftClass(DefaultExpectedOwnerClass), ECastCheckedType::NullAllowed);
}

UClass* UNarrativeSettings::TryResolveOrLoadSoftClass(const FSoftClassPath& SoftClassPath)
{
	if (UClass* Resolved = SoftClassPath.ResolveClass())
	{
		return Resolved;
	}

	return SoftClassPath.TryLoadClass<UObject>();
}