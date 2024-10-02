#pragma once

#include "EditorSubsystem.h"

#include "MocapImporterSubsystem.generated.h"

class UActorIdentityEntryWidget;
class UBodyMocapEntryWidget;
class UFaceMocapEntryWidget;
class UMetahumanIdentityEntryWidget;
class USoundClass;

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

	void SetTripletImportPath(const FString& InTripletImportPath);
	FString GetTripletImportPath() const;
	void SetActiveMetahumanIdentity(TWeakObjectPtr<UMetahumanIdentityEntryWidget> InMetahumanIdentity);
	TWeakObjectPtr<UMetahumanIdentityEntryWidget> GetActiveMetahumanIdentity() const;
	void SetActiveActorIdentity(TWeakObjectPtr<UActorIdentityEntryWidget> InActorIdentity);
	TWeakObjectPtr<UActorIdentityEntryWidget> GetActiveActorIdentity() const;
	void SetDialogueSoundClass(TObjectPtr<USoundClass> InMetahumanIdentity);
	TObjectPtr<USoundClass> GetDialogueSoundClass() const;

	void ImportFaceAnimation(const FString& InFaceMocapExtractPath, TWeakObjectPtr<UFaceMocapEntryWidget> InSelectedFaceMocapEntry,
		TWeakObjectPtr<UActorIdentityEntryWidget> InActorIdentityEntryWidget);
	void ImportBodyAnimation(const FString& InBodyMocapExtractPath, const FString& InTripletImportPath,
		TWeakObjectPtr<UBodyMocapEntryWidget> InSelectedBodyMocapEntry, TWeakObjectPtr<UMetahumanIdentityEntryWidget> InSelectedMetahumanIdentityEntry);

	UFUNCTION()
	void ImportMetahumanAiCalibration(const FString& InMetahumanAiCalibrationFilePath);
	UFUNCTION()
	void ImportAudioMocap(const FString& InMetahumanVideoFilePath);

private:
	UFUNCTION()
	void OnProcessFaceAnimationComplete();
	UFUNCTION()
	USoundWave* ImportSoundWave(const FString& InSoundWavePackageName, const FString& InSoundWaveAssetName, const FString& InWavFilename) const;
	
	FString ConvertToProjectRelativePath(const FString& InAbsolutePath);

	bool bIsAwaitingCanProcess = false;
	bool bIsFaceAnimationProcessing = false;
	FString TripletImportPath;
	TWeakObjectPtr<UMetahumanIdentityEntryWidget> ActiveMetahumanIdentity;
	TWeakObjectPtr<UActorIdentityEntryWidget> ActiveActorIdentity;

	FString CachedDestinationAudioFile;
	FString CachedAudioFileNameNoExtension;

	UPROPERTY()
	TObjectPtr<USoundClass> DialogueSoundClass;
};
