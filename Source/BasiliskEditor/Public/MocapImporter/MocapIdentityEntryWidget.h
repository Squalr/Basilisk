#pragma once

#include "Blueprint/UserWidget.h"

#include "CoreMinimal.h"
#include "MocapImporter/MocapIdentity.h"

#include "MocapIdentityEntryWidget.generated.h"

class UEditorUtilityButton;
class USkeletalMesh;
class UTextBlock;

UCLASS(BlueprintType, Blueprintable)
class UMocapIdentityEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetIsSelected(bool bInIsSelected);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMocapIdentityEntryClicked, UMocapIdentityEntryWidget*, MocapEntry);
	UPROPERTY()
	FOnMocapIdentityEntryClicked OnMocapIdentityEntryClickedEvent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Retargeter)
	TSoftObjectPtr<USkeletalMesh> SkeletalMeshRef;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VoiceTransformer)
	FString ElevenLabsName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VoiceTransformer)
	FString RespeecherName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VoiceTransformer)
	float PitchCorrection;

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> IdentityNameTextBlock;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UEditorUtilityButton> SelectButton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FMocapIdentity MocapIdentity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bIsSelected;

private:
	UFUNCTION()
	void OnMocapIdentityEntryClicked();
};
