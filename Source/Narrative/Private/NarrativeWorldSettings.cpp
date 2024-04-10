// Copyright XiaoYao

#include "NarrativeWorldSettings.h"
#include "NarrativeComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativeWorldSettings)

ANarrativeWorldSettings::ANarrativeWorldSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NarrativeComponent = CreateDefaultSubobject<UNarrativeComponent>(TEXT("NarrativeComponent"));

	// We need this if project uses custom AWorldSettings classed inherited after this one
	// In this case engine would call BeginPlay multiple times... for ANarrativeWorldSettings and every inherited AWorldSettings class...
	NarrativeComponent->bAllowMultipleInstances = false;
}
