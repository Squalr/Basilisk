#include "MocapImporter/MocapImporterSubsystem.h"

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

bool UMocapImporterSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return true;
}

void UMocapImporterSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UMocapImporterSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UMocapImporterSubsystem::Tick(float DeltaTime)
{
}

bool UMocapImporterSubsystem::IsTickable() const
{
    // Prevent the Class Default Object from ticking, otherwise we basically have two subsystems
    return !IsTemplate(RF_ClassDefaultObject);
}

bool UMocapImporterSubsystem::IsTickableWhenPaused() const
{
    return true;
}

bool UMocapImporterSubsystem::IsTickableInEditor() const
{
    return true;
}

TStatId UMocapImporterSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UMocapImporterSubsystem, STATGROUP_Tickables);
}

void UMocapImporterSubsystem::ImportFaceAnimation(const FString& InFaceMocapExtractPath, TWeakObjectPtr<UFaceMocapEntryWidget> InSelectedFaceMocapEntry,
	TSoftObjectPtr<UObject> InMetahumanPerformanceImporter)
{
	if (!ensureAlways(InSelectedFaceMocapEntry.IsValid()))
	{
		return;
	}

	if (!ensureAlways(InMetahumanPerformanceImporter.IsPending() || InMetahumanPerformanceImporter.IsValid()))
	{
		return;
	}

	// Remove trailing date numbers
	static const FRegexPattern Pattern(TEXT("^[0-9]+_"));
	FString MocapName = InSelectedFaceMocapEntry->MocapName;
	FRegexMatcher Matcher(Pattern, MocapName);

	if (Matcher.FindNext())
	{
		MocapName = FString::Printf(TEXT("%s"), *MocapName.Replace(*Matcher.GetCaptureGroup(0), TEXT("")));
	}

	UClass* FootageCaptureDataClass = LoadObject<UClass>(nullptr, TEXT("/Script/MetaHumanCaptureData.FootageCaptureData"));
	if (!FootageCaptureDataClass)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to find the FootageCaptureData class."));
		return;
	}

	FString MocapPath = ConvertToProjectRelativePath(InFaceMocapExtractPath) / FString::Printf(TEXT("%s.%s"), *MocapName, *MocapName);
	UObject* MocapClipObject = StaticLoadObject(FootageCaptureDataClass, nullptr, *MocapPath);
	UObject* MetahumanPerformanceImporterInstance = InMetahumanPerformanceImporter.LoadSynchronous();

	if (!MocapClipObject)
	{
		UE_LOG(LogTemp, Error, TEXT("Unable to load footage. Check the name of the footage capture and make sure it matches the iPhone directory name."));
		return;
	}

	if (!ensureAlways(MetahumanPerformanceImporterInstance))
	{
		return;
	}

	// Open the metahuman performance importer in Unreal Editor
	GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(MetahumanPerformanceImporterInstance);

	// Enable to dump all properties and functions to log
	/*
	for (TFieldIterator<FProperty> PropIt(MetahumanPerformanceImporterInstance->GetClass()); PropIt; ++PropIt)
	{
		FProperty* Property = *PropIt;
		UE_LOG(LogTemp, Log, TEXT("Property Name: %s"), *Property->GetName());
	}

	for (TFieldIterator<UFunction> PropIt(MetahumanPerformanceImporterInstance->GetClass()); PropIt; ++PropIt)
	{
		UFunction* Property = *PropIt;
		UE_LOG(LogTemp, Log, TEXT("Function Name: %s"), *Property->GetName());
	}*/

	if (FObjectProperty* FootageCaptureData = FindFProperty<FObjectProperty>(MetahumanPerformanceImporterInstance->GetClass(), TEXT("FootageCaptureData")); ensureAlways(FootageCaptureData))
	{
		FootageCaptureData->SetPropertyValue_InContainer(MetahumanPerformanceImporterInstance, MocapClipObject);
		FPropertyChangedEvent PropertyEvent(FootageCaptureData);
		MetahumanPerformanceImporterInstance->PostEditChangeProperty(PropertyEvent);
	}

	FName NAME_OnProcessFaceAnimationComplete = TEXT("OnProcessFaceAnimationComplete");

	if (FMulticastDelegateProperty* OnProcessingFinishedDelegateProp = FindFProperty<FMulticastDelegateProperty>(MetahumanPerformanceImporterInstance->GetClass(), TEXT("OnProcessingFinishedDynamic")); ensureAlways(OnProcessingFinishedDelegateProp))
	{
		FScriptDelegate ScriptDelegate;
		ScriptDelegate.BindUFunction(this, NAME_OnProcessFaceAnimationComplete);
		OnProcessingFinishedDelegateProp->AddDelegate(ScriptDelegate, MetahumanPerformanceImporterInstance);
	}

	if (UFunction* StartPipelineFunc = MetahumanPerformanceImporterInstance->FindFunction(FName("StartPipeline")); ensureAlways(StartPipelineFunc))
	{
		MetahumanPerformanceImporterInstance->ProcessEvent(StartPipelineFunc, nullptr);
	}
}

