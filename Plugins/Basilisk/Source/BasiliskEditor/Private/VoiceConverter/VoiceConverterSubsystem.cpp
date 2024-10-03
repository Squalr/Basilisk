#include "VoiceConverter/VoiceConverterSubsystem.h"

#include "IPythonScriptPlugin.h"
#include "RuntimeAudioExporter.h"
#include "RuntimeAudioImporterLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/Paths.h"
#include "HAL/PlatformProcess.h"

bool UVoiceConverterSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	// Only create this subsystem for the editor
	return GIsEditor;
}

void UVoiceConverterSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	// Get the path to the project directory
	FString ProjectDir = FPaths::ProjectDir();
	// Get path to plugin source thirdparty directory
	FString SourceThirdPartyDir = FPaths::Combine(FPaths::ProjectPluginsDir(), TEXT("Basilisk/Source/ThirdParty"));

	// Append the relative path from the project directory to the FFmpeg executable
#if defined(_WIN32) || defined(_WIN64)
	FFmpegPath = FPaths::Combine(SourceThirdPartyDir, TEXT("ffmpeg_windows.exe"));
#else
	throw std::logic_error("This code is not implemented for this OS. Compile or Download FFMPEG for your OS and add to the project's ThirdParty folder.");
#endif
}

void UVoiceConverterSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

FString UVoiceConverterSubsystem::GetFFmpegPath()
{
	// Get the path to the project directory
	FString ProjectDir = FPaths::ProjectDir();

	// Append the relative path from the project directory to the FFmpeg executable
#if defined(_WIN32) || defined(_WIN64)
	FFmpegPath = FPaths::Combine(ProjectDir, TEXT("ThirdParty"), TEXT("ffmpeg_windows_64.exe"));
#else
	throw std::logic_error("This code is not implemented for this OS. Compile FFMPEG for your OS and add to the project's ThirdParty folder.");
#endif

	return FFmpegPath;
}

void UVoiceConverterSubsystem::RunFFmpegCommand(const FString& Command)
{
	// Check if the FFmpeg executable exists
	if (!FPaths::FileExists(FFmpegPath))
	{
		UE_LOG(LogTemp, Error, TEXT("FFmpeg executable not found at path: %s"), *FFmpegPath);
		return;
	}

	// Run the FFmpeg command
	FProcHandle ProcHandle = FPlatformProcess::CreateProc(*FFmpegPath, *Command, true, false, false, nullptr, 0, nullptr, nullptr);

	if (!ProcHandle.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to start FFmpeg process"));
		return;
	}

	// Optionally, wait for the FFmpeg process to finish
	while (FPlatformProcess::IsProcRunning(ProcHandle))
	{
		FPlatformProcess::Sleep(0.01); // Sleep for a bit to reduce CPU usage
	}

	// Close the process handle
	FPlatformProcess::CloseProc(ProcHandle);
}

void UVoiceConverterSubsystem::ConvertMovToWav(const FString& InputMovFilePath, const FString& OutputWavFilePath)
{
	// Construct the FFmpeg command to convert .mov to .wav
	FString Command = FString::Printf(TEXT("-i %s -vn -acodec pcm_s16le -ar 44100 -ac 2 %s"), *InputMovFilePath, *OutputWavFilePath);

	// Run the FFmpeg command
	RunFFmpegCommand(Command);
}

void UVoiceConverterSubsystem::HandleImportedSoundWave(bool bSucceeded, UImportedSoundWave* ImportedSoundWave)
{
	if (bSucceeded && ImportedSoundWave)
	{
		const FString SoundWaveName = ImportedSoundWave->GetName();
		const FString& SavePath = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Temp"), SoundWaveName + TEXT(".wav"));
		ERuntimeAudioFormat AudioFormat = ERuntimeAudioFormat::Wav;
		constexpr uint8 Quality = 100; // Set the quality
		FRuntimeAudioExportOverrideOptions OverrideOptions; // Set the override options
		FOnAudioExportToFileResultNative Result; // Set the result delegate

		Result.BindLambda([this, SavePath](bool bSucceeded)
		{
			if (bSucceeded)
			{
				UE_LOG(LogTemp, Log, TEXT("Audio export succeeded."));
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Audio export failed."));
			}
		});

		// Convert SoundWavePath to wav file using RuntimeAudioImporter library
		URuntimeAudioExporter::ExportSoundWaveToFile(ImportedSoundWave, SavePath, AudioFormat, Quality, OverrideOptions, Result);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to convert regular SoundWave to ImportedSoundWave"));
	}
}

