// Copyright XiaoYao

#include "Graph/NarrativeGraphConnectionDrawingPolicy.h"

#include "Graph/NarrativeGraph.h"
#include "Graph/NarrativeGraphEditor.h"
#include "Graph/NarrativeGraphEditorSettings.h"
#include "Graph/NarrativeGraphSchema.h"
#include "Graph/NarrativeGraphSettings.h"
#include "Graph/NarrativeGraphUtils.h"
#include "Graph/Nodes/NarrativeGraphNode.h"

#include "NarrativeAsset.h"
#include "Graph/Nodes/NarrativeGraphNode_Reroute.h"
#include "Nodes/NarrativeNode.h"

#include "Misc/App.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativeGraphConnectionDrawingPolicy)

FConnectionDrawingPolicy* FNarrativeGraphConnectionDrawingPolicyFactory::CreateConnectionPolicy(const class UEdGraphSchema* Schema, int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const class FSlateRect& InClippingRect, class FSlateWindowElementList& InDrawElements, class UEdGraph* InGraphObj) const
{
	if (Schema->IsA(UNarrativeGraphSchema::StaticClass()))
	{
		return new FNarrativeGraphConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, ZoomFactor, InClippingRect, InDrawElements, InGraphObj);
	}
	return nullptr;
}

/////////////////////////////////////////////////////
// FNarrativeGraphConnectionDrawingPolicy

FNarrativeGraphConnectionDrawingPolicy::FNarrativeGraphConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj)
	: FConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, ZoomFactor, InClippingRect, InDrawElements)
	, GraphObj(InGraphObj)
{
	// Cache off the editor options
	RecentWireDuration = UNarrativeGraphSettings::Get()->RecentWireDuration;

	InactiveColor = UNarrativeGraphSettings::Get()->InactiveWireColor;
	RecentColor = UNarrativeGraphSettings::Get()->RecentWireColor;
	RecordedColor = UNarrativeGraphSettings::Get()->RecordedWireColor;
	SelectedColor = UNarrativeGraphSettings::Get()->SelectedWireColor;

	InactiveWireThickness = UNarrativeGraphSettings::Get()->InactiveWireThickness;
	RecentWireThickness = UNarrativeGraphSettings::Get()->RecentWireThickness;
	RecordedWireThickness = UNarrativeGraphSettings::Get()->RecordedWireThickness;
	SelectedWireThickness = UNarrativeGraphSettings::Get()->SelectedWireThickness;

	// Don't want to draw ending arrowheads
	ArrowImage = nullptr;
	ArrowRadius = FVector2D::ZeroVector;
}

void FNarrativeGraphConnectionDrawingPolicy::BuildPaths()
{
	if (const UNarrativeAsset* NarrativeInstance = CastChecked<UNarrativeGraph>(GraphObj)->GetNarrativeAsset()->GetInspectedInstance())
	{
		const double CurrentTime = FApp::GetCurrentTime();

		for (const UNarrativeNode* Node : NarrativeInstance->GetRecordedNodes())
		{
			const UNarrativeGraphNode* NarrativeGraphNode = Cast<UNarrativeGraphNode>(Node->GetGraphNode());

			for (const TPair<uint8, FPinRecord>& Record : Node->GetWireRecords())
			{
				if (UEdGraphPin* OutputPin = NarrativeGraphNode->OutputPins[Record.Key])
				{
					// check if Output pin is connected to anything
					if (OutputPin->LinkedTo.Num() > 0)
					{
						RecordedPaths.Emplace(OutputPin, OutputPin->LinkedTo[0]);

						if (CurrentTime < Record.Value.Time + RecentWireDuration)
						{
							RecentPaths.Emplace(OutputPin, OutputPin->LinkedTo[0]);
						}
					}
				}
			}
		}
	}

	if (GraphObj && (UNarrativeGraphEditorSettings::Get()->bHighlightInputWiresOfSelectedNodes || UNarrativeGraphEditorSettings::Get()->bHighlightOutputWiresOfSelectedNodes))
	{
		const TSharedPtr<SNarrativeGraphEditor> NarrativeGraphEditor = FNarrativeGraphUtils::GetNarrativeGraphEditor(GraphObj);
		if (NarrativeGraphEditor.IsValid())
		{
			for (UNarrativeGraphNode* SelectedNode : NarrativeGraphEditor->GetSelectedNarrativeNodes())
			{
				for (UEdGraphPin* Pin : SelectedNode->Pins)
				{
					if ((Pin->Direction == EGPD_Input && UNarrativeGraphEditorSettings::Get()->bHighlightInputWiresOfSelectedNodes)
						|| (Pin->Direction == EGPD_Output && UNarrativeGraphEditorSettings::Get()->bHighlightOutputWiresOfSelectedNodes))
					{
						for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
						{
							SelectedPaths.Emplace(Pin, LinkedPin);
						}
					}
				}
			}
		}
	}
}

