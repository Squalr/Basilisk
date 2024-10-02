#include "MocapImporter/MocapImporterWidget.h"

#include "Components/VerticalBox.h"
#include "ContentBrowserModule.h"
#include "DesktopPlatformModule.h"
#include "DirectoryWatcherModule.h"
#include "EditorAssetLibrary.h"
#include "EditorDirectories.h"
#include "EditorUtilityWidgetComponents.h"
#include "HAL/FileManager.h"
#include "IContentBrowserSingleton.h"
#include "Internationalization/Regex.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "MocapImporter/ActorIdentityEntryWidget.h"
#include "MocapImporter/BodyMocapEntryWidget.h"
#include "MocapImporter/FaceMocapEntryWidget.h"
#include "MocapImporter/MetahumanIdentityEntryWidget.h"
#include "MocapImporter/MocapImporterSubsystem.h"
#include "Runtime/Core/Public/HAL/PlatformProcess.h"
#include "Runtime/Core/Public/Misc/Paths.h"
#include "Sound/SoundWave.h"

#pragma optimize("", off)

UMocapImporterWidget::UMocapImporterWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer), FaceMocapWidgetsPool(*this), BodyMocapWidgetsPool(*this)
{
	FString DesktopUserName = FPlatformProcess::UserName();
	FString DesktopUserDir = FPlatformProcess::UserDir();

	FaceMocapExtractPath = TEXT("Content/Temp/");
	BodyMocapExtractPath = FString(TEXT("C:/Users/")) + DesktopUserName + TEXT("/AppData/LocalLow/Rokoko Electronics/Rokoko Studio/Exports");
	TripletImportPath = TEXT("Content/Temp/");
}

void UMocapImporterWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (ensure(SelectFaceMocapPathButton))
	{
		SelectFaceMocapPathButton->OnClicked.AddUniqueDynamic(this, &UMocapImporterWidget::OnSelectFaceMocapPathClicked);
	}

	if (ensure(SelectBodyMocapPathButton))
	{
		SelectBodyMocapPathButton->OnClicked.AddUniqueDynamic(this, &UMocapImporterWidget::OnSelectBodyMocapPathButtonClicked);
	}

	if (ensure(SelectImportPathButton))
	{
		SelectImportPathButton->OnClicked.AddUniqueDynamic(this, &UMocapImporterWidget::OnSelectImportPathButtonClicked);
	}

	if (ensure(AutoPathButton))
	{
		AutoPathButton->OnClicked.AddUniqueDynamic(this, &UMocapImporterWidget::OnAutoPathButtonButtonClicked);
	}

	if (ensure(ImportFaceMocapButton))
	{
		ImportFaceMocapButton->OnClicked.AddUniqueDynamic(this, &UMocapImporterWidget::OnImportFaceMocapButtonClicked);
	}

	if (ensure(ImportBodyMocapButton))
	{
		ImportBodyMocapButton->OnClicked.AddUniqueDynamic(this, &UMocapImporterWidget::OnImportBodyMocapButtonClicked);
	}

	if (ensure(ImportVoiceButton))
	{
		ImportVoiceButton->OnClicked.AddUniqueDynamic(this, &UMocapImporterWidget::OnImportVoiceButtonClicked);
	}

	LoadConfig();

	// Force refresh to start directory watchers
	SetFaceMocapDirectory(FaceMocapExtractPath);
	SetBodyMocapDirectory(BodyMocapExtractPath);

	if (TObjectPtr<UMocapImporterSubsystem> Subsystem = GEditor->GetEditorSubsystem<UMocapImporterSubsystem>(); ensure(Subsystem))
	{
		FString ProjectDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());

		// Set Path to a default path if it is not under the project directory
		if (!FPaths::IsUnderDirectory(TripletImportPath, ProjectDir))
		{
			Subsystem->SetTripletImportPath(TEXT("Content/Temp/"));
		}
		else
		{
			Subsystem->SetTripletImportPath(TripletImportPath);
		}
		
		Subsystem->SetDialogueSoundClass(DialogueSoundClass);
	}

	MetahumanIdentityEntries.Empty();

	if (ensure(MocapIdentitiesVerticalBox))
	{
		for (TWeakObjectPtr<UWidget> Next : MocapIdentitiesVerticalBox->GetAllChildren())
		{
			if (!Next.IsValid())
			{
				continue;
			}

			TWeakObjectPtr<UMetahumanIdentityEntryWidget> IdentityEntry = Cast<UMetahumanIdentityEntryWidget>(Next);

			if (IdentityEntry.IsValid())
			{
				MetahumanIdentityEntries.Add(IdentityEntry);
				IdentityEntry->OnMetahumanIdentityEntryClickedEvent.AddUniqueDynamic(this, &UMocapImporterWidget::OnMetahumanIdentityEntrySelected);
			}
		}
	}

	ActorIdentityEntries.Empty();

	if (ensure(ActorIdentitiesVerticalBox))
	{
		for (TWeakObjectPtr<UWidget> Next : ActorIdentitiesVerticalBox->GetAllChildren())
		{
			if (!Next.IsValid())
			{
				continue;
			}

			TWeakObjectPtr<UActorIdentityEntryWidget> IdentityEntry = Cast<UActorIdentityEntryWidget>(Next);

			if (IdentityEntry.IsValid())
			{
				ActorIdentityEntries.Add(IdentityEntry);
				IdentityEntry->OnActorIdentityEntryClickedEvent.AddUniqueDynamic(this, &UMocapImporterWidget::OnActorIdentityEntrySelected);
			}
		}
	}
}

void UMocapImporterWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	FaceMocapWidgetsPool.ReleaseAllSlateResources();
	BodyMocapWidgetsPool.ReleaseAllSlateResources();
}

void UMocapImporterWidget::SetFaceMocapDirectory(const FString& InFaceMocapDirectory)
{
	FDirectoryWatcherModule& DirectoryWatcherModule = FModuleManager::LoadModuleChecked<FDirectoryWatcherModule>(TEXT("DirectoryWatcher"));
	IDirectoryWatcher* DirectoryWatcher = DirectoryWatcherModule.Get();

	if (ensure(DirectoryWatcher))
	{
		// Stop old watch
		DirectoryWatcher->UnregisterDirectoryChangedCallback_Handle(FaceMocapExtractPath, BodyMocapDirectoryChangedDelegateHandle);

		FaceMocapExtractPath = InFaceMocapDirectory;

		// Start new watch
		if (FPaths::DirectoryExists(FaceMocapExtractPath))
		{
			FaceMocapDirectoryChangedDelegate = IDirectoryWatcher::FDirectoryChanged::CreateUObject(this, &UMocapImporterWidget::HandleFaceMocapDirectoryChanged);
			DirectoryWatcher->RegisterDirectoryChangedCallback_Handle(FaceMocapExtractPath, FaceMocapDirectoryChangedDelegate, FaceMocapDirectoryChangedDelegateHandle);
		}
	}

	RebuildFaceMocapList();
}

void UMocapImporterWidget::SetBodyMocapDirectory(const FString& InBodyMocapDirectory)
{
	FDirectoryWatcherModule& DirectoryWatcherModule = FModuleManager::LoadModuleChecked<FDirectoryWatcherModule>(TEXT("DirectoryWatcher"));
	IDirectoryWatcher* DirectoryWatcher = DirectoryWatcherModule.Get();

	if (ensure(DirectoryWatcher))
	{
		// Stop old watch
		DirectoryWatcher->UnregisterDirectoryChangedCallback_Handle(BodyMocapExtractPath, BodyMocapDirectoryChangedDelegateHandle);

		BodyMocapExtractPath = InBodyMocapDirectory;

		// Start new watch
		if (FPaths::DirectoryExists(BodyMocapExtractPath))
		{
			BodyMocapDirectoryChangedDelegate = IDirectoryWatcher::FDirectoryChanged::CreateUObject(this, &UMocapImporterWidget::HandleBodyMocapDirectoryChanged);
			DirectoryWatcher->RegisterDirectoryChangedCallback_Handle(BodyMocapExtractPath, BodyMocapDirectoryChangedDelegate, BodyMocapDirectoryChangedDelegateHandle);
		}
	}

	RebuildBodyMocapList();
}

