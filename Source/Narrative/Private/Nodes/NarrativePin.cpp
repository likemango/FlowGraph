// Copyright XiaoYao

#include "Nodes/NarrativePin.h"

#include "Misc/DateTime.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativePin)

//////////////////////////////////////////////////////////////////////////
// Pin Record

#if !UE_BUILD_SHIPPING
FString FPinRecord::NoActivations = TEXT("No activations");
FString FPinRecord::PinActivations = TEXT("Pin activations");
FString FPinRecord::ForcedActivation = TEXT(" (forced activation)");
FString FPinRecord::PassThroughActivation = TEXT(" (pass-through activation)");

FPinRecord::FPinRecord()
	: Time(0.0f)
	, HumanReadableTime(FString())
	, ActivationType(ENarrativePinActivationType::Default)
{
}

FPinRecord::FPinRecord(const double InTime, const ENarrativePinActivationType InActivationType)
	: Time(InTime)
	, ActivationType(InActivationType)
{
	const FDateTime SystemTime(FDateTime::Now());
	HumanReadableTime = DoubleDigit(SystemTime.GetHour()) + TEXT(".")
		+ DoubleDigit(SystemTime.GetMinute()) + TEXT(".")
		+ DoubleDigit(SystemTime.GetSecond()) + TEXT(":")
		+ DoubleDigit(SystemTime.GetMillisecond()).Left(3);
}

FORCEINLINE FString FPinRecord::DoubleDigit(const int32 Number)
{
	return Number > 9 ? FString::FromInt(Number) : TEXT("0") + FString::FromInt(Number);
}
#endif

//////////////////////////////////////////////////////////////////////////
// Pin Trait

void FNarrativePinTrait::AllowTrait()
{
	if (!bTraitAllowed)
	{
		bTraitAllowed = true;
		bEnabled = true;
	}
}

void FNarrativePinTrait::DisallowTrait()
{
	if (bTraitAllowed)
	{
		bTraitAllowed = false;
		bEnabled = false;
	}
}

bool FNarrativePinTrait::IsAllowed() const
{
	return bTraitAllowed;
}

void FNarrativePinTrait::EnableTrait()
{
	if (bTraitAllowed && !bEnabled)
	{
		bEnabled = true;
	}
}

void FNarrativePinTrait::DisableTrait()
{
	if (bTraitAllowed && bEnabled)
	{
		bEnabled = false;
	}
}

void FNarrativePinTrait::ToggleTrait()
{
	if (bTraitAllowed)
	{
		bTraitAllowed = false;
		bEnabled = false;
	}
	else
	{
		bTraitAllowed = true;
		bEnabled = true;
	}
}

bool FNarrativePinTrait::CanEnable() const
{
	return bTraitAllowed && !bEnabled;
}

bool FNarrativePinTrait::IsEnabled() const
{
	return bTraitAllowed && bEnabled;
}

void FNarrativePinTrait::MarkAsHit()
{
	bHit = true;
}

void FNarrativePinTrait::ResetHit()
{
	bHit = false;
}

bool FNarrativePinTrait::IsHit() const
{
	return bHit;
}

