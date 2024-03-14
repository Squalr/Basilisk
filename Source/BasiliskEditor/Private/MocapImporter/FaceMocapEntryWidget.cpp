#include "MocapImporter/FaceMocapEntryWidget.h"

#include "EditorUtilityWidgetComponents.h"

void UFaceMocapEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (SelectButton)
	{
		SelectButton->OnClicked.AddUniqueDynamic(this, &UFaceMocapEntryWidget::OnMocapEntryClicked);
	}
}

void UFaceMocapEntryWidget::Setup(const FString& InMocapName)
{
	MocapName = InMocapName;
}

void UFaceMocapEntryWidget::SetIsSelected(bool bInIsSelected)
{
	bIsSelected = bInIsSelected;
}

void UFaceMocapEntryWidget::OnMocapEntryClicked()
{
	OnMocapEntryClickedEvent.Broadcast(this);
}
