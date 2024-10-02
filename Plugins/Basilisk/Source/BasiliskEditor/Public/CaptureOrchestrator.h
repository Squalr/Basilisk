#pragma once

#include "Blueprint/UserWidget.h"

#include "CoreMinimal.h"

#include "CaptureOrchestrator.generated.h"

class UEditorUtilityButton;
class UTextBlock;

UCLASS(BlueprintType, Blueprintable)
class UCaptureOrchestrator : public UUserWidget
{
	GENERATED_BODY()

public:
	UCaptureOrchestrator(const FObjectInitializer& ObjectInitializer);

	virtual void NativePreConstruct() override;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnOrchestratorStart);
	UPROPERTY(BlueprintAssignable, Category = "Basilisk|Orchestrator|Delegates")
	FOnOrchestratorStart OnOrchestratorStart;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnOrchestratorStop);
	UPROPERTY(BlueprintAssignable, Category = "Basilisk|Orchestrator|Delegates")
	FOnOrchestratorStop OnOrchestratorStop;
	
	UFUNCTION()
	void StartListeningToDevice();
	
	UFUNCTION()
	void StopListeningToDevice();

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UEditorUtilityButton> StartWhisperStreamButton;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UEditorUtilityButton> StopWhisperStreamButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> StartTextBlock;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> StopTextBlock;

private:
	UFUNCTION()
	void InterpretText(FString OutputString, float StartTimeSeconds, float EndTimeSeconds);
};
