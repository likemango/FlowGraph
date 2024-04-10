// Copyright XiaoYao

#include "Nodes/NarrativeNodeBlueprintFactory.h"

#include "Nodes/NarrativeNode.h"
#include "Nodes/NarrativeNodeBlueprint.h"

#include "BlueprintEditorSettings.h"
#include "ClassViewerFilter.h"
#include "ClassViewerModule.h"
#include "Editor.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/MessageDialog.h"
#include "Modules/ModuleManager.h"
#include "SlateOptMacros.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SWindow.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativeNodeBlueprintFactory)

#define LOCTEXT_NAMESPACE "NarrativeNodeBlueprintFactory"

// ------------------------------------------------------------------------------
// Dialog to configure creation properties
// ------------------------------------------------------------------------------
BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

class SNarrativeNodeBlueprintCreateDialog final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SNarrativeNodeBlueprintCreateDialog) {}
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs)
	{
		bOkClicked = false;
		ParentClass = UNarrativeNode::StaticClass();

		ChildSlot
		[
			SNew(SBorder)
				.Visibility(EVisibility::Visible)
				.BorderImage(FAppStyle::GetBrush("Menu.Background"))
				[
					SNew(SBox)
						.Visibility(EVisibility::Visible)
						.WidthOverride(500.0f)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
								.FillHeight(1)
								[
									SNew(SBorder)
										.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
										.Content()
										[
											SAssignNew(ParentClassContainer, SVerticalBox)
										]
								]
							+ SVerticalBox::Slot()
								.AutoHeight()
								.HAlign(HAlign_Right)
								.VAlign(VAlign_Bottom)
								.Padding(8)
								[
									SNew(SUniformGridPanel)
										.SlotPadding(FAppStyle::GetMargin("StandardDialog.SlotPadding"))
										.MinDesiredSlotWidth(FAppStyle::GetFloat("StandardDialog.MinDesiredSlotWidth"))
										.MinDesiredSlotHeight(FAppStyle::GetFloat("StandardDialog.MinDesiredSlotHeight"))
										+ SUniformGridPanel::Slot(0, 0)
											[
												SNew(SButton)
													.HAlign(HAlign_Center)
													.ContentPadding(FAppStyle::GetMargin("StandardDialog.ContentPadding"))
													.OnClicked(this, &SNarrativeNodeBlueprintCreateDialog::OkClicked)
													.Text(LOCTEXT("CreateNarrativeNodeBlueprintOk", "OK"))
											]
										+ SUniformGridPanel::Slot(1, 0)
											[
												SNew(SButton)
													.HAlign(HAlign_Center)
													.ContentPadding(FAppStyle::GetMargin("StandardDialog.ContentPadding"))
													.OnClicked(this, &SNarrativeNodeBlueprintCreateDialog::CancelClicked)
													.Text(LOCTEXT("CreateNarrativeNodeBlueprintCancel", "Cancel"))
											]
								]
						]
				]
		];

			MakeParentClassPicker();
		}

	/** Sets properties for the supplied NarrativeNodeBlueprintFactory */
	bool ConfigureProperties(const TWeakObjectPtr<UNarrativeNodeBlueprintFactory> InNarrativeNodeBlueprintFactory)
	{
		NarrativeNodeBlueprintFactory = InNarrativeNodeBlueprintFactory;

		const TSharedRef<SWindow> Window = SNew(SWindow)
			.Title(LOCTEXT("CreateNarrativeNodeBlueprintOptions", "Pick Parent Class"))
			.ClientSize(FVector2D(400, 700))
			.SupportsMinimize(false).SupportsMaximize(false)
			[
				AsShared()
			];

		PickerWindow = Window;
		GEditor->EditorAddModalWindow(Window);
		
		NarrativeNodeBlueprintFactory.Reset();
		return bOkClicked;
	}