void UMocapImporterWidget::HandleFaceMocapDirectoryChanged(const TArray<FFileChangeData>& InFileChanges)
{
	RebuildFaceMocapList();
}

void UMocapImporterWidget::HandleBodyMocapDirectoryChanged(const TArray<FFileChangeData>& InFileChanges)
{
	RebuildBodyMocapList();
}

void UMocapImporterWidget::RebuildFaceMocapList()
{
	for (TObjectPtr<UFaceMocapEntryWidget> FaceMocapEntry : FaceMocapEntries)
	{
		FaceMocapWidgetsPool.Release(FaceMocapEntry, true);
	}

	FaceMocapEntries.Empty();

	if (!FPaths::DirectoryExists(FaceMocapExtractPath))
	{
		UE_LOG(LogTemp, Error, TEXT("Unable to find face mocap extract path: "), *FaceMocapExtractPath);
		return;
	}

	TArray<FString> DirectoryList;
	IFileManager& FileManager = IFileManager::Get();

	FileManager.FindFiles(DirectoryList, *(FaceMocapExtractPath / TEXT("*")), false, true);

	FaceMocapVerticalBox->ClearChildren();

	for (const FString& Directory : DirectoryList)
	{
		if (TObjectPtr<UFaceMocapEntryWidget> WidgetInstance = FaceMocapWidgetsPool.GetOrCreateInstance(FaceMocapEntryWidgetClass))
		{
			FaceMocapEntries.Add(WidgetInstance);

			if (ensure(FaceMocapVerticalBox))
			{
				FaceMocapVerticalBox->AddChildToVerticalBox(WidgetInstance);
				WidgetInstance->Setup(Directory);
				WidgetInstance->OnMocapEntryClickedEvent.AddUniqueDynamic(this, &UMocapImporterWidget::OnFaceMocapEntrySelected);
			}
		}
	}
}

void UMocapImporterWidget::RebuildBodyMocapList()
{
	for (TObjectPtr<UBodyMocapEntryWidget> BodyMocapEntry : BodyMocapEntries)
	{
		BodyMocapWidgetsPool.Release(BodyMocapEntry, true);
	}

	BodyMocapEntries.Empty();

	if (!FPaths::DirectoryExists(BodyMocapExtractPath))
	{
		UE_LOG(LogTemp, Error, TEXT("Unable to find body mocap extract path: "), *BodyMocapExtractPath);
		return;
	}

	TArray<FString> FileList;
	IFileManager& FileManager = IFileManager::Get();
	FString SearchPattern = BodyMocapExtractPath;

	SearchPattern.Append(TEXT("/*.fbx"));
	FileManager.FindFiles(FileList, *SearchPattern, true, false);

	BodyMocapVerticalBox->ClearChildren();

	for (const FString& File : FileList)
	{
		if (TObjectPtr<UBodyMocapEntryWidget> WidgetInstance = BodyMocapWidgetsPool.GetOrCreateInstance(BodyMocapEntryWidgetClass))
		{
			BodyMocapEntries.Add(WidgetInstance);

			if (ensure(BodyMocapVerticalBox))
			{
				BodyMocapVerticalBox->AddChildToVerticalBox(WidgetInstance);
				WidgetInstance->Setup(File);
				WidgetInstance->OnMocapEntryClickedEvent.AddUniqueDynamic(this, &UMocapImporterWidget::OnBodyMocapEntrySelected);
			}
		}
	}
}

