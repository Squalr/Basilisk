#pragma once

#include "EditorSubsystem.h"

#include "MocapImporterSubsystem.generated.h"

class UBodyMocapEntryWidget;
class UFaceMocapEntryWidget;
class UIKRetargeter;
class USkeleton;

UCLASS()
class UMocapImporterSubsystem : public UEditorSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	//~UEditorSubsystem interface
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~End of UEditorSubsystem interface

	//~FTickableGameObject interface
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual bool IsTickableWhenPaused() const override;
	virtual bool IsTickableInEditor() const override;
	virtual TStatId GetStatId() const override;
	//~End of FTickableGameObject interface

	void ImportFaceAnimation(const FString& InFaceMocapExtractPath, TWeakObjectPtr<UFaceMocapEntryWidget> InSelectedFaceMocapEntry,
		TSoftObjectPtr<UObject> InMetahumanPerformanceImporter);
	void ImportBodyAnimation(const FString& InBodyMocapExtractPath, const FString& InTripletImportPath,
		TWeakObjectPtr<UBodyMocapEntryWidget> InSelectedBodyMocapEntry, TWeakObjectPtr<UMocapIdentityEntryWidget> InSelectedMocapIdentityEntry,
		TSoftObjectPtr<USkeleton> InUE4SkeletonRef, TSoftObjectPtr<USkeletalMesh> InUE4SkeletalMeshRef,
		TSoftObjectPtr<USkeletalMesh> InUE5SkeletalMeshRef, TSoftObjectPtr<UIKRetargeter> InUE4ToUE5Retargeter,
		TSoftObjectPtr<UIKRetargeter> InUE5ToMetahumanRetargeter);
	void ImportAudio(const FString& InFaceMocapExtractPath, const FString& InTripletImportPath, const FString& InCondaEnvPath,
		TObjectPtr<USoundClass> InDialogueSoundClass, TWeakObjectPtr<UFaceMocapEntryWidget> InSelectedFaceMocapEntry,
		TWeakObjectPtr<UMocapIdentityEntryWidget> InSelectedMocapIdentityEntry);

private:
	UFUNCTION()
	void OnProcessFaceAnimationComplete();
	USoundWave* ImportSoundWave(const FString& InSoundWavePackageName, const FString& InSoundWaveAssetName, const FString& InWavFilename) const;
	bool ExecutePython(const FString& InScriptPath, const FString& InCondaEnv, const FString& InArgs);
	bool ExecutePythonWsl(const FString& InScriptPath, const FString& InArgs);
	FString ConvertToWSLPath(const FString& InWindowsPath);
	FString ConvertToProjectRelativePath(const FString& InAbsolutePath);

	bool bIsAwaitingCanProcess = false;
	bool bIsFaceAnimationProcessing = false;

	FString CachedDestinationAudioFile;
	FString CachedAudioFileNameNoExtension;
	FString CachedTripletImportPath;
	FString CachedCondaEnvPath;

	UPROPERTY()
	TObjectPtr<USoundWave> CachedSoundWave;

	UPROPERTY()
	TObjectPtr<USoundClass> DialogueSoundClass;
};
