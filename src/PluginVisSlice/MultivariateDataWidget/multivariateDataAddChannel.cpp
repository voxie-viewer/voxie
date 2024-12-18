
#include "multivariateDataAddChannel.hpp"
#include "ui_multivariateDataAddChannel.h"

MultivariateDataAddChannel::MultivariateDataAddChannel(
    QWidget* parent,
    QHash<MultivariateDataListItemWidget*, channelMetaData>* dataChannels)
    : QDialog(parent), ui(new Ui::AddChannel_Dialog) {
  ui->setupUi(this);

  this->dataChannels = dataChannels;

  if (dataChannels && dataChannels->size() >= 1) {
    for (auto channel = dataChannels->cbegin(), end = dataChannels->cend();
         channel != end; ++channel) {
      if (channel.value().active == false) {
        // process all widgets that are not active
        MultivariateDataListItemWidget* widgetItem = channel.value().widItem;
        if (widgetItem) {
          QListWidget* list = ui->AddChannel_ListWidget;
          QListWidgetItem* row = new QListWidgetItem(list, 1042);
          list->addItem(row);
          row->setSizeHint(widgetItem->minimumSizeHint());
          list->setItemWidget(row, widgetItem);
          widgetItem->setDescription_LineEdit_readOnly(true);

          connect(widgetItem, &MultivariateDataListItemWidget::colorChanged,
                  this, &MultivariateDataAddChannel::listItem_colorChanged);

          // to keep MultivariateDataListItemWidgets selection checkbox and
          // QListWidgetItem selection synchronized
          widgetItem->setSelectionCheckBoxState(row->isSelected());
          connect(
              widgetItem,
              &MultivariateDataListItemWidget::selectionChangedByCheckBox, this,
              &MultivariateDataAddChannel::listWidgetCheckBoxSelectionChanged);

          connect(ui->AddChannel_ListWidget, &QListWidget::itemSelectionChanged,
                  this,
                  &MultivariateDataAddChannel::listWidgetSelcetionsChanged);

        } else {
          qWarning() << "List item == nullptr";
        }
      }
    }
  }
}

MultivariateDataAddChannel::~MultivariateDataAddChannel() { delete ui; }

QList<QListWidgetItem*> MultivariateDataAddChannel::getSelectedItems() {
  return this->ui->AddChannel_ListWidget->selectedItems();
}

MultivariateDataListItemWidget*
MultivariateDataAddChannel::getCustomWidgetFromListItem(QListWidgetItem* row) {
  return (MultivariateDataListItemWidget*)this->ui->AddChannel_ListWidget
      ->itemWidget(row);
};

void MultivariateDataAddChannel::clearSelection() {
  ui->AddChannel_ListWidget->clearSelection();
  ui->AddChannel_ListWidget->update();
};

void MultivariateDataAddChannel::listWidgetSelcetionsChanged() {
  qDebug() << "itemSelectionChanged add channel";
  int count = ui->AddChannel_ListWidget->count();
  for (int i = 0; i < count; i++) {
    QListWidgetItem* row = ui->AddChannel_ListWidget->item(i);
    MultivariateDataListItemWidget* customItem =
        (MultivariateDataListItemWidget*)ui->AddChannel_ListWidget->itemWidget(
            row);
    customItem->setSelectionCheckBoxState(row->isSelected());
  }
}

void MultivariateDataAddChannel::listWidgetCheckBoxSelectionChanged() {
  int count = ui->AddChannel_ListWidget->count();
  ui->AddChannel_ListWidget->blockSignals(true);
  for (int i = 0; i < count; i++) {
    QListWidgetItem* row = ui->AddChannel_ListWidget->item(i);
    MultivariateDataListItemWidget* customItem =
        (MultivariateDataListItemWidget*)ui->AddChannel_ListWidget->itemWidget(
            row);

    row->setSelected(customItem->getSelectionCheckboxState());
  }
  ui->AddChannel_ListWidget->blockSignals(false);
};

void MultivariateDataAddChannel::listItem_colorChanged(
    MultivariateDataListItemWidget* item, QColor color) {
  // Pointer to list item used as key
  if (item && this->dataChannels->contains(item)) {
    (*this->dataChannels)[item].color = color;
  }
}
