#pragma once

#include "Blueprint/UserWidget.h"

#include "CoreMinimal.h"

#include "FaceMocapEntryWidget.generated.h"

class UEditorUtilityButton;
class UTextBlock;

UCLASS(BlueprintType, Blueprintable)
class UFaceMocapEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(const FString& InMocapName);
	void SetIsSelected(bool bInIsSelected);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMocapEntryClicked, UFaceMocapEntryWidget*, MocapEntry);
	UPROPERTY()
	FOnMocapEntryClicked OnMocapEntryClickedEvent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString MocapName;

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> NameTextBlock;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UEditorUtilityButton> SelectButton;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bIsSelected;

private:
	UFUNCTION()
	void OnMocapEntryClicked();
};
