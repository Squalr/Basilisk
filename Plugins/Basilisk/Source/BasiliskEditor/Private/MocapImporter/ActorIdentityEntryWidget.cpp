#include "MocapImporter/ActorIdentityEntryWidget.h"

#include "EditorUtilityWidgetComponents.h"

void UActorIdentityEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (SelectButton)
	{
		SelectButton->OnClicked.AddUniqueDynamic(this, &UActorIdentityEntryWidget::OnActorIdentityEntryClicked);
	}
}

void UActorIdentityEntryWidget::SetIsSelected(bool bInIsSelected)
{
	bIsSelected = bInIsSelected;
}

void UActorIdentityEntryWidget::OnActorIdentityEntryClicked()
{
	OnActorIdentityEntryClickedEvent.Broadcast(this);
}