void UMocapImporterSubsystem::OnProcessFaceAnimationComplete()
{
	// Crashy, forget it
	/*
	UObject* MetahumanPerformanceImporterInstance = MetahumanPerformanceImporter.LoadSynchronous();

	if (!ensureAlways(MetahumanPerformanceImporterInstance))
	{
		return;
	}

	if (UFunction* ExportAnimationFunc = MetahumanPerformanceImporterInstance->FindFunction(FName("ExportAnimation")); ensureAlways(ExportAnimationFunc))
	{
		uint8 EnumValue = 0;
		MetahumanPerformanceImporterInstance->ProcessEvent(ExportAnimationFunc, &EnumValue);
	}*/

	UE_LOG(LogTemp, Log, TEXT("Face motion capture processing complete! Please import it to the desired location."));
}

void UMocapImporterSubsystem::ImportBodyAnimation(const FString& InBodyMocapExtractPath, const FString& InTripletImportPath,
	TWeakObjectPtr<UBodyMocapEntryWidget> InSelectedBodyMocapEntry, TWeakObjectPtr<UMocapIdentityEntryWidget> InSelectedMocapIdentityEntry,
	TSoftObjectPtr<USkeleton> InUE4SkeletonRef, TSoftObjectPtr<USkeletalMesh> InUE4SkeletalMeshRef,
	TSoftObjectPtr<USkeletalMesh> InUE5SkeletalMeshRef, TSoftObjectPtr<UIKRetargeter> InUE4ToUE5Retargeter,
	TSoftObjectPtr<UIKRetargeter> InUE5ToMetahumanRetargeter)
{
	if (!ensureAlways(InSelectedBodyMocapEntry.IsValid()))
	{
		return;
	}

	if (!ensureAlways(InUE4SkeletonRef.IsPending() || InUE4SkeletonRef.IsValid()))
	{
		return;
	}

	if (!ensureAlways(InUE4SkeletalMeshRef.IsPending() || InUE4SkeletalMeshRef.IsValid()))
	{
		return;
	}

	if (!ensureAlways(InUE5SkeletalMeshRef.IsPending() || InUE5SkeletalMeshRef.IsValid()))
	{
		return;
	}

	if (!ensureAlways(InUE4ToUE5Retargeter.IsPending() || InUE4ToUE5Retargeter.IsValid()))
	{
		return;
	}

	if (!ensureAlways(InUE5ToMetahumanRetargeter.IsPending() || InUE5ToMetahumanRetargeter.IsValid()))
	{
		return;
	}

	if (!ensureAlways(InSelectedMocapIdentityEntry.IsValid()))
	{
		return;
	}

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	const TStrongObjectPtr<UFbxFactory> FbxFactory(NewObject<UFbxFactory>());
	const FString RelativeTripletPath = ConvertToProjectRelativePath(InTripletImportPath);

	FbxFactory->SetDetectImportTypeOnImport(false);

	FbxFactory->ImportUI->bImportTextures = false;
	FbxFactory->ImportUI->bCreatePhysicsAsset = false;
	FbxFactory->ImportUI->bImportMaterials = false;
	FbxFactory->ImportUI->bImportMesh = false;

	FbxFactory->ImportUI->SkeletalMeshImportData->bImportMorphTargets = true;
	FbxFactory->ImportUI->SkeletalMeshImportData->bConvertScene = true;
	FbxFactory->ImportUI->bImportAsSkeletal = true;
	FbxFactory->ImportUI->bImportAnimations = true;

	FbxFactory->ImportUI->Skeleton = InUE4SkeletonRef.LoadSynchronous();
	FbxFactory->ImportUI->MeshTypeToImport = FBXIT_Animation;

	UAutomatedAssetImportData* ImportData = NewObject<UAutomatedAssetImportData>(UAutomatedAssetImportData::StaticClass());
	ImportData->FactoryName = TEXT("FbxFactory");
	ImportData->Factory = FbxFactory.Get();
	ImportData->Filenames = { InBodyMocapExtractPath / InSelectedBodyMocapEntry->MocapName };
	ImportData->DestinationPath = InTripletImportPath;
	ImportData->bReplaceExisting = true;

	TArray<UObject*> ImportedAssets = AssetTools.ImportAssetsAutomated(ImportData);

	if (ImportedAssets.Num() > 1)
	{
		UE_LOG(LogTemp, Warning, TEXT("Unexpectedly found multiple imported assets. The converter will assume the first asset is the animation we want."));
	}

	TWeakObjectPtr<UAnimSequence> AnimSequence = Cast<UAnimSequence>(ImportedAssets.Num() >= 1 ? ImportedAssets[0] : nullptr);

	if (!AnimSequence.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("No animations were successfully imported"));
		return;
	}

	const TStrongObjectPtr<UIKRetargetBatchOperation> UE5BatchOperation(NewObject<UIKRetargetBatchOperation>());
	TArray<FAssetData> UE5Result = UE5BatchOperation->DuplicateAndRetarget(
		{ AnimSequence.Get() },
		InUE4SkeletalMeshRef.LoadSynchronous(),
		InUE5SkeletalMeshRef.LoadSynchronous(),
		InUE4ToUE5Retargeter.LoadSynchronous()
	);
	TWeakObjectPtr<UAnimationAsset> UE5Anim = Cast<UAnimationAsset>(UE5Result.Num() == 1 ? UE5Result[0].GetAsset() : nullptr);

	UEditorAssetLibrary::DeleteAsset(AnimSequence->GetPathName());

	if (!UE5Anim.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("No UE5 mannequin skeleton found."));
		return;
	}

	const TStrongObjectPtr<UIKRetargetBatchOperation> MetahumanBatchOperation(NewObject<UIKRetargetBatchOperation>());
	FIKRetargetBatchOperationContext BatchContext;

	BatchContext.AssetsToRetarget = { UE5Anim.Get() };
	BatchContext.NameRule.FolderPath = RelativeTripletPath;
	BatchContext.SourceMesh = InUE5SkeletalMeshRef.LoadSynchronous();
	BatchContext.TargetMesh = InSelectedMocapIdentityEntry->SkeletalMeshRef.LoadSynchronous();
	BatchContext.IKRetargetAsset = InUE5ToMetahumanRetargeter.LoadSynchronous();
	BatchContext.bRemapReferencedAssets = true;

	MetahumanBatchOperation->RunRetarget(BatchContext);

	UEditorAssetLibrary::DeleteAsset(UE5Anim->GetPathName());

	return;
}

