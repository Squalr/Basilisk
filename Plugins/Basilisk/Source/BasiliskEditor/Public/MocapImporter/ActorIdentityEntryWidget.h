#pragma once

#include "Blueprint/UserWidget.h"

#include "CoreMinimal.h"

#include "ActorIdentityEntryWidget.generated.h"

class UEditorUtilityButton;
class USkeletalMesh;
class UTextBlock;

UCLASS(BlueprintType, Blueprintable)
class UActorIdentityEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetIsSelected(bool bInIsSelected);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActorIdentityEntryClicked, UActorIdentityEntryWidget*, MocapEntry);
	UPROPERTY()
	FOnActorIdentityEntryClicked OnActorIdentityEntryClickedEvent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Retargeter)
	TSoftObjectPtr<UObject> MetahumanPerformanceImporter;

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ActorIdentity;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> IdentityNameTextBlock;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UEditorUtilityButton> SelectButton;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bIsSelected;

private:
	UFUNCTION()
	void OnActorIdentityEntryClicked();
};
