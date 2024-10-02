#pragma once

#include "Blueprint/UserWidget.h"

#include "CoreMinimal.h"

#include "MetahumanIdentityEntryWidget.generated.h"

class UEditorUtilityButton;
class USkeletalMesh;
class UTextBlock;

UENUM(BlueprintType, Blueprintable)
enum class EVoiceModelType : uint8
{
	ElevenLabs,
	Respeecher,
	LocalModel,
};

UCLASS(BlueprintType, Blueprintable)
class UMetahumanIdentityEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetIsSelected(bool bInIsSelected);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMetahumanIdentityEntryClicked, UMetahumanIdentityEntryWidget*, MocapEntry);
	UPROPERTY()
	FOnMetahumanIdentityEntryClicked OnMetahumanIdentityEntryClickedEvent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Retargeter)
	TSoftObjectPtr<USkeletalMesh> SkeletalMeshRef;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VoiceTransformer)
	FString MetahumanIdentityName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VoiceTransformer)
	EVoiceModelType VoiceModelType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VoiceTransformer)
	FString ModelName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VoiceTransformer)
	FString Accent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VoiceTransformer)
	float PitchCorrection;

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> IdentityNameTextBlock;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UEditorUtilityButton> SelectButton;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bIsSelected;

private:
	UFUNCTION()
	void OnMetahumanIdentityEntryClicked();
};