void UMocapImporterSubsystem::ImportAudio(const FString& InFaceMocapExtractPath, const FString& InTripletImportPath, const FString& InCondaEnvPath,
	TObjectPtr<USoundClass> InDialogueSoundClass, TWeakObjectPtr<UFaceMocapEntryWidget> InSelectedFaceMocapEntry,
	TWeakObjectPtr<UMocapIdentityEntryWidget> InSelectedMocapIdentityEntry)
{
	if (!InSelectedFaceMocapEntry.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("No face mocap selected. Audio is imported from the face mocap."));
		return;
	}

	if (!InSelectedMocapIdentityEntry.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("No game identity selected."));
		return;
	}

	FString FaceMocapPath = InFaceMocapExtractPath / InSelectedFaceMocapEntry->MocapName;

	if (!FPaths::DirectoryExists(FaceMocapPath))
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid face mocap path, select the same directory to which you exported your iPhone Unreal Live Link capture data."));
		return;
	}

	TArray<FString> FileList;
	IFileManager& FileManager = IFileManager::Get();
	FString SearchPattern = FaceMocapPath;

	SearchPattern.Append(TEXT("/*.wav"));
	FileManager.FindFiles(FileList, *SearchPattern, true, false);

	if (FileList.Num() != 1)
	{
		UE_LOG(LogTemp, Error, TEXT("Expected a single audio source with the mocap data, but instead found %d."), FileList.Num());
		return;
	}

	static const FString ImportPrefix = TEXT("Dialogue_");
	FString AudioFileName = FileList[0];
	FString AudioFileNameNoExtension = FPaths::GetBaseFilename(AudioFileName);
	FString SourceAudioFile = FaceMocapPath / AudioFileName;
	FString DestinationAudioFile = InTripletImportPath / (ImportPrefix + AudioFileName);
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	FString PluginBaseDir = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*IPluginManager::Get().FindPlugin("Basilisk")->GetBaseDir());

	CachedDestinationAudioFile = DestinationAudioFile;
	CachedAudioFileNameNoExtension = ImportPrefix + AudioFileNameNoExtension;
	CachedTripletImportPath = InTripletImportPath;
	CachedCondaEnvPath = InCondaEnvPath;
	DialogueSoundClass = InDialogueSoundClass;

	if (!ensureAlways(PlatformFile.FileExists(*SourceAudioFile)))
	{
		return;
	}

	PlatformFile.CopyFile(*DestinationAudioFile, *SourceAudioFile);

	// Annoyingly resemble_ai's library depends on some Linux only libraries, so we need to execute this under a WSL environment. Please god end my suffering.
	FString DenoiseScriptPath = FPaths::Combine(*PluginBaseDir, TEXT("Content/Scripts/Denoiser/Denoise.py"));
	FString DenoiseArgs = FString::Printf(TEXT("'%s' '%s'"), *ConvertToWSLPath(DestinationAudioFile), *ConvertToWSLPath(DestinationAudioFile));

	if (!ensureAlways(FPaths::FileExists(DenoiseScriptPath)))
	{
		return;
	}

	if (!ExecutePythonWsl(ConvertToWSLPath(DenoiseScriptPath), DenoiseArgs))
	{
		UE_LOG(LogTemp, Error, TEXT("Denoiser failed with args %s"), *DenoiseArgs);
		return;
	}

	/*
	FString DenoiseScriptPath = FPaths::Combine(*PluginBaseDir, TEXT("Content/Scripts/Denoiser/DenoiseUvr.py"));
	FString DenoiseArgs = FString::Printf(TEXT("\"%s\" %s"), *DestinationAudioFile, *DestinationAudioFile);

	if (!ensureAlways(FPaths::FileExists(DenoiseScriptPath)))
	{
		return;
	}

	if (!ExecutePython(DenoiseScriptPath, TEXT("PantomimeVoice"), DenoiseArgs))
	{
		UE_LOG(LogTemp, Error, TEXT("Denoiser failed with args %s"), *DenoiseArgs);
		return;
	}
	*/

	FString TrimmerScriptPath = FPaths::Combine(*PluginBaseDir, TEXT("Content/Scripts/Trimmer/Trim.py"));
	FString TrimmerArgs = FString::Printf(TEXT("\"%s\" %s"), *DestinationAudioFile, *DestinationAudioFile);

	if (!ensureAlways(FPaths::FileExists(TrimmerScriptPath)))
	{
		return;
	}

	if (!ExecutePython(TrimmerScriptPath, TEXT("PantomimeVoice"), TrimmerArgs))
	{
		UE_LOG(LogTemp, Error, TEXT("Trimmer failed with args %s"), *TrimmerArgs);
		return;
	}

	/*
	FString ConverterScriptPath = FPaths::Combine(*PluginBaseDir, TEXT("Content/Scripts/Converter/ConvertElevenLabs.py"));
	FString ElevenLabsName = InSelectedMocapIdentityEntry->ElevenLabsName;
	float PitchCorrection = InSelectedMocapIdentityEntry->PitchCorrection;
	FString ConverterArgs = FString::Printf(TEXT("\"%s\" \"%s\" %s %.1f"), *DestinationAudioFile, *DestinationAudioFile, *ElevenLabsName, PitchCorrection);

	if (!ensureAlways(FPaths::FileExists(ConverterScriptPath)))
	{
		return;
	}

	if (!ExecutePython(ConverterScriptPath, TEXT("PantomimeVoice"), ConverterArgs))
	{
		UE_LOG(LogTemp, Error, TEXT("Converter failed with args %s"), *ConverterArgs);
		return;
	}
	*/

	FString ConverterScriptPath = FPaths::Combine(*PluginBaseDir, TEXT("Content/Scripts/Converter/ConvertRespeecher.py"));
	FString RespeecherName = InSelectedMocapIdentityEntry->RespeecherName;
	float PitchCorrection = InSelectedMocapIdentityEntry->PitchCorrection;
	FString ConverterArgs = FString::Printf(TEXT("\"%s\" \"%s\" %s %.1f"), *DestinationAudioFile, *DestinationAudioFile, *RespeecherName, PitchCorrection);

	if (!ensureAlways(FPaths::FileExists(ConverterScriptPath)))
	{
		return;
	}

	if (!ExecutePython(ConverterScriptPath, TEXT("PantomimeVoice"), ConverterArgs))
	{
		UE_LOG(LogTemp, Error, TEXT("Converter failed with args %s"), *ConverterArgs);
		return;
	}

	// Quality is sometimes sus after converting, enhance it // TODO: This enhancer is actually not very good though
	/*
	FString EnhanceScriptPath = FPaths::Combine(*PluginBaseDir, TEXT("Content/Scripts/Enhancer/Enhance.py"));
	FString EnhanceArgs = FString::Printf(TEXT("'%s' '%s'"), *ConvertToWSLPath(DestinationAudioFile), *ConvertToWSLPath(DestinationAudioFile));

	if (!ensureAlways(FPaths::FileExists(EnhanceScriptPath)))
	{
		return;
	}

	if (!ExecutePythonWsl(ConvertToWSLPath(EnhanceScriptPath), EnhanceArgs))
	{
		UE_LOG(LogTemp, Error, TEXT("Enhancer failed with args %s"), *EnhanceArgs);
		return;
	}*/
}

