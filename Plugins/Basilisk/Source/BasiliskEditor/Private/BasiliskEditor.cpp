#include "BasiliskEditor.h"

#include "SubtitleCreator/SubtitleContextMenuActions.h"

void FBasiliskEditorModule::StartupModule()
{
	FSubtitleContextMenuActions::RegisterMenus();
}

void FBasiliskEditorModule::ShutdownModule()
{
    if (!UObjectInitialized())
    {
        return;
    }
}

IMPLEMENT_MODULE(FBasiliskEditorModule, BasiliskEditor)
