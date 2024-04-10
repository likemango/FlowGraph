// Copyright XiaoYao

#pragma once

#include "NarrativePin.generated.h"

USTRUCT()
struct NARRATIVE_API FNarrativePin
{
	GENERATED_BODY()

	// A logical name, used during execution of pin
	UPROPERTY(EditDefaultsOnly, Category = "NarrativePin")
	FName PinName;

	// An optional Display Name, you can use it to override PinName without the need to update graph connections
	UPROPERTY(EditDefaultsOnly, Category = "NarrativePin")
	FText PinFriendlyName;

	UPROPERTY(EditDefaultsOnly, Category = "NarrativePin")
	FString PinToolTip;

	static inline FName AnyPinName = TEXT("AnyPinName");

	FNarrativePin()
		: PinName(NAME_None)
	{
	}

	FNarrativePin(const FName& InPinName)
		: PinName(InPinName)
	{
	}

	FNarrativePin(const FString& InPinName)
		: PinName(*InPinName)
	{
	}

	FNarrativePin(const FText& InPinName)
		: PinName(*InPinName.ToString())
	{
	}

	FNarrativePin(const TCHAR* InPinName)
		: PinName(FName(InPinName))
	{
	}

	FNarrativePin(const uint8& InPinName)
		: PinName(FName(*FString::FromInt(InPinName)))
	{
	}

	FNarrativePin(const int32& InPinName)
		: PinName(FName(*FString::FromInt(InPinName)))
	{
	}

	FNarrativePin(const FStringView InPinName, const FText& InPinFriendlyName)
		: PinName(InPinName)
		, PinFriendlyName(InPinFriendlyName)
	{
	}

	FNarrativePin(const FStringView InPinName, const FString& InPinTooltip)
		: PinName(InPinName)
		, PinToolTip(InPinTooltip)
	{
	}

	FNarrativePin(const FStringView InPinName, const FText& InPinFriendlyName, const FString& InPinTooltip)
		: PinName(InPinName)
		, PinFriendlyName(InPinFriendlyName)
		, PinToolTip(InPinTooltip)
	{
	}

	FORCEINLINE bool IsValid() const
	{
		return !PinName.IsNone();
	}

	FORCEINLINE bool operator==(const FNarrativePin& Other) const
	{
		return PinName == Other.PinName;
	}

	FORCEINLINE bool operator!=(const FNarrativePin& Other) const
	{
		return PinName != Other.PinName;
	}

	FORCEINLINE bool operator==(const FName& Other) const
	{
		return PinName == Other;
	}

	FORCEINLINE bool operator!=(const FName& Other) const
	{
		return PinName != Other;
	}

	friend uint32 GetTypeHash(const FNarrativePin& NarrativePin)
	{
		return GetTypeHash(NarrativePin.PinName);
	}
};

USTRUCT()
struct NARRATIVE_API FNarrativePinHandle
{
	GENERATED_BODY()

	// Update SNarrativePinHandleBase code if this property name would be ever changed
	UPROPERTY()
	FName PinName;

	FNarrativePinHandle()
		: PinName(NAME_None)
	{
	}
};

USTRUCT(BlueprintType)
struct NARRATIVE_API FNarrativeInputPinHandle : public FNarrativePinHandle
{
	GENERATED_BODY()

	FNarrativeInputPinHandle()
	{
	}
};

USTRUCT(BlueprintType)
struct NARRATIVE_API FNarrativeOutputPinHandle : public FNarrativePinHandle
{
	GENERATED_BODY()

	FNarrativeOutputPinHandle()
	{
	}
};

// Processing Narrative Nodes creates map of connected pins
USTRUCT()
struct NARRATIVE_API FConnectedPin
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FGuid NodeGuid;

	UPROPERTY()
	FName PinName;

	FConnectedPin()
		: NodeGuid(FGuid())
		, PinName(NAME_None)
	{
	}

	FConnectedPin(const FGuid InNodeId, const FName& InPinName)
		: NodeGuid(InNodeId)
		, PinName(InPinName)
	{
	}

	FORCEINLINE bool operator==(const FConnectedPin& Other) const
	{
		return NodeGuid == Other.NodeGuid && PinName == Other.PinName;
	}

	FORCEINLINE bool operator!=(const FConnectedPin& Other) const
	{
		return NodeGuid != Other.NodeGuid || PinName != Other.PinName;
	}

	friend uint32 GetTypeHash(const FConnectedPin& ConnectedPin)
	{
		return GetTypeHash(ConnectedPin.NodeGuid) + GetTypeHash(ConnectedPin.PinName);
	}
};

UENUM(BlueprintType)
enum class ENarrativePinActivationType : uint8
{
	Default,
	Forced,
	PassThrough
};

// Every time pin is activated, we record it and display this data while user hovers mouse over pin
#if !UE_BUILD_SHIPPING
struct NARRATIVE_API FPinRecord
{
	double Time;
	FString HumanReadableTime;
	ENarrativePinActivationType ActivationType;

	static FString NoActivations;
	static FString PinActivations;
	static FString ForcedActivation;
	static FString PassThroughActivation;

	FPinRecord();
	FPinRecord(const double InTime, const ENarrativePinActivationType InActivationType);

private:
	FORCEINLINE static FString DoubleDigit(const int32 Number);
};
#endif

// It can represent any trait added on the specific node instance, i.e. breakpoint
USTRUCT()
struct NARRATIVE_API FNarrativePinTrait
{
	GENERATED_USTRUCT_BODY()

protected:	
	UPROPERTY()
	uint8 bTraitAllowed : 1;

	uint8 bEnabled : 1;
	uint8 bHit : 1;

public:
	FNarrativePinTrait()
		: bTraitAllowed(false)
		, bEnabled(false)
		, bHit(false)
	{
	};

	explicit FNarrativePinTrait(const bool bInitialState)
		: bTraitAllowed(bInitialState)
		, bEnabled(bInitialState)
		, bHit(false)
	{
	};

	void AllowTrait();
	void DisallowTrait();
	bool IsAllowed() const;

	void EnableTrait();
	void DisableTrait();
	void ToggleTrait();

	bool CanEnable() const;
	bool IsEnabled() const;

	void MarkAsHit();
	void ResetHit();
	bool IsHit() const;
};