void FNarrativeGraphConnectionDrawingPolicy::DrawConnection(int32 LayerId, const FVector2D& Start, const FVector2D& End, const FConnectionParams& Params)
{
	switch (UNarrativeGraphSettings::Get()->ConnectionDrawType)
	{
		case ENarrativeConnectionDrawType::Default:
			FConnectionDrawingPolicy::DrawConnection(LayerId, Start, End, Params);
			break;
		case ENarrativeConnectionDrawType::Circuit:
			DrawCircuitSpline(LayerId, Start, End, Params);
			break;
		default: ;
	}
}

// Give specific editor modes a chance to highlight this connection or darken non-interesting connections
void FNarrativeGraphConnectionDrawingPolicy::DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, FConnectionParams& Params)
{
	Params.AssociatedPin1 = OutputPin;
	Params.AssociatedPin2 = InputPin;

	// Get the schema and grab the default color from it
	check(OutputPin);
	check(GraphObj);
	const UEdGraphSchema* Schema = GraphObj->GetSchema();

	if (OutputPin->bOrphanedPin || (InputPin && InputPin->bOrphanedPin))
	{
		Params.WireColor = FLinearColor::Red;
	}
	else if (Cast<UNarrativeGraphNode>(OutputPin->GetOwningNode())->GetSignalMode() == ENarrativeSignalMode::Disabled)
	{
		Params.WireColor *= 0.5f;
		Params.WireThickness = 0.5f;
	}
	else
	{
		Params.WireColor = Schema->GetPinTypeColor(OutputPin->PinType);

		if (InputPin)
		{
			// selected paths
			if (SelectedPaths.Contains(OutputPin) || SelectedPaths.Contains(InputPin))
			{
				Params.WireColor = SelectedColor;
				Params.WireThickness = SelectedWireThickness;
				Params.bDrawBubbles = false;
				return;
			}

			// recent paths
			if (RecentPaths.Contains(OutputPin) && RecentPaths[OutputPin] == InputPin)
			{
				Params.WireColor = RecentColor;
				Params.WireThickness = RecentWireThickness;
				Params.bDrawBubbles = true;
				return;
			}

			// all paths, showing graph history
			if (RecordedPaths.Contains(OutputPin) && RecordedPaths[OutputPin] == InputPin)
			{
				Params.WireColor = RecordedColor;
				Params.WireThickness = RecordedWireThickness;
				Params.bDrawBubbles = false;
				return;
			}

			// It's not followed, fade it and keep it thin
			Params.WireColor = InactiveColor;
			Params.WireThickness = InactiveWireThickness;
		}
	}

	// If reroute node path goes backwards, we need to flip the direction to make it look nice
	// (all of the logic for this is basically same as in FKismetConnectionDrawingPolicy)
	{
		UEdGraphNode* OutputNode = OutputPin->GetOwningNode();
		UEdGraphNode* InputNode = (InputPin != nullptr) ? InputPin->GetOwningNode() : nullptr;
		if (auto* OutputRerouteNode = Cast<UNarrativeGraphNode_Reroute>(OutputNode))
		{
			if (ShouldChangeTangentForReroute(OutputRerouteNode))
			{
				Params.StartDirection = EGPD_Input;
			}
		}

		if (auto* InputRerouteNode = Cast<UNarrativeGraphNode_Reroute>(InputNode))
		{
			if (ShouldChangeTangentForReroute(InputRerouteNode))
			{
				Params.EndDirection = EGPD_Output;
			}
		}
	}

	const bool bDeemphasizeUnhoveredPins = HoveredPins.Num() > 0;

	if (bDeemphasizeUnhoveredPins)
	{
		ApplyHoverDeemphasis(OutputPin, InputPin, /*inout*/ Params.WireThickness, /*inout*/ Params.WireColor);
	}

}