private:
	class FNarrativeNodeBlueprintParentFilter final : public IClassViewerFilter
	{
	public:
		/** All children of these classes will be included unless filtered out by another setting. */
		TSet<const UClass*> AllowedChildrenOfClasses;

		FNarrativeNodeBlueprintParentFilter() {}

		virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) override
		{
			// If it appears on the allowed child-of classes list (or there is nothing on that list)
			return InFilterFuncs->IfInChildOfClassesSet(AllowedChildrenOfClasses, InClass) != EFilterReturn::Failed;
		}

		virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef< const IUnloadedBlueprintData > InUnloadedClassData, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) override
		{
			// If it appears on the allowed child-of classes list (or there is nothing on that list)
			return InFilterFuncs->IfInChildOfClassesSet(AllowedChildrenOfClasses, InUnloadedClassData) != EFilterReturn::Failed;
		}
	};

	/** Creates the combo menu for the parent class */
	void MakeParentClassPicker()
	{
		// Load the Class Viewer module to display a class picker
		FClassViewerModule& ClassViewerModule = FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer");

		FClassViewerInitializationOptions Options;
		Options.Mode = EClassViewerMode::ClassPicker;
		Options.DisplayMode = EClassViewerDisplayMode::TreeView;
		Options.bIsBlueprintBaseOnly = true;

		const TSharedPtr<FNarrativeNodeBlueprintParentFilter> Filter = MakeShareable(new FNarrativeNodeBlueprintParentFilter);

		// All child child classes of UNarrativeNode are valid
		Filter->AllowedChildrenOfClasses.Add(UNarrativeNode::StaticClass());
		Options.ClassFilters = {Filter.ToSharedRef()};

		ParentClassContainer->ClearChildren();
		ParentClassContainer->AddSlot()
			[
				ClassViewerModule.CreateClassViewer(Options, FOnClassPicked::CreateSP(this, &SNarrativeNodeBlueprintCreateDialog::OnClassPicked))
			];
	}

	/** Handler for when a parent class is selected */
	void OnClassPicked(UClass* ChosenClass)
	{
		ParentClass = ChosenClass;
	}

	/** Handler for when ok is clicked */
	FReply OkClicked()
	{
		if (NarrativeNodeBlueprintFactory.IsValid())
		{
			NarrativeNodeBlueprintFactory->ParentClass = ParentClass.Get();
		}

		CloseDialog(true);

		return FReply::Handled();
	}

	void CloseDialog(bool bWasPicked = false)
	{
		bOkClicked = bWasPicked;
		if (PickerWindow.IsValid())
		{
			PickerWindow.Pin()->RequestDestroyWindow();
		}
	}

	/** Handler for when cancel is clicked */
	FReply CancelClicked()
	{
		CloseDialog();
		return FReply::Handled();
	}

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override
	{
		if (InKeyEvent.GetKey() == EKeys::Escape)
		{
			CloseDialog();
			return FReply::Handled();
		}
		return SWidget::OnKeyDown(MyGeometry, InKeyEvent);
	}

private:
	/** The factory for which we are setting up properties */
	TWeakObjectPtr<UNarrativeNodeBlueprintFactory> NarrativeNodeBlueprintFactory;

	/** A pointer to the window that is asking the user to select a parent class */
	TWeakPtr<SWindow> PickerWindow;

	/** The container for the Parent Class picker */
	TSharedPtr<SVerticalBox> ParentClassContainer;

	/** The selected class */
	TWeakObjectPtr<UClass> ParentClass;

	/** True if Ok was clicked */
	bool bOkClicked = false;
};

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

/*------------------------------------------------------------------------------
	UNarrativeNodeBlueprintFactory implementation
------------------------------------------------------------------------------*/

UNarrativeNodeBlueprintFactory::UNarrativeNodeBlueprintFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UNarrativeNodeBlueprint::StaticClass();
	ParentClass = UNarrativeNode::StaticClass();

	bCreateNew = true;
	bEditAfterNew = true;
}

bool UNarrativeNodeBlueprintFactory::ConfigureProperties()
{
	const TSharedRef<SNarrativeNodeBlueprintCreateDialog> Dialog = SNew(SNarrativeNodeBlueprintCreateDialog);
	return Dialog->ConfigureProperties(this);
}

UObject* UNarrativeNodeBlueprintFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext)
{
	check(Class->IsChildOf(UNarrativeNodeBlueprint::StaticClass()));

	if (ParentClass == nullptr || !FKismetEditorUtilities::CanCreateBlueprintOfClass(ParentClass) || !ParentClass->IsChildOf(UNarrativeNode::StaticClass()))
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("ClassName"), ParentClass ? FText::FromString(ParentClass->GetName()) : LOCTEXT("Null", "(null)"));
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("CannotCreateNarrativeNodeBlueprint", "Cannot create a Narrative Node Blueprint based on the class '{ClassName}'."), Args));
		return nullptr;
	}

	UNarrativeNodeBlueprint* NewBP = CastChecked<UNarrativeNodeBlueprint>(FKismetEditorUtilities::CreateBlueprint(ParentClass, InParent, Name, BPTYPE_Normal, UNarrativeNodeBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass(), CallingContext));

	if (NewBP && NewBP->UbergraphPages.Num() > 0)
	{
		const UBlueprintEditorSettings* Settings = GetMutableDefault<UBlueprintEditorSettings>();
		if(Settings && Settings->bSpawnDefaultBlueprintNodes)
		{
			int32 NodePositionY = 0;
			FKismetEditorUtilities::AddDefaultEventNode(NewBP, NewBP->UbergraphPages[0], FName(TEXT("K2_ExecuteInput")), UNarrativeNode::StaticClass(), NodePositionY);
			FKismetEditorUtilities::AddDefaultEventNode(NewBP, NewBP->UbergraphPages[0], FName(TEXT("K2_Cleanup")), UNarrativeNode::StaticClass(), NodePositionY);
		}
	}
	
	return NewBP;
}

UObject* UNarrativeNodeBlueprintFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return FactoryCreateNew(Class, InParent, Name, Flags, Context, Warn, NAME_None);
}

#undef LOCTEXT_NAMESPACE
