// Copyright XiaoYao

#pragma once

#include "Modules/ModuleInterface.h"

class FNarrativeModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
