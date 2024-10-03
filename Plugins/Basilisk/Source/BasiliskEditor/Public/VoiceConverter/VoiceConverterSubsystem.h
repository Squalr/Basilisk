#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "VoiceConverterSubsystem.generated.h"

class UImportedSoundWave;

UCLASS()
class UVoiceConverterSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	//~UEditorSubsystem interface
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~End of UEditorSubsystem interface

	FString GetFFmpegPath();
	void ConvertMovToWav(const FString& InputMovFilePath, const FString& OutputWavFilePath);

private:
	FString FFmpegPath;
	FString CachedDestinationAudioFile;
	FString CachedAudioFileNameNoExtension;

	UPROPERTY()
	TObjectPtr<USoundWave> CachedSoundWave;
	UFUNCTION() // Needs to be declared as a UFUNCTION to be used as a delegate
	void HandleImportedSoundWave(bool bSucceeded, UImportedSoundWave* ImportedSoundWave);
	void RunFFmpegCommand(const FString& Command);
};

struct FWordData
{
	double Start;
	FString Text;

	bool InitializeFromJson(TSharedPtr<FJsonObject> JsonObject)
	{
		return JsonObject->TryGetNumberField(TEXT("start"), Start) &&
			   JsonObject->TryGetStringField(TEXT("word"), Text);
	}
};

struct FSegmentData
{
	TArray<FWordData> Words;

	bool InitializeFromJson(TSharedPtr<FJsonObject> JsonObject)
	{
		const TArray<TSharedPtr<FJsonValue>>* WordsArray;
		if (!JsonObject->TryGetArrayField(TEXT("words"), WordsArray))
		{
			return false;
		}

		for (const auto& Value : *WordsArray)
		{
			FWordData WordData;
			if (WordData.InitializeFromJson(Value->AsObject()))
			{
				Words.Add(MoveTemp(WordData));
			}
			else
			{
				return false;
			}
		}

		return true;
	}
};