USoundWave* UMocapImporterSubsystem::ImportSoundWave(const FString& InSoundWavePackageName, const FString& InSoundWaveAssetName, const FString& InWavFilename) const
{
	// Find or create the package to host the sound wave
	UPackage* const SoundWavePackage = CreatePackage(*InSoundWavePackageName);
	if (!ensureAlways(SoundWavePackage))
	{
		return nullptr;
	}

	// Make sure the destination package is loaded
	SoundWavePackage->FullyLoad();

	// We set the correct options in the constructor, so run the import silently
	USoundFactory* SoundWaveFactory = NewObject<USoundFactory>();
	SoundWaveFactory->SuppressImportDialogs();

	// Perform the actual import 
	USoundWave* SoundWave = ImportObject<USoundWave>(SoundWavePackage, *InSoundWaveAssetName, RF_Public | RF_Standalone, *InWavFilename, nullptr, SoundWaveFactory);

	if (ensureAlways(SoundWave))
	{
		// RetainOnLoad prevents the dialogue from cutting out when a cutscene ends
		SoundWave->LoadingBehavior = ESoundWaveLoadingBehavior::RetainOnLoad;
		SoundWave->SoundClassObject = DialogueSoundClass;
		SoundWave->SoundGroup = ESoundGroup::SOUNDGROUP_Voice;

		// Compress to whatever formats the active target platforms want prior to saving the asset
		ITargetPlatformManagerModule* TargetPlatformManagerModule = GetTargetPlatformManager();
		if (TargetPlatformManagerModule)
		{
			const TArray<ITargetPlatform*>& Platforms = TargetPlatformManagerModule->GetActiveTargetPlatforms();
			for (ITargetPlatform* Platform : Platforms)
			{
				SoundWave->GetCompressedData(Platform->GetWaveFormat(SoundWave));
			}
		}

		FAssetRegistryModule& AssetRegistry = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		AssetRegistry.AssetCreated(SoundWave);
	}

	return SoundWave;
}

