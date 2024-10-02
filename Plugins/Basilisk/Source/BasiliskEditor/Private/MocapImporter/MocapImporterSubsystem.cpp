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
#include "HttpModule.h"
#include "Factories/FbxFactory.h"
#include "Factories/FbxImportUI.h"
#include "Factories/FbxTextureImportData.h"
#include "Factories/FbxSkeletalMeshImportData.h"
#include "Factories/SoundFactory.h"
#include "HAL/FileManager.h"
#include "IContentBrowserSingleton.h"
#include "IDesktopPlatform.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Interfaces/IPluginManager.h"
#include "Internationalization/Regex.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "MocapImporter/ActorIdentityEntryWidget.h"
#include "MocapImporter/BodyMocapEntryWidget.h"
#include "MocapImporter/FaceMocapEntryWidget.h"
#include "MocapImporter/MetahumanIdentityEntryWidget.h"
#include "MocapImporter/ProceduralRetargetAssets.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "RuntimeAudioImporterLibrary.h"
#include "Runtime/Core/Public/HAL/PlatformProcess.h"
#include "Runtime/Core/Public/Misc/Paths.h"
#include "Sound/SoundWave.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "UObject/UObjectIterator.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"
#include "UObject/UnrealTypePrivate.h"
#include "Utils.h"
#include "Tests/AutomationEditorCommon.h"
#include "VoiceConverter/VoiceConverterSubsystem.h"

#pragma optimize("", off)

bool UMocapImporterSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return GIsEditor;
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

void UMocapImporterSubsystem::SetTripletImportPath(const FString& InTripletImportPath)
{
	TripletImportPath = InTripletImportPath;
}

FString UMocapImporterSubsystem::GetTripletImportPath() const
{
	return TripletImportPath;
}

void UMocapImporterSubsystem::SetActiveMetahumanIdentity(TWeakObjectPtr<UMetahumanIdentityEntryWidget> InMetahumanIdentity)
{
	ActiveMetahumanIdentity = InMetahumanIdentity;
}

TWeakObjectPtr<UMetahumanIdentityEntryWidget> UMocapImporterSubsystem::GetActiveMetahumanIdentity() const
{
	return ActiveMetahumanIdentity;
}

void UMocapImporterSubsystem::SetActiveActorIdentity(TWeakObjectPtr<UActorIdentityEntryWidget> InActorIdentity)
{
	ActiveActorIdentity = InActorIdentity;
}

TWeakObjectPtr<UActorIdentityEntryWidget> UMocapImporterSubsystem::GetActiveActorIdentity() const
{
	return ActiveActorIdentity;
}

void UMocapImporterSubsystem::SetDialogueSoundClass(TObjectPtr<USoundClass> InDialogueSoundClass)
{
	DialogueSoundClass = InDialogueSoundClass;
}

TObjectPtr<USoundClass> UMocapImporterSubsystem::GetDialogueSoundClass() const
{
	return DialogueSoundClass;
}

