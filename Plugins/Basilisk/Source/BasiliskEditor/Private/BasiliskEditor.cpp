#include "BasiliskEditor.h"

void FBasiliskEditorModule::StartupModule()
{
}

void FBasiliskEditorModule::ShutdownModule()
{
    if (!UObjectInitialized())
    {
        return;
    }
}

IMPLEMENT_MODULE(FBasiliskEditorModule, BasiliskEditor)