bool UMocapImporterSubsystem::ExecutePython(const FString& InScriptPath, const FString& InCondaEnv, const FString& InArgs)
{
	if (!FPaths::DirectoryExists(CachedCondaEnvPath))
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid python conda root set: %s"), *CachedCondaEnvPath);
		return false;
	}

	FString PythonExecutable = FString::Printf(TEXT("%s/%s/python.exe"), *CachedCondaEnvPath, *InCondaEnv);

	if (!FPaths::FileExists(PythonExecutable))
	{
		UE_LOG(LogTemp, Error, TEXT("Unable to find Conda environment at: %s"), *PythonExecutable);
		return false;
	}

	FString Command = FString::Printf(TEXT("\"%s\" %s"), *InScriptPath, *InArgs);

	int32 ReturnCode = 0;
	FString StdOut;
	FString StdErr;
	FPlatformProcess::ExecProcess(*PythonExecutable, *Command, &ReturnCode, &StdOut, &StdErr);

	if (ReturnCode == 0)
	{
		UE_LOG(LogTemp, Log, TEXT("Python Script in Conda Executed Successfully: %s"), *StdOut);
		return true;
	}

	UE_LOG(LogTemp, Error, TEXT("Execution Failed: %s"), *StdErr);
	return false;
}

bool UMocapImporterSubsystem::ExecutePythonWsl(const FString& InScriptPath, const FString& InArgs)
{
	// The path to the Python script inside WSL. Currently assumes a default WSL install location.
	FString WSLScriptPath = InScriptPath;
	int32 ReturnCode = 0;
	FString StdOut;
	FString StdErr;

	FString FullCommand = FString::Printf(TEXT("python3 %s %s"), *WSLScriptPath, *InArgs);
	FPlatformProcess::ExecProcess(TEXT("C:/Program Files/WSL/wsl.exe"), *FullCommand, &ReturnCode, &StdOut, &StdErr);

	if (ReturnCode == 0)
	{
		UE_LOG(LogTemp, Log, TEXT("Python Script Executed Successfully in WSL: %s"), *StdOut);
		return true;
	}

	UE_LOG(LogTemp, Error, TEXT("WSL Execution Failed: %s"), *StdErr);
	return false;
}

FString UMocapImporterSubsystem::ConvertToWSLPath(const FString& InWindowsPath)
{
	if (InWindowsPath.Len() < 3 || InWindowsPath[1] != ':' || InWindowsPath[2] != '/')
	{
		return InWindowsPath;
	}

	FString DriveLetter = InWindowsPath.Left(1).ToLower();
	FString WSLPath = "/mnt/" + DriveLetter + InWindowsPath.Mid(2);
	WSLPath.ReplaceInline(TEXT("\\"), TEXT("/"));

	return WSLPath;
}

FString UMocapImporterSubsystem::ConvertToProjectRelativePath(const FString& InAbsolutePath)
{
	FString AbsolutePath = InAbsolutePath;
	FString ProjectContentDir = FPaths::ProjectContentDir();
	FString RelativePath;

	if (FPaths::MakePathRelativeTo(AbsolutePath, *ProjectContentDir))
	{
		RelativePath = AbsolutePath;
	}

	return FString("/Game/") + RelativePath;
}