void UMocapImporterSubsystem::ImportFaceAnimation(const FString& InFaceMocapExtractPath, TWeakObjectPtr<UFaceMocapEntryWidget> InSelectedFaceMocapEntry,
	TWeakObjectPtr<UActorIdentityEntryWidget> InActorIdentityEntryWidget)
{
	if (!ensureAlways(InSelectedFaceMocapEntry.IsValid()))
	{
		return;
	}

	if (!ensureAlways(InActorIdentityEntryWidget.IsValid()))
	{
		return;
	}

	TSoftObjectPtr<UObject> MetahumanPerformanceImporter = InActorIdentityEntryWidget->MetahumanPerformanceImporter;

	if (!ensureAlways(MetahumanPerformanceImporter.IsPending() || MetahumanPerformanceImporter.IsValid()))
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
	UObject* MetahumanPerformanceImporterInstance = MetahumanPerformanceImporter.LoadSynchronous();

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
		// MetahumanPerformanceImporterInstance->ProcessEvent(StartPipelineFunc, nullptr);
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
	TWeakObjectPtr<UBodyMocapEntryWidget> InSelectedBodyMocapEntry, TWeakObjectPtr<UMetahumanIdentityEntryWidget> InSelectedMetahumanIdentityEntry)
{
	if (!ensureAlways(InSelectedBodyMocapEntry.IsValid()))
	{
		return;
	}

	if (!ensureAlways(InSelectedMetahumanIdentityEntry.IsValid()))
	{
		return;
	}

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	const TStrongObjectPtr<UFbxFactory> FbxFactory(NewObject<UFbxFactory>());
	const FString RelativeTripletPath = ConvertToProjectRelativePath(InTripletImportPath);

	FbxFactory->SetDetectImportTypeOnImport(true);

	FbxFactory->ImportUI->bImportTextures = false;
	FbxFactory->ImportUI->bImportMaterials = false;

	FbxFactory->ImportUI->bCreatePhysicsAsset = true;
	FbxFactory->ImportUI->SkeletalMeshImportData->bImportMorphTargets = true;
	FbxFactory->ImportUI->SkeletalMeshImportData->bConvertScene = true;
	FbxFactory->ImportUI->bImportAsSkeletal = true;
	FbxFactory->ImportUI->bImportAnimations = true;
	FbxFactory->ImportUI->bImportMesh = true;
	FbxFactory->ImportUI->MeshTypeToImport = FBXIT_SkeletalMesh;

	UAutomatedAssetImportData* ImportData = NewObject<UAutomatedAssetImportData>(UAutomatedAssetImportData::StaticClass());
	ImportData->FactoryName = TEXT("FbxFactory");
	ImportData->Factory = FbxFactory.Get();
	ImportData->Filenames = { InBodyMocapExtractPath / InSelectedBodyMocapEntry->MocapName };
	ImportData->DestinationPath = InTripletImportPath;
	ImportData->bReplaceExisting = true;

	TArray<UObject*> ImportedAssets = AssetTools.ImportAssetsAutomated(ImportData);

	if (ImportedAssets.Num() > 1)
	{
		UE_LOG(LogTemp, Warning, TEXT("Unexpectedly found multiple imported assets."));
	}

	TWeakObjectPtr<USkeletalMesh> SkeletalMesh = Cast<USkeletalMesh>(ImportedAssets.Num() >= 1 ? ImportedAssets[0] : nullptr);

	if (!SkeletalMesh.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("No skeletal mesh was successfully imported"));
		return;
	}

	TWeakObjectPtr<UAnimSequence> AnimSequence = Cast<UAnimSequence>(UEditorAssetLibrary::LoadAsset(RelativeTripletPath / SkeletalMesh->GetName() + TEXT("_Anim")));
	TWeakObjectPtr<USkeleton> Skeleton = Cast<USkeleton>(UEditorAssetLibrary::LoadAsset(RelativeTripletPath / SkeletalMesh->GetName() + TEXT("_Skeleton")));
	TWeakObjectPtr<UPhysicsAsset> PhysicsAsset = Cast<UPhysicsAsset>(UEditorAssetLibrary::LoadAsset(RelativeTripletPath / SkeletalMesh->GetName() + TEXT("_PhysicsAsset")));

	if (!AnimSequence.IsValid() || !Skeleton.IsValid() || !PhysicsAsset.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("One or more assets were unsuccessfully imported."));
		return;
	}

	FProceduralRetargetAssets ProceduralAssets;

	ProceduralAssets.AutoGenerateIKRigAsset(SkeletalMesh.Get(), ERetargetSourceOrTarget::Source);
	ProceduralAssets.AutoGenerateIKRigAsset(InSelectedMetahumanIdentityEntry->SkeletalMeshRef.LoadSynchronous(), ERetargetSourceOrTarget::Target);
	ProceduralAssets.AutoGenerateIKRetargetAsset();

	const TStrongObjectPtr<UIKRetargetBatchOperation> MetahumanBatchOperation(NewObject<UIKRetargetBatchOperation>());
	FIKRetargetBatchOperationContext BatchContext;

	BatchContext.AssetsToRetarget = { AnimSequence.Get() };
	BatchContext.NameRule.FolderPath = RelativeTripletPath;
	BatchContext.SourceMesh = SkeletalMesh.Get();
	BatchContext.TargetMesh = InSelectedMetahumanIdentityEntry->SkeletalMeshRef.LoadSynchronous();
	BatchContext.bOverwriteExistingFiles = true;
	BatchContext.IKRetargetAsset = ProceduralAssets.Retargeter;
	MetahumanBatchOperation->RunRetarget(BatchContext);

	// Delete pre-retargeting assets
	UEditorAssetLibrary::DeleteAsset(PhysicsAsset->GetPathName());
	UEditorAssetLibrary::DeleteAsset(AnimSequence->GetPathName());
	UEditorAssetLibrary::DeleteAsset(SkeletalMesh->GetPathName());
	UEditorAssetLibrary::DeleteAsset(Skeleton->GetPathName());
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

void UMocapImporterSubsystem::ImportMetahumanAiCalibration(const FString& InMetahumanAiCalibrationFilePath)
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	FString FileName = FPaths::GetCleanFilename(InMetahumanAiCalibrationFilePath);
	FString PackagePath = ConvertToProjectRelativePath(TripletImportPath / FileName);

	TArray<UObject*> Results = AssetTools.ImportAssets({ InMetahumanAiCalibrationFilePath }, PackagePath, nullptr, true);
}

void UMocapImporterSubsystem::ImportAudioMocap(const FString& InMetahumanVideoFilePath)
{
	if (!ensureAlways(ActiveMetahumanIdentity.IsValid()))
	{
		return;
	}

	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this, InMetahumanVideoFilePath]()
	{
		// Phase 0: Setup Variables
		static const FString ImportPrefix = TEXT("Dialogue_");
		IFileManager& FileManager = IFileManager::Get();
		FString WavFileName = ImportPrefix + FPaths::GetBaseFilename(InMetahumanVideoFilePath);
		FString DestinationAudioFilePath = FPaths::Combine(FPaths::GetPath(InMetahumanVideoFilePath), WavFileName + TEXT(".wav"));
		FString ImportWavPackage = ConvertToProjectRelativePath(TripletImportPath / WavFileName);
		FString ImportWavPath = TripletImportPath / WavFileName + TEXT(".wav");

		if (TObjectPtr<UVoiceConverterSubsystem> VoiceConverterSubsystem = GEditor->GetEditorSubsystem<UVoiceConverterSubsystem>(); ensure(VoiceConverterSubsystem))
		{
			// Phase 1: Convert the MOV to WAV
			VoiceConverterSubsystem->ConvertMovToWav(*InMetahumanVideoFilePath, *DestinationAudioFilePath);

			// Phase 2: Voice conversions (Omitted from public github)

			// Phase 3: Import Audio
			AsyncTask(ENamedThreads::GameThread, [this, ImportWavPackage, ImportWavPath, WavFileName]()
			{
				ImportSoundWave(ImportWavPackage, WavFileName, ImportWavPath);
			});
		}
	});
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

#pragma optimize("", on)