void FNarrativeGraphConnectionDrawingPolicy::Draw(TMap<TSharedRef<SWidget>, FArrangedWidget>& InPinGeometries, FArrangedChildren& ArrangedNodes)
{
	BuildPaths();

	FConnectionDrawingPolicy::Draw(InPinGeometries, ArrangedNodes);
}

void FNarrativeGraphConnectionDrawingPolicy::DrawCircuitSpline(const int32& LayerId, const FVector2D& Start, const FVector2D& End, const FConnectionParams& Params) const
{
	const FVector2D StartingPoint = FVector2D(Start.X + UNarrativeGraphSettings::Get()->CircuitConnectionSpacing.X, Start.Y);
	const FVector2D EndPoint = FVector2D(End.X - UNarrativeGraphSettings::Get()->CircuitConnectionSpacing.Y, End.Y);
	const FVector2D ControlPoint = GetControlPoint(StartingPoint, EndPoint);

	const FVector2D StartDirection = (Params.StartDirection == EGPD_Output) ? FVector2D(1.0f, 0.0f) : FVector2D(-1.0f, 0.0f);
	const FVector2D EndDirection = (Params.EndDirection == EGPD_Input) ? FVector2D(1.0f, 0.0f) : FVector2D(-1.0f, 0.0f);

	DrawCircuitConnection(LayerId, Start, StartDirection, StartingPoint, EndDirection, Params);
	DrawCircuitConnection(LayerId, StartingPoint, StartDirection, ControlPoint, EndDirection, Params);
	DrawCircuitConnection(LayerId, ControlPoint, StartDirection, EndPoint, EndDirection, Params);
	DrawCircuitConnection(LayerId, EndPoint, StartDirection, End, EndDirection, Params);
}

void FNarrativeGraphConnectionDrawingPolicy::DrawCircuitConnection(const int32& LayerId, const FVector2D& Start, const FVector2D& StartDirection, const FVector2D& End, const FVector2D& EndDirection, const FConnectionParams& Params) const
{
	FSlateDrawElement::MakeDrawSpaceSpline(DrawElementsList, LayerId, Start, StartDirection, End, EndDirection, Params.WireThickness, ESlateDrawEffect::None, Params.WireColor);

	if (Params.bDrawBubbles)
	{
		// This table maps distance along curve to alpha
		FInterpCurve<float> SplineReparamTable;
		const float SplineLength = MakeSplineReparamTable(Start, StartDirection, End, EndDirection, SplineReparamTable);

		// Draw bubbles on the spline
		if (Params.bDrawBubbles)
		{
			const float BubbleSpacing = 64.f * ZoomFactor;
			const float BubbleSpeed = 192.f * ZoomFactor;
			const FVector2D BubbleSize = BubbleImage->ImageSize * ZoomFactor * 0.2f * Params.WireThickness;

			const float Time = (FPlatformTime::Seconds() - GStartTime);
			const float BubbleOffset = FMath::Fmod(Time * BubbleSpeed, BubbleSpacing);
			const int32 NumBubbles = FMath::CeilToInt(SplineLength / BubbleSpacing);
			for (int32 i = 0; i < NumBubbles; ++i)
			{
				const float Distance = (static_cast<float>(i) * BubbleSpacing) + BubbleOffset;
				if (Distance < SplineLength)
				{
					const float Alpha = SplineReparamTable.Eval(Distance, 0.f);
					FVector2D BubblePos = FMath::CubicInterp(Start, StartDirection, End, EndDirection, Alpha);
					BubblePos -= (BubbleSize * 0.5f);

					FSlateDrawElement::MakeBox(DrawElementsList, LayerId, FPaintGeometry(BubblePos, BubbleSize, ZoomFactor), BubbleImage, ESlateDrawEffect::None, Params.WireColor);
				}
			}
		}
	}
}