void UMocapImporterWidget::OnFaceMocapEntrySelected(UFaceMocapEntryWidget* InMocapEntry)
{
	SelectedFaceMocapEntry = InMocapEntry;

	for (TObjectPtr<UFaceMocapEntryWidget> FaceMocapEntry : FaceMocapEntries)
	{
		if (FaceMocapEntry)
		{
			FaceMocapEntry->SetIsSelected(FaceMocapEntry == InMocapEntry);
		}
	}
}

void UMocapImporterWidget::OnBodyMocapEntrySelected(UBodyMocapEntryWidget* InMocapEntry)
{
	SelectedBodyMocapEntry = InMocapEntry;

	for (TObjectPtr<UBodyMocapEntryWidget> BodyMocapEntry : BodyMocapEntries)
	{
		if (BodyMocapEntry)
		{
			BodyMocapEntry->SetIsSelected(BodyMocapEntry == InMocapEntry);
		}
	}
}

void UMocapImporterWidget::OnMetahumanIdentityEntrySelected(UMetahumanIdentityEntryWidget* InMetahumanIdentityEntry)
{
	SelectedMetahumanIdentityEntry = InMetahumanIdentityEntry;

	for (TWeakObjectPtr<UMetahumanIdentityEntryWidget> MetahumanIdentityEntry : MetahumanIdentityEntries)
	{
		if (MetahumanIdentityEntry.IsValid())
		{
			MetahumanIdentityEntry->SetIsSelected(MetahumanIdentityEntry == InMetahumanIdentityEntry);
		}
	}

	if (TObjectPtr<UMocapImporterSubsystem> Subsystem = GEditor->GetEditorSubsystem<UMocapImporterSubsystem>(); ensure(Subsystem))
	{
		Subsystem->SetActiveMetahumanIdentity(SelectedMetahumanIdentityEntry);
	}
}

void UMocapImporterWidget::OnActorIdentityEntrySelected(UActorIdentityEntryWidget* InActorIdentityEntry)
{
	SelectedActorIdentityEntry = InActorIdentityEntry;

	for (TWeakObjectPtr<UActorIdentityEntryWidget> ActorIdentityEntry : ActorIdentityEntries)
	{
		if (ActorIdentityEntry.IsValid())
		{
			ActorIdentityEntry->SetIsSelected(ActorIdentityEntry == InActorIdentityEntry);
		}
	}

	if (TObjectPtr<UMocapImporterSubsystem> Subsystem = GEditor->GetEditorSubsystem<UMocapImporterSubsystem>(); ensure(Subsystem))
	{
		Subsystem->SetActiveActorIdentity(InActorIdentityEntry);
	}
}

void UMocapImporterWidget::OnSelectFaceMocapPathClicked()
{
	if (IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get(); ensure(DesktopPlatform))
	{
		FString OutFolder;
		const void* ParentWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);
		const FString Title = TEXT("Choose iPhone Mocap Export Folder");
		const FString DefaultLocation(FEditorDirectories::Get().GetLastDirectory(ELastDirectory::GENERIC_IMPORT));

		const bool bFolderSelected = DesktopPlatform->OpenDirectoryDialog(
			ParentWindowHandle,
			Title,
			DefaultLocation,
			OutFolder
		);

		if (bFolderSelected)
		{
			SetFaceMocapDirectory(OutFolder);
			SaveConfig();
		}
	}
}

void UMocapImporterWidget::OnSelectBodyMocapPathButtonClicked()
{
	if (IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get(); ensure(DesktopPlatform))
	{
		FString OutFolder;
		const void* ParentWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);
		const FString Title = TEXT("Choose Rokoko Export Folder");
		const FString DefaultLocation(FEditorDirectories::Get().GetLastDirectory(ELastDirectory::GENERIC_IMPORT));

		const bool bFolderSelected = DesktopPlatform->OpenDirectoryDialog(
			ParentWindowHandle,
			Title,
			DefaultLocation,
			OutFolder
		);

		if (bFolderSelected)
		{
			SetBodyMocapDirectory(OutFolder);
			SaveConfig();
		}
	}
}

