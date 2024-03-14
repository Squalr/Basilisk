#include "MocapImporter/BodyMocapEntryWidget.h"

#include "EditorUtilityWidgetComponents.h"

void UBodyMocapEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (SelectButton)
	{
		SelectButton->OnClicked.AddUniqueDynamic(this, &UBodyMocapEntryWidget::OnMocapEntryClicked);
	}
}

void UBodyMocapEntryWidget::Setup(const FString& InMocapName)
{
	MocapName = InMocapName;
}

void UBodyMocapEntryWidget::SetIsSelected(bool bInIsSelected)
{
	bIsSelected = bInIsSelected;
}

void UBodyMocapEntryWidget::OnMocapEntryClicked()
{
	OnMocapEntryClickedEvent.Broadcast(this);
}