FVector2D FNarrativeGraphConnectionDrawingPolicy::GetControlPoint(const FVector2D& Source, const FVector2D& Target)
{
	const FVector2D Delta = Target - Source;
	const float Tangent = FMath::Tan(UNarrativeGraphSettings::Get()->CircuitConnectionAngle * (PI / 180.f));

	const float DeltaX = FMath::Abs(Delta.X);
	const float DeltaY = FMath::Abs(Delta.Y);

	const float SlopeWidth = DeltaY / Tangent;
	if (DeltaX > SlopeWidth)
	{
		return Delta.X > 0.f ? FVector2D(Target.X - SlopeWidth, Source.Y) : FVector2D(Source.X - SlopeWidth, Target.Y);
	}

	const float SlopeHeight = DeltaX * Tangent;
	if (DeltaY > SlopeHeight)
	{
		if (Delta.Y > 0.f)
		{
			return Delta.X < 0.f ? FVector2D(Source.X, Target.Y - SlopeHeight) : FVector2D(Target.X, Source.Y + SlopeHeight);
		}

		if (Delta.X < 0.f)
		{
			return FVector2D(Source.X, Target.Y + SlopeHeight);
		}
	}

	return FVector2D(Target.X, Source.Y - SlopeHeight);
}

bool FNarrativeGraphConnectionDrawingPolicy::ShouldChangeTangentForReroute(UNarrativeGraphNode_Reroute* Reroute)
{
	if (const bool* pResult = RerouteToReversedDirectionMap.Find(Reroute))
	{
		return *pResult;
	}
	else
	{
		bool bPinReversed = false;

		FVector2D AverageLeftPin;
		FVector2D AverageRightPin;
		FVector2D CenterPin;
		const bool bCenterValid = Reroute->OutputPins.Num() == 0 ? false : FindPinCenter(Reroute->OutputPins[0], /*out*/ CenterPin);
		const bool bLeftValid = GetAverageConnectedPosition(Reroute, EGPD_Input, /*out*/ AverageLeftPin);
		const bool bRightValid = GetAverageConnectedPosition(Reroute, EGPD_Output, /*out*/ AverageRightPin);

		if (bLeftValid && bRightValid)
		{
			bPinReversed = AverageRightPin.X < AverageLeftPin.X;
		}
		else if (bCenterValid)
		{
			if (bLeftValid)
			{
				bPinReversed = CenterPin.X < AverageLeftPin.X;
			}
			else if (bRightValid)
			{
				bPinReversed = AverageRightPin.X < CenterPin.X;
			}
		}

		RerouteToReversedDirectionMap.Add(Reroute, bPinReversed);

		return bPinReversed;
	}
}

bool FNarrativeGraphConnectionDrawingPolicy::FindPinCenter(const UEdGraphPin* Pin, FVector2D& OutCenter) const
{
	if (const TSharedPtr<SGraphPin>* PinWidget = PinToPinWidgetMap.Find(Pin))
	{
		if (const FArrangedWidget* PinEntry = PinGeometries->Find((*PinWidget).ToSharedRef()))
		{
			OutCenter = FGeometryHelper::CenterOf(PinEntry->Geometry);
			return true;
		}
	}

	return false;
}

bool FNarrativeGraphConnectionDrawingPolicy::GetAverageConnectedPosition(UNarrativeGraphNode_Reroute* Reroute, EEdGraphPinDirection Direction, FVector2D& OutPos) const
{
	FVector2D Result = FVector2D::ZeroVector;
	int32 ResultCount = 0;

	if(Reroute->InputPins.Num() == 0 || Reroute->OutputPins.Num() == 0)
	{
		return false;
	}
	
	UEdGraphPin* Pin = (Direction == EGPD_Input) ? Reroute->InputPins[0] : Reroute->OutputPins[0];
	for (const UEdGraphPin* LinkedPin : Pin->LinkedTo)
	{
		FVector2D CenterPoint;
		if (FindPinCenter(LinkedPin, /*out*/ CenterPoint))
		{
			Result += CenterPoint;
			ResultCount++;
		}
	}

	if (ResultCount > 0)
	{
		OutPos = Result * (1.0f / ResultCount);
		return true;
	}
	else
	{
		return false;
	}
}

