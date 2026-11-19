// Copyright UnrealEssence. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

class FSimpleRichTextModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
