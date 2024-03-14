#include "CaptureOrchestrator.h"

#include "EditorUtilityWidgetComponents.h"

UCaptureOrchestrator::UCaptureOrchestrator(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Initialize properties or perform other setup here.
}

void UCaptureOrchestrator::NativePreConstruct()
{
	Super::NativePreConstruct();
}

void UCaptureOrchestrator::InterpretText(FString OutputString, float StartTimeSeconds, float EndTimeSeconds)
{
	UE_LOG(LogTemp, Warning, TEXT("%s, %0.3f, %0.3f"), *OutputString, StartTimeSeconds, EndTimeSeconds);
}
