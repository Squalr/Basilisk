#include "CaptureOrchestrator/CaptureOrchestratorWidget.h"

#include "CaptureOrchestrator/CaptureOrchestratorSubsystem.h"
#include "EditorUtilityWidgetComponents.h"
#include "Modules/ModuleManager.h"

UCaptureOrchestratorWidget::UCaptureOrchestratorWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UCaptureOrchestratorWidget::NativePreConstruct()
{
    Super::NativePreConstruct();

    UCaptureOrchestratorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UCaptureOrchestratorSubsystem>();

    if (ensure(Subsystem))
    {
        // Initialize default ports
        DesktopOscPort = FString::FromInt(Subsystem->GetDesktopOscPort());
    }

    LoadConfig();
}

void UCaptureOrchestratorWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (RecordButton)
    {
        RecordButton->OnClicked.AddUniqueDynamic(this, &UCaptureOrchestratorWidget::OnRecordButtonClicked);
    }

    if (TObjectPtr<UCaptureOrchestratorSubsystem> Subsystem = GEditor->GetEditorSubsystem<UCaptureOrchestratorSubsystem>(); ensure(Subsystem))
    {
        DesktopOscPort = FString::FromInt(Subsystem->GetDesktopOscPort());
    }
}

FText UCaptureOrchestratorWidget::GetSceneName() const
{
    if (TObjectPtr<UCaptureOrchestratorSubsystem> Subsystem = GEditor->GetEditorSubsystem<UCaptureOrchestratorSubsystem>(); ensure(Subsystem))
    {
        return Subsystem->GetSceneName();
    }

    return {};
}

FText UCaptureOrchestratorWidget::GetTakeNumber() const
{
    if (TObjectPtr<UCaptureOrchestratorSubsystem> Subsystem = GEditor->GetEditorSubsystem<UCaptureOrchestratorSubsystem>(); ensure(Subsystem))
    {
        return FText::FromString(FString::FromInt(Subsystem->GetTakeNumber()));
    }

    return {};
}

FText UCaptureOrchestratorWidget::GetIphoneIpAddress() const
{
    if (TObjectPtr<UCaptureOrchestratorSubsystem> Subsystem = GEditor->GetEditorSubsystem<UCaptureOrchestratorSubsystem>(); ensure(Subsystem))
    {
        return Subsystem->GetIphoneIpAddress();
    }

    return {};
}

FText UCaptureOrchestratorWidget::GetIphoneStatus() const
{
    if (TObjectPtr<UCaptureOrchestratorSubsystem> Subsystem = GEditor->GetEditorSubsystem<UCaptureOrchestratorSubsystem>(); ensure(Subsystem))
    {
        return Subsystem->GetIphoneStatus();
    }

    return {};
}

FText UCaptureOrchestratorWidget::GetRokokoStatus() const
{
    if (TObjectPtr<UCaptureOrchestratorSubsystem> Subsystem = GEditor->GetEditorSubsystem<UCaptureOrchestratorSubsystem>(); ensure(Subsystem))
    {
        return Subsystem->GetRokokoStatus();
    }

    return {};
}

FText UCaptureOrchestratorWidget::GetDesktopStatus() const
{
    if (TObjectPtr<UCaptureOrchestratorSubsystem> Subsystem = GEditor->GetEditorSubsystem<UCaptureOrchestratorSubsystem>(); ensure(Subsystem))
    {
        return Subsystem->GetDesktopStatus();
    }

    return {};
}

void UCaptureOrchestratorWidget::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    if (TObjectPtr<UCaptureOrchestratorSubsystem> Subsystem = GEditor->GetEditorSubsystem<UCaptureOrchestratorSubsystem>(); ensure(Subsystem))
    {
        if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UCaptureOrchestratorWidget, DesktopOscPort))
        {
            if (DesktopOscPort.IsNumeric())
            {
                Subsystem->SetDesktopOscPort(FCString::Atoi(*DesktopOscPort));
            }
        }
    }
}

void UCaptureOrchestratorWidget::OnRecordButtonClicked()
{
    if (TObjectPtr<UCaptureOrchestratorSubsystem> Subsystem = GEditor->GetEditorSubsystem<UCaptureOrchestratorSubsystem>(); ensure(Subsystem))
    {
        // Subsystem->RemoteSetSceneName(TEXT("Test Record"));
    }
}
