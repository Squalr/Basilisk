#include "CaptureOrchestrator.h"

#include "EditorUtilityWidgetComponents.h"
#include "VoiceConverter/VoiceConverterSubsystem.h"

UCaptureOrchestrator::UCaptureOrchestrator(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Initialize properties or perform other setup here.
}

void UCaptureOrchestrator::NativePreConstruct()
{
	Super::NativePreConstruct();
	
	if (ensure(StartWhisperStreamButton) && ensure(StopWhisperStreamButton))
	{
		StartWhisperStreamButton->OnClicked.AddUniqueDynamic(this, &UCaptureOrchestrator::StartListeningToDevice);
		StopWhisperStreamButton->OnClicked.AddUniqueDynamic(this, &UCaptureOrchestrator::StopListeningToDevice);
	}
}

void UCaptureOrchestrator::StartListeningToDevice()
{

}

void UCaptureOrchestrator::InterpretText(FString OutputString, float StartTimeSeconds, float EndTimeSeconds)
{
	UE_LOG(LogTemp, Warning, TEXT("%s, %0.3f, %0.3f"), *OutputString, StartTimeSeconds, EndTimeSeconds);
}

void UCaptureOrchestrator::StopListeningToDevice()
{

}