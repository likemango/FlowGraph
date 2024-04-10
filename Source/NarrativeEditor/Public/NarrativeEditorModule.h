// Copyright XiaoYao

#pragma once

#include "AssetTypeCategories.h"
#include "IAssetTypeActions.h"
#include "Modules/ModuleInterface.h"
#include "PropertyEditorDelegates.h"
#include "Toolkits/IToolkit.h"

class FSlateStyleSet;
class FToolBarBuilder;
struct FGraphPanelPinConnectionFactory;

class FNarrativeAssetEditor;
class UNarrativeAsset;

class NARRATIVEEDITOR_API FNarrativeEditorModule : public IModuleInterface
{
public:
	static EAssetTypeCategories::Type NarrativeAssetCategory;

private:
	TArray<TSharedRef<IAssetTypeActions>> RegisteredAssetActions;
	TSet<FName> CustomClassLayouts;
	TSet<FName> CustomStructLayouts;

public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void RegisterAssets();
	void UnregisterAssets();

	void RegisterDetailCustomizations();
	void UnregisterDetailCustomizations();

	void RegisterCustomClassLayout(const TSubclassOf<UObject> Class, const FOnGetDetailCustomizationInstance DetailLayout);
	void RegisterCustomStructLayout(const UScriptStruct& Struct, const FOnGetPropertyTypeCustomizationInstance DetailLayout);

public:
	FDelegateHandle NarrativeTrackCreateEditorHandle;
	FDelegateHandle ModulesChangedHandle;

private:
	void ModulesChangesCallback(FName ModuleName, EModuleChangeReason ReasonForChange) const;
	void RegisterAssetIndexers() const;

	void CreateNarrativeToolbar(FToolBarBuilder& ToolbarBuilder) const;

public:
	static TSharedRef<FNarrativeAssetEditor> CreateNarrativeAssetEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UNarrativeAsset* NarrativeAsset);
};
