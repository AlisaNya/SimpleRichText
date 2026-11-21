// Copyright 2026 UnrealEssence. All Rights Reserved.

#include "SimpleRichTextEditor.h"
#include "SimpleRichTextBlockCustomization.h"
#include "SimpleRichTextBlock.h"

#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"

#define LOCTEXT_NAMESPACE "FSimpleRichTextEditorModule"

void FSimpleRichTextEditorModule::StartupModule()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomClassLayout(
		USimpleRichTextBlock::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FSimpleRichTextBlockCustomization::MakeInstance));
	PropertyModule.NotifyCustomizationModuleChanged();
}

void FSimpleRichTextEditorModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomClassLayout(USimpleRichTextBlock::StaticClass()->GetFName());
		PropertyModule.NotifyCustomizationModuleChanged();
	}
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FSimpleRichTextEditorModule, SimpleRichTextEditor)