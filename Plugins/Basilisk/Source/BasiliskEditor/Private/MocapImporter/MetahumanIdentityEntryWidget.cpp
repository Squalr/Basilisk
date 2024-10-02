#include "MocapImporter/MetahumanIdentityEntryWidget.h"

#include "EditorUtilityWidgetComponents.h"

void UMetahumanIdentityEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (SelectButton)
	{
		SelectButton->OnClicked.AddUniqueDynamic(this, &UMetahumanIdentityEntryWidget::OnMetahumanIdentityEntryClicked);
	}
}

void UMetahumanIdentityEntryWidget::SetIsSelected(bool bInIsSelected)
{
	bIsSelected = bInIsSelected;
}

void UMetahumanIdentityEntryWidget::OnMetahumanIdentityEntryClicked()
{
	OnMetahumanIdentityEntryClickedEvent.Broadcast(this);
}
