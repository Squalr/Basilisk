#pragma once

#include "Blueprint/UserWidget.h"

#include "CoreMinimal.h"

#include "CaptureOrchestratorWidget.generated.h"

class UEditorUtilityButton;
class UTextBlock;

UCLASS(BlueprintType, Blueprintable, Config = "EditorSettings")
class UCaptureOrchestratorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UCaptureOrchestratorWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintPure)
	FText GetSceneName() const;

	UFUNCTION(BlueprintPure)
	FText GetTakeNumber() const;

	UFUNCTION(BlueprintPure)
	FText GetIphoneIpAddress() const;

	UFUNCTION(BlueprintPure)
	FText GetIphoneStatus() const;

	UFUNCTION(BlueprintPure)
	FText GetRokokoStatus() const;

	UFUNCTION(BlueprintPure)
	FText GetDesktopStatus() const;

protected:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UFUNCTION()
	void OnStartRecordingButtonClicked();
	UFUNCTION()
	void OnStopRecordingButtonClicked();

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> SceneNameTextBlock;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> IphoneIpAddressTextBlock;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> IphoneStatusTextBlock;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UEditorUtilityButton> StartRecordingButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UEditorUtilityButton> StopRecordingButton;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Config, Category = "Mocap")
	FString DesktopOscPort;
};