void UMocapImporterWidget::OnSelectImportPathButtonClicked()
{
	if (IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get(); ensure(DesktopPlatform))
	{
		FString OutFolder;
		const void* ParentWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);
		const FString Title = TEXT("Choose Triplet Import Folder");
		const FString DefaultLocation(FEditorDirectories::Get().GetLastDirectory(ELastDirectory::GENERIC_IMPORT));

		const bool bFolderSelected = DesktopPlatform->OpenDirectoryDialog(
			ParentWindowHandle,
			Title,
			DefaultLocation,
			OutFolder
		);

		if (bFolderSelected)
		{
			TripletImportPath = OutFolder;

			if (TObjectPtr<UMocapImporterSubsystem> Subsystem = GEditor->GetEditorSubsystem<UMocapImporterSubsystem>(); ensure(Subsystem))
			{
				Subsystem->SetTripletImportPath(TripletImportPath);
			}

			SaveConfig();
		}
	}
}

void UMocapImporterWidget::OnAutoPathButtonButtonClicked()
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	TArray<FString> SelectedFolders;
	ContentBrowserModule.Get().GetSelectedPathViewFolders(SelectedFolders);

	if (SelectedFolders.Num() > 0)
	{
		FString BaseDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
		FString FocusedFolderPath = SelectedFolders[0].Replace(TEXT("/All/Game/"), *BaseDir);

		TripletImportPath = FocusedFolderPath;

		if (TObjectPtr<UMocapImporterSubsystem> Subsystem = GEditor->GetEditorSubsystem<UMocapImporterSubsystem>(); ensure(Subsystem))
		{
			Subsystem->SetTripletImportPath(TripletImportPath);
		}

		SaveConfig();
	}
}

void UMocapImporterWidget::OnImportFaceMocapButtonClicked()
{
	if (!ensureAlways(FPaths::DirectoryExists(TripletImportPath)))
	{
		return;
	}

	if (TObjectPtr<UMocapImporterSubsystem> Subsystem = GEditor->GetEditorSubsystem<UMocapImporterSubsystem>(); ensure(Subsystem))
	{
		Subsystem->ImportFaceAnimation(FaceMocapExtractPath, SelectedFaceMocapEntry, SelectedActorIdentityEntry);
	}
}

void UMocapImporterWidget::OnImportBodyMocapButtonClicked()
{
	if (!ensureAlways(FPaths::DirectoryExists(TripletImportPath)))
	{
		return;
	}

	if (TObjectPtr<UMocapImporterSubsystem> Subsystem = GEditor->GetEditorSubsystem<UMocapImporterSubsystem>(); ensure(Subsystem))
	{
		Subsystem->ImportBodyAnimation(BodyMocapExtractPath, TripletImportPath, SelectedBodyMocapEntry, SelectedMetahumanIdentityEntry);
	}
}

void UMocapImporterWidget::OnImportVoiceButtonClicked()
{
	if (!ensureAlways(FPaths::DirectoryExists(TripletImportPath)))
	{
		return;
	}

	if (!ensureAlways(SelectedFaceMocapEntry.IsValid()))
	{
		return;
	}
	
	if (TObjectPtr<UMocapImporterSubsystem> Subsystem = GEditor->GetEditorSubsystem<UMocapImporterSubsystem>(); ensure(Subsystem))
	{
		const FRegexPattern Pattern(TEXT("^[0-9]+_?"));
		FString MovFileName = SelectedFaceMocapEntry->MocapName + TEXT("_iPhone.mov");
		FRegexMatcher Matcher(Pattern, MovFileName);

		if (Matcher.FindNext())
		{
			int32 EndPosition = Matcher.GetMatchEnding();
			MovFileName = MovFileName.RightChop(EndPosition);
		}

		FString MovFilePath = FaceMocapExtractPath / SelectedFaceMocapEntry->MocapName / MovFileName;
		Subsystem->ImportAudioMocap(MovFilePath);
	}
}

#pragma optimize("", on)
