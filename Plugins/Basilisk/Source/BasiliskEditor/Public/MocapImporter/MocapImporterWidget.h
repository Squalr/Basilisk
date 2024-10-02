#pragma once

#include "Blueprint/UserWidget.h"

#include "Blueprint/UserWidgetPool.h"
#include "CoreMinimal.h"
#include "IDirectoryWatcher.h"

#include "MocapImporterWidget.generated.h"

class UBodyMocapEntryWidget;
class UEditorUtilityButton;
class UFaceMocapEntryWidget;
class UIKRetargeter;
class USkeleton;
class UTextBlock;
class UVerticalBox;

UCLASS(BlueprintType, Blueprintable, Config = "EditorSettings")
class UMocapImporterWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UMocapImporterWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativePreConstruct() override;

protected:
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UFaceMocapEntryWidget> FaceMocapEntryWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UBodyMocapEntryWidget> BodyMocapEntryWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<USoundClass> DialogueSoundClass;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UEditorUtilityButton> SelectFaceMocapPathButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> FaceMocapPathTextBlock;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UVerticalBox> FaceMocapVerticalBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UEditorUtilityButton> SelectBodyMocapPathButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> BodyMocapPathTextBlock;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UVerticalBox> BodyMocapVerticalBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UVerticalBox> ActorIdentitiesVerticalBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UVerticalBox> MocapIdentitiesVerticalBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UEditorUtilityButton> SelectImportPathButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UEditorUtilityButton> AutoPathButton;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UEditorUtilityButton> ImportBodyMocapButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UEditorUtilityButton> ImportFaceMocapButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UEditorUtilityButton> ImportVoiceButton;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Config, Category = "Mocap")
	FString FaceMocapExtractPath;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Config, Category = "Mocap")
	FString BodyMocapExtractPath;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Config, Category = "Mocap")
	FString TripletImportPath;

private:
	void SetFaceMocapDirectory(const FString& InFaceMocapDirectory);
	void SetBodyMocapDirectory(const FString& InBodyMocapDirectory);
	void HandleFaceMocapDirectoryChanged(const TArray<FFileChangeData>& InFileChanges);
	void HandleBodyMocapDirectoryChanged(const TArray<FFileChangeData>& InFileChanges);
	void RebuildFaceMocapList();
	void RebuildBodyMocapList();
	
	UFUNCTION()
	void OnFaceMocapEntrySelected(UFaceMocapEntryWidget* InMocapEntry);
	UFUNCTION()
	void OnBodyMocapEntrySelected(UBodyMocapEntryWidget* InMocapEntry);
	UFUNCTION()
	void OnActorIdentityEntrySelected(UActorIdentityEntryWidget* InActorIdentityEntry);
	UFUNCTION()
	void OnMetahumanIdentityEntrySelected(UMetahumanIdentityEntryWidget* InMetahumanIdentityEntry);

	UFUNCTION()
	void OnSelectFaceMocapPathClicked();
	UFUNCTION()
	void OnSelectBodyMocapPathButtonClicked();
	UFUNCTION()
	void OnSelectImportPathButtonClicked();
	UFUNCTION()
	void OnAutoPathButtonButtonClicked();
	UFUNCTION()
	void OnImportFaceMocapButtonClicked();
	UFUNCTION()
	void OnImportBodyMocapButtonClicked();
	UFUNCTION()
	void OnImportVoiceButtonClicked();

	UPROPERTY(Transient)
	FUserWidgetPool FaceMocapWidgetsPool;

	UPROPERTY(Transient)
	FUserWidgetPool BodyMocapWidgetsPool;

	TArray<TObjectPtr<UFaceMocapEntryWidget>> FaceMocapEntries;
	TArray<TObjectPtr<UBodyMocapEntryWidget>> BodyMocapEntries;
	TArray<TWeakObjectPtr<UActorIdentityEntryWidget>> ActorIdentityEntries;
	TArray<TWeakObjectPtr<UMetahumanIdentityEntryWidget>> MetahumanIdentityEntries;
	IDirectoryWatcher::FDirectoryChanged FaceMocapDirectoryChangedDelegate;
	IDirectoryWatcher::FDirectoryChanged BodyMocapDirectoryChangedDelegate;
	FDelegateHandle FaceMocapDirectoryChangedDelegateHandle;
	FDelegateHandle BodyMocapDirectoryChangedDelegateHandle;

	TWeakObjectPtr<UFaceMocapEntryWidget> SelectedFaceMocapEntry;
	TWeakObjectPtr<UBodyMocapEntryWidget> SelectedBodyMocapEntry;
	TWeakObjectPtr<UActorIdentityEntryWidget> SelectedActorIdentityEntry;
	TWeakObjectPtr<UMetahumanIdentityEntryWidget> SelectedMetahumanIdentityEntry;
};
