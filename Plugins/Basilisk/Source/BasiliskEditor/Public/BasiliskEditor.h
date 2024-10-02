#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "UObject/StrongObjectPtr.h"
#include "Async/Future.h"
#include "Delegates/IDelegateInstance.h"
#include "Templates/UniquePtr.h"

class FBasiliskEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
