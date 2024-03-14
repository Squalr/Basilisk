#include "MocapImporter/MocapImporterWidget.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Animation/AnimSequence.h"
#include "Components/VerticalBox.h"
#include "ContentBrowserModule.h"
#include "DesktopPlatformModule.h"
#include "DirectoryWatcherModule.h"
#include "EditorAssetLibrary.h"
#include "EditorDirectories.h"
#include "EditorUtilityWidgetComponents.h"
#include "Factories/FbxFactory.h"
#include "Factories/FbxImportUI.h"
#include "Factories/FbxTextureImportData.h"
#include "Factories/FbxSkeletalMeshImportData.h"
#include "Factories/SoundFactory.h"
#include "HAL/FileManager.h"
#include "IContentBrowserSingleton.h"
#include "IDesktopPlatform.h"
#include "Interfaces/IPluginManager.h"
#include "Internationalization/Regex.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "MocapImporter/BodyMocapEntryWidget.h"
#include "MocapImporter/FaceMocapEntryWidget.h"
#include "MocapImporter/MocapIdentityEntryWidget.h"
#include "MocapImporter/MocapImporterSubsystem.h"
#include "Retargeter/IKRetargeter.h"
#include "Retargeter/IKRetargetProcessor.h"
#include "RetargetEditor/IKRetargetBatchOperation.h"
#include "Runtime/Core/Public/HAL/PlatformProcess.h"
#include "Runtime/Core/Public/Misc/Paths.h"
#include "UObject/UObjectIterator.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"
#include "UObject/UnrealTypePrivate.h"
#include "Utils.h"
#include "Sound/SoundWave.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Tests/AutomationEditorCommon.h"

UMocapImporterWidget::UMocapImporterWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer), FaceMocapWidgetsPool(*this), BodyMocapWidgetsPool(*this)
{
	FString DesktopUserName = FPlatformProcess::UserName();
	FString DesktopUserDir = FPlatformProcess::UserDir();

	FaceMocapExtractPath = TEXT("Content/Temp/");
	BodyMocapExtractPath = FString(TEXT("C:/Users/")) + DesktopUserName + TEXT("/AppData/LocalLow/Rokoko Electronics/Rokoko Studio/Exports");
	PythonCondaEnvironmentsRootPath = FString(TEXT("C:/Users/")) + DesktopUserName + TEXT("anaconda3/envs/");
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

	if (ensure(SelectPythonCondaEnvironmentsRootPathButton))
	{
		SelectPythonCondaEnvironmentsRootPathButton->OnClicked.AddUniqueDynamic(this, &UMocapImporterWidget::OnSelectPythonCondaEnvironmentsRootPathButtonClicked);
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

	MocapIdentityEntries.Empty();

	if (ensure(MocapIdentitiesVerticalBox))
	{
		for (TWeakObjectPtr<UWidget> Next : MocapIdentitiesVerticalBox->GetAllChildren())
		{
			if (!Next.IsValid())
			{
				continue;
			}

			TWeakObjectPtr<UMocapIdentityEntryWidget> IdentityEntry = Cast<UMocapIdentityEntryWidget>(Next);

			if (IdentityEntry.IsValid())
			{
				MocapIdentityEntries.Add(IdentityEntry);
				IdentityEntry->OnMocapIdentityEntryClickedEvent.AddUniqueDynamic(this, &UMocapImporterWidget::OnMocapIdentityEntrySelected);
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

void UMocapImporterWidget::OnMocapIdentityEntrySelected(UMocapIdentityEntryWidget* InMocapIdentityEntry)
{
	SelectedMocapIdentityEntry = InMocapIdentityEntry;

	for (TWeakObjectPtr<UMocapIdentityEntryWidget> MocapIdentityEntry : MocapIdentityEntries)
	{
		if (MocapIdentityEntry.IsValid())
		{
			MocapIdentityEntry->SetIsSelected(MocapIdentityEntry == InMocapIdentityEntry);
		}
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

void UMocapImporterWidget::OnSelectPythonCondaEnvironmentsRootPathButtonClicked()
{
	if (IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get(); ensure(DesktopPlatform))
	{
		FString OutFolder;
		const void* ParentWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);
		const FString Title = TEXT("Choose Conda Environments Root Folder");
		const FString DefaultLocation(FEditorDirectories::Get().GetLastDirectory(ELastDirectory::GENERIC_IMPORT));

		const bool bFolderSelected = DesktopPlatform->OpenDirectoryDialog(
			ParentWindowHandle,
			Title,
			DefaultLocation,
			OutFolder
		);

		if (bFolderSelected)
		{
			PythonCondaEnvironmentsRootPath = OutFolder;
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
		Subsystem->ImportFaceAnimation(FaceMocapExtractPath, SelectedFaceMocapEntry, MetahumanPerformanceImporter);
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
		Subsystem->ImportBodyAnimation(BodyMocapExtractPath, TripletImportPath, SelectedBodyMocapEntry, SelectedMocapIdentityEntry,
			UE4SkeletonRef, UE4SkeletalMeshRef, UE5SkeletalMeshRef, UE4ToUE5Retargeter, UE5ToMetahumanRetargeter);
	}
}

void UMocapImporterWidget::OnImportVoiceButtonClicked()
{
	if (!ensureAlways(FPaths::DirectoryExists(TripletImportPath)))
	{
		return;
	}

	if (TObjectPtr<UMocapImporterSubsystem> Subsystem = GEditor->GetEditorSubsystem<UMocapImporterSubsystem>(); ensure(Subsystem))
	{
		Subsystem->ImportAudio(FaceMocapExtractPath, TripletImportPath, PythonCondaEnvironmentsRootPath, DialogueSoundClass, SelectedFaceMocapEntry, SelectedMocapIdentityEntry);
	}
}
