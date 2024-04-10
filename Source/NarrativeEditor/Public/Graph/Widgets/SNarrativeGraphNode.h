// Copyright XiaoYao

#pragma once

#include "SGraphNode.h"
#include "KismetPins/SGraphPinExec.h"

#include "Graph/Nodes/NarrativeGraphNode.h"

class NARRATIVEEDITOR_API SNarrativeGraphPinExec : public SGraphPinExec
{
public:
	SNarrativeGraphPinExec();

	SLATE_BEGIN_ARGS(SNarrativeGraphPinExec) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InPin);
};

class NARRATIVEEDITOR_API SNarrativeGraphNode : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SNarrativeGraphNode) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UNarrativeGraphNode* InNode);

protected:
	// SNodePanel::SNode
	virtual void GetNodeInfoPopups(FNodeInfoContext* Context, TArray<FGraphInformationPopupInfo>& Popups) const override;
	virtual const FSlateBrush* GetShadowBrush(bool bSelected) const override;
	virtual void GetOverlayBrushes(bool bSelected, const FVector2D WidgetSize, TArray<FOverlayBrushInfo>& Brushes) const override;
	// --

	virtual void GetPinBrush(const bool bLeftSide, const float WidgetWidth, const int32 PinIndex, const FNarrativePinTrait& Breakpoint, TArray<FOverlayBrushInfo>& Brushes) const;

	// SGraphNode
	virtual void UpdateGraphNode() override;
	virtual void UpdateErrorInfo() override;

	virtual TSharedRef<SWidget> CreateTitleWidget(TSharedPtr<SNodeTitle> NodeTitle) override;
	virtual TSharedRef<SWidget> CreateNodeContentArea() override;
	virtual const FSlateBrush* GetNodeBodyBrush() const override;

	// purposely overriden non-virtual methods, added PR #9791 to made these methods virtual: https://github.com/EpicGames/UnrealEngine/pull/9791
	FSlateColor GetNodeTitleColor() const;
	FSlateColor GetNodeBodyColor() const;
	FSlateColor GetNodeTitleIconColor() const;
	FLinearColor GetNodeTitleTextColor() const;
	TSharedPtr<SWidget> GetEnabledStateWidget() const;

	virtual void CreateStandardPinWidget(UEdGraphPin* Pin) override;
	virtual TSharedPtr<SToolTip> GetComplexTooltip() override;

	virtual void CreateInputSideAddButton(TSharedPtr<SVerticalBox> OutputBox) override;
	virtual void CreateOutputSideAddButton(TSharedPtr<SVerticalBox> OutputBox) override;
	// --
	
	// Variant of SGraphNode::AddPinButtonContent
	virtual void AddPinButton(TSharedPtr<SVerticalBox> OutputBox, TSharedRef<SWidget> ButtonContent, const EEdGraphPinDirection Direction, FString DocumentationExcerpt = FString(), TSharedPtr<SToolTip> CustomTooltip = nullptr);

	// Variant of SGraphNode::OnAddPin
	virtual FReply OnAddNarrativePin(const EEdGraphPinDirection Direction);

private:
	static int32 ValidPinsCount(const TArray<FNarrativePin>& Pins);

protected:
	UNarrativeGraphNode* NarrativeGraphNode = nullptr;
};
