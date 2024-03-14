#include "MocapImporter/MocapIdentityEntryWidget.h"

#include "EditorUtilityWidgetComponents.h"

void UMocapIdentityEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (SelectButton)
	{
		SelectButton->OnClicked.AddUniqueDynamic(this, &UMocapIdentityEntryWidget::OnMocapIdentityEntryClicked);
	}
}

void UMocapIdentityEntryWidget::SetIsSelected(bool bInIsSelected)
{
	bIsSelected = bInIsSelected;
}

void UMocapIdentityEntryWidget::OnMocapIdentityEntryClicked()
{
	OnMocapIdentityEntryClickedEvent.Broadcast(this);
}
