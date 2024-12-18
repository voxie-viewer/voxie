#include "multivariateDataWidget.hpp"
#include "ui_multivariateDataWidget.h"

MultivariateDataWidget::MultivariateDataWidget(QWidget* parent,
                                               SliceVisualizer* sv)
    : QWidget(parent), ui(new Ui::MultivariateData_Widget) {
  ui->setupUi(this);

  this->sv = sv;

  // Grey-out not activated Widget Functions
  this->disableOverviewUI();
  this->disableDetailsUI();
  this->disableEffZUI();

  this->activeMode = ActiveMode::none;
  this->setOverviewOptions();
  this->setEffZOptions();
  this->setInitUIValues();

  // connections from own widget elements to slots

  // ## Overview mode

  connect(ui->Overview_CheckBox, &QCheckBox::stateChanged, this,
          &MultivariateDataWidget::overview_Checkbox_clicked);

  connect(
      ui->Overview_MappingStrategy_DropDownList,
      static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
      this,
      &MultivariateDataWidget::overview_MappingStrategyDropdown_OptionChanged);

  connect(ui->Overview_AvgMin_LineEdit, &QLineEdit::editingFinished, this,
          &MultivariateDataWidget::overview_AvgMin_TextChanged);

  connect(ui->Overview_AvgMax_LineEdit, &QLineEdit::editingFinished, this,
          &MultivariateDataWidget::overview_AvgMax_TextChanged);

  connect(ui->Overview_StdDevMin_LineEdit, &QLineEdit::editingFinished, this,
          &MultivariateDataWidget::overview_StdDevMin_TextChanged);

  connect(ui->Overview_StdDevMax_LineEdit, &QLineEdit::editingFinished, this,
          &MultivariateDataWidget::overview_StdDevMax_TextChanged);

  // ## Overview mode
  // ## Details mode

  connect(ui->Details_Activation_CheckBox, &QCheckBox::stateChanged, this,
          &MultivariateDataWidget::detailsCheckBox_clicked);
  connect(ui->Details_Add_AddChannel_PushButton, &QPushButton::clicked, this,
          &MultivariateDataWidget::addChannelPushButton_clicked);

  connect(ui->Details_Add_AddRage_PushButton, &QPushButton::clicked, this,
          &MultivariateDataWidget::addRangePushButton_clicked);

  connect(ui->Details_RemoveSelected_PushButton, &QPushButton::clicked, this,
          &MultivariateDataWidget::removeSelectedPushButton_clicked);

  connect(ui->Details_ListWidget, &QListWidget::itemSelectionChanged, this,
          &MultivariateDataWidget::listWidgetSelcetionsChanged);

  // ## Details mode
  // ## Eff. Z & density mode

  connect(ui->EffZ_CheckBox, &QCheckBox::stateChanged, this,
          &MultivariateDataWidget::effZ_Checkbox_clicked);

  connect(
      ui->EffZ_MappingStrategy_DropDownList,
      static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
      this,
      &MultivariateDataWidget::effZ_MappingStrategyDropdown_OptionChanged);

  connect(ui->EffZ_EffZMin_LineEdit, &QLineEdit::editingFinished, this,
          &MultivariateDataWidget::effZ_EffZMin_TextChanged);

  connect(ui->EffZ_EffZMax_LineEdit, &QLineEdit::editingFinished, this,
          &MultivariateDataWidget::effZ_EffZMax_TextChanged);

  connect(ui->EffZ_DensityMin_LineEdit, &QLineEdit::editingFinished, this,
          &MultivariateDataWidget::effZ_DensityMin_TextChanged);

  connect(ui->EffZ_DensityMax_LineEdit, &QLineEdit::editingFinished, this,
          &MultivariateDataWidget::effZ_DensityMax_TextChanged);

  // ## Eff. Z & density mode
}

MultivariateDataWidget::~MultivariateDataWidget() { delete ui; }

// ######## Request methods ########

ActiveMode MultivariateDataWidget::getActiveMode() { return this->activeMode; }

OverviewStrategyOption MultivariateDataWidget::getOverviewStrategy() {
  return this->overviewStrategy;
}

float MultivariateDataWidget::getOverviewAvgMin() {
  return this->overviewAvgMin;
}

float MultivariateDataWidget::getOverviewAvgMax() {
  return this->overviewAvgMax;
}

float MultivariateDataWidget::getOverviewStdDevMin() {
  return this->overviewStdDevMin;
}

float MultivariateDataWidget::getOverviewStdDevMax() {
  return this->overviewStdDevMax;
}
EffZStrategyOption MultivariateDataWidget::getEffZStrategy() {
  return this->effZStrategy;
};
float MultivariateDataWidget::getEffZ_EffZMin() { return this->effZ_EffZMin; };
float MultivariateDataWidget::getEffZ_EffZMax() { return this->effZ_EffZMax; };
float MultivariateDataWidget::getEffZ_DensityMin() {
  return this->effZ_DensityMin;
};
float MultivariateDataWidget::getEffZ_DensityMax() {
  return this->effZ_DensityMax;
};

QList<channelMetaData> MultivariateDataWidget::getActiveChannels() {
  QList<channelMetaData> activeList;

  for (auto i = this->dataChannels.cbegin(), end = this->dataChannels.cend();
       i != end; ++i) {
    if (i.value().active == true) {
      activeList.append(i.value());
    }
  }
  for (auto i = this->customChannels.cbegin(),
            end = this->customChannels.cend();
       i != end; ++i) {
    if (i.value().active == true) {
      activeList.append(i.value());
    }
  }
  return activeList;
}

// ######## Request methods ########
// ######## private methods ########
void MultivariateDataWidget::removeData() {
  this->dataChannels.clear();
  this->customChannels.clear();
  this->ui->Details_ListWidget->clear();
}

void MultivariateDataWidget::setDataChannelsList() {
  QList<rawMetaData> rawData = this->sv->getMultivariateDimensionData();

  // build new data structures
  for (rawMetaData raw : rawData) {
    channelMetaData channel;

    channel.active = false;
    channel.isCustomChannel = false;
    channel.entryIndex = raw.entryIndex;
    channel.description = raw.description;
    channel.infoText = raw.infoText;

    // create new listItem
    MultivariateDataListItemWidget* listWidgetItem =
        new MultivariateDataListItemWidget(
            this, channel.mappingValue, channel.color, channel.isCustomChannel,
            0, 0, channel.description, channel.infoText);

    channel.widItem = listWidgetItem;

    this->dataChannels.insert(listWidgetItem, channel);
  }
}

void MultivariateDataWidget::connectChanneltoDataWidget(
    MultivariateDataListItemWidget* listWidgetItem) {
  connect(listWidgetItem, &MultivariateDataListItemWidget::rangeChanged, this,
          &MultivariateDataWidget::listItem_rangeChanged);

  connect(listWidgetItem, &MultivariateDataListItemWidget::mappingValueChanged,
          this, &MultivariateDataWidget::listItem_mappingValueChanged);

  connect(listWidgetItem, &MultivariateDataListItemWidget::colorChanged, this,
          &MultivariateDataWidget::listItem_colorChanged);
}

void MultivariateDataWidget::addCustomWidgetToListWidget(
    MultivariateDataListItemWidget* customWidget) {
  QListWidget* list = ui->Details_ListWidget;
  const int customItemType = 1042;

  // Creates a new row for given list with given type
  QListWidgetItem* row = new QListWidgetItem(list, customItemType);

  // Adds new row to list view
  list->addItem(row);
  row->setSizeHint(customWidget->minimumSizeHint());

  // Adds custom widget into given row
  list->setItemWidget(row, customWidget);

  customWidget->setSelectionCheckBoxState(row->isSelected());

  // to keep MultivariateDataListItemWidgets selection checkbox and
  // QListWidgetItem selection synchronized
  connect(customWidget,
          &MultivariateDataListItemWidget::selectionChangedByCheckBox, this,
          &MultivariateDataWidget::listWidgetCheckBoxSelectionChanged);
  list->update();
};

void MultivariateDataWidget::addListItemToListWidget(QListWidgetItem* item) {
  this->ui->Details_ListWidget->addItem(item);
}

void MultivariateDataWidget::setMixedColor() {
  int red = 0;
  int green = 0;
  int blue = 0;
  int activechannels = 0;

  // Iterate over all channels given by data
  for (auto i = dataChannels.cbegin(), end = dataChannels.cend(); i != end;
       ++i) {
    if (i.value().active == true) {
      red += i.value().color.red();
      green += i.value().color.green();
      blue += i.value().color.blue();

      activechannels += 1;
    }
  }

  // Iterate over all custom channels
  for (auto j = customChannels.cbegin(), end = customChannels.cend(); j != end;
       ++j) {
    if (j.value().active == true) {
      red += j.value().color.red();
      green += j.value().color.green();
      blue += j.value().color.blue();

      activechannels += 1;
    }
  }

  if (activechannels > 0) {
    QColor mixedColor = QColor(red / activechannels, green / activechannels,
                               blue / activechannels);

    this->setColor(mixedColor);
  } else {
    this->setColor(QColor(249, 240, 107));
  }
}

void MultivariateDataWidget::setColor(QColor color) {
  // Build Style Sheet
  QString clStr = "background-color:";
  clStr.operator+=(color.name());
  clStr.operator+=(" ; border: 1px solid black; ");

  QPushButton* clButt = ui->Details_MixedColor_ColorButton;
  clButt->setStyleSheet(clStr);
  clButt->update();
}

void MultivariateDataWidget::enableOverviewUI() {
  ui->Overview_MappingStrategy_Label->setEnabled(true);
  ui->Overview_MappingStrategy_DropDownList->setEnabled(true);

  ui->Overview_AvgMinMax_Label->setEnabled(true);
  ui->Overview_AvgMin_LineEdit->setEnabled(true);
  ui->Overview_AvgMax_LineEdit->setEnabled(true);
  ui->Overview_StdDevMinMax_Label->setEnabled(true);
  ui->Overview_StdDevMin_LineEdit->setEnabled(true);
  ui->Overview_StdDevMax_LineEdit->setEnabled(true);

  ui->Overview_MappingStrategy_Label->update();
  ui->Overview_MappingStrategy_DropDownList->update();
  ui->Overview_AvgMinMax_Label->update();
  ui->Overview_AvgMin_LineEdit->update();
  ui->Overview_AvgMax_LineEdit->update();
  ui->Overview_StdDevMinMax_Label->update();
  ui->Overview_StdDevMin_LineEdit->update();
  ui->Overview_StdDevMax_LineEdit->update();
}

void MultivariateDataWidget::disableOverviewUI() {
  ui->Overview_CheckBox->blockSignals(true);
  ui->Overview_CheckBox->setChecked(false);
  ui->Overview_CheckBox->blockSignals(false);

  ui->Overview_MappingStrategy_Label->setEnabled(false);
  ui->Overview_MappingStrategy_DropDownList->setEnabled(false);

  ui->Overview_AvgMinMax_Label->setEnabled(false);
  ui->Overview_AvgMin_LineEdit->setEnabled(false);
  ui->Overview_AvgMax_LineEdit->setEnabled(false);
  ui->Overview_StdDevMinMax_Label->setEnabled(false);
  ui->Overview_StdDevMin_LineEdit->setEnabled(false);
  ui->Overview_StdDevMax_LineEdit->setEnabled(false);

  ui->Overview_MappingStrategy_Label->update();
  ui->Overview_MappingStrategy_DropDownList->update();
  ui->Overview_AvgMinMax_Label->update();
  ui->Overview_AvgMin_LineEdit->update();
  ui->Overview_AvgMax_LineEdit->update();
  ui->Overview_StdDevMinMax_Label->update();
  ui->Overview_StdDevMin_LineEdit->update();
  ui->Overview_StdDevMax_LineEdit->update();
}

void MultivariateDataWidget::enableDetailsUI() {
  ui->Details_HLayout2->setEnabled(true);
  ui->Details_Header_Description_Label->setEnabled(true);
  ui->Details_Header_Mapping_Label->setEnabled(true);
  ui->Details_ListWidget->setEnabled(true);
  ui->Details_RemoveSelected_PushButton->setEnabled(true);

  ui->Details_Activation_CheckBox->update();
  ui->Details_HLayout2->update();
  ui->Details_Header_Description_Label->update();
  ui->Details_Header_Mapping_Label->update();
  ui->Details_ListWidget->update();
  ui->Details_RemoveSelected_PushButton->update();
}

void MultivariateDataWidget::disableDetailsUI() {
  ui->Details_Activation_CheckBox->blockSignals(true);
  ui->Details_Activation_CheckBox->setChecked(false);
  ui->Details_Activation_CheckBox->blockSignals(false);

  ui->Details_HLayout2->setEnabled(false);
  ui->Details_Header_Description_Label->setEnabled(false);
  ui->Details_Header_Mapping_Label->setEnabled(false);
  ui->Details_ListWidget->setEnabled(false);
  ui->Details_RemoveSelected_PushButton->setEnabled(false);

  ui->Details_Activation_CheckBox->update();
  ui->Details_HLayout2->update();
  ui->Details_Header_Description_Label->update();
  ui->Details_Header_Mapping_Label->update();
  ui->Details_ListWidget->update();
  ui->Details_RemoveSelected_PushButton->update();
}

void MultivariateDataWidget::enableEffZUI() {
  ui->EffZ_MappingStrategy_Label->setEnabled(true);
  ui->EffZ_MappingStrategy_DropDownList->setEnabled(true);

  ui->EffZ_EffZMinMax_Label->setEnabled(true);
  ui->EffZ_EffZMin_LineEdit->setEnabled(true);
  ui->EffZ_EffZMax_LineEdit->setEnabled(true);
  ui->EffZ_DensityMinMax_Label->setEnabled(true);
  ui->EffZ_DensityMin_LineEdit->setEnabled(true);
  ui->EffZ_DensityMax_LineEdit->setEnabled(true);

  ui->EffZ_EffZMinMax_Label->update();
  ui->EffZ_EffZMin_LineEdit->update();
  ui->EffZ_EffZMax_LineEdit->update();
  ui->EffZ_DensityMinMax_Label->update();
  ui->EffZ_DensityMin_LineEdit->update();
  ui->EffZ_DensityMax_LineEdit->update();
}

void MultivariateDataWidget::disableEffZUI() {
  ui->EffZ_CheckBox->blockSignals(true);
  ui->EffZ_CheckBox->setChecked(false);
  ui->EffZ_CheckBox->blockSignals(false);

  ui->EffZ_MappingStrategy_Label->setEnabled(false);
  ui->EffZ_MappingStrategy_DropDownList->setEnabled(false);

  ui->EffZ_EffZMinMax_Label->setEnabled(false);
  ui->EffZ_EffZMin_LineEdit->setEnabled(false);
  ui->EffZ_EffZMax_LineEdit->setEnabled(false);
  ui->EffZ_DensityMinMax_Label->setEnabled(false);
  ui->EffZ_DensityMin_LineEdit->setEnabled(false);
  ui->EffZ_DensityMax_LineEdit->setEnabled(false);

  ui->EffZ_EffZMinMax_Label->update();
  ui->EffZ_EffZMin_LineEdit->update();
  ui->EffZ_EffZMax_LineEdit->update();
  ui->EffZ_DensityMinMax_Label->update();
  ui->EffZ_DensityMin_LineEdit->update();
  ui->EffZ_DensityMax_LineEdit->update();
}

void MultivariateDataWidget::setOverviewOptions() {
  QComboBox* dropBox = ui->Overview_MappingStrategy_DropDownList;
  for (int i = 0; i < OverviewStrategyOption::overview_COUNT; i++) {
    dropBox->addItem(
        getOverviewStrategyDetails(OverviewStrategyOption(i)).longname);
  }
}

void MultivariateDataWidget::setEffZOptions() {
  QComboBox* dropBox = ui->EffZ_MappingStrategy_DropDownList;
  for (int i = 0; i < EffZStrategyOption::Eff_COUNT; i++) {
    dropBox->addItem(getEffZStrategyDetails(EffZStrategyOption(i)).longname);
  }
}

void MultivariateDataWidget::setInitUIValues() {
  setInitUIOverviewValues();
  setInitUIEffZValues();
}

void MultivariateDataWidget::setInitUIOverviewValues() {
  setInitUILineEdit(ui->Overview_AvgMin_LineEdit, &this->overviewAvgMin);
  setInitUILineEdit(ui->Overview_AvgMax_LineEdit, &this->overviewAvgMax);

  setInitUILineEdit(ui->Overview_StdDevMin_LineEdit, &this->overviewStdDevMin);
  setInitUILineEdit(ui->EffZ_DensityMax_LineEdit, &this->overviewStdDevMax);

  QComboBox* dropBox = ui->Overview_MappingStrategy_DropDownList;
  int currentOptionIndex = dropBox->currentIndex();
  if (currentOptionIndex > -1) {
    this->overviewStrategy = OverviewStrategyOption(currentOptionIndex);
  }
}

void MultivariateDataWidget::setInitUIEffZValues() {
  setInitUILineEdit(ui->EffZ_EffZMin_LineEdit, &this->effZ_EffZMin);
  setInitUILineEdit(ui->EffZ_EffZMax_LineEdit, &this->effZ_EffZMax);

  setInitUILineEdit(ui->EffZ_DensityMin_LineEdit, &this->effZ_DensityMin);
  setInitUILineEdit(ui->EffZ_DensityMax_LineEdit, &this->effZ_DensityMax);

  QComboBox* dropBox = ui->EffZ_MappingStrategy_DropDownList;
  int currentOptionIndex = dropBox->currentIndex();
  if (currentOptionIndex > -1) {
    this->effZStrategy = EffZStrategyOption(currentOptionIndex);
  }
}

void MultivariateDataWidget::setInitUILineEdit(QLineEdit* lEdit,
                                               float* thisValue) {
  QString text = lEdit->text();
  bool flag = false;
  float value = text.toFloat(&flag);
  if (flag) {
    // Init set at UI
    *thisValue = value;
  } else {
    if (*thisValue >= 0.0) {
      // Init set in backend
      lEdit->setText(QString::number(*thisValue));
    } else {
      // At both no init value set
      *thisValue = 0.0;
      lEdit->setText(QString::number(*thisValue));
    }
  }
}

void MultivariateDataWidget::LineEdit_TextChanged(QLineEdit* lEdit,
                                                  float* thisValue,
                                                  ActiveMode act) {
  QString text = lEdit->text();
  text = text.trimmed();

  bool flag;
  float mappingValue = text.toFloat(&flag);

  if (flag) {
    *thisValue = mappingValue;
    if (this->activeMode == act) {
      Q_EMIT multivariateDataVisChanged();
    }
  } else {
    lEdit->setText(QString::number(*thisValue));
  }
}

// ######## private methods ########
// ######## Slots for list items ########

void MultivariateDataWidget::listItem_rangeChanged(
    MultivariateDataListItemWidget* item, int min, int max) {
  if (this->dataChannels.contains(item)) {
    // entry with given key exists in dataChannels
    this->dataChannels[item].channelMin = min;
    this->dataChannels[item].channelMax = max;

  } else if (this->customChannels.contains(item)) {
    // entry with given key exists in customChannels
    this->customChannels[item].channelMin = min;
    this->customChannels[item].channelMax = max;
  }
  Q_EMIT multivariateDataVisChanged();
}

void MultivariateDataWidget::listItem_mappingValueChanged(
    MultivariateDataListItemWidget* item, int mappingValue) {
  if (this->dataChannels.contains(item)) {
    // entry with given key exists in dataChannels
    this->dataChannels[item].mappingValue = mappingValue;
  } else if (this->customChannels.contains(item)) {
    // entry with given key exists in customChannels
    this->customChannels[item].mappingValue = mappingValue;
  }
  Q_EMIT multivariateDataVisChanged();
}

void MultivariateDataWidget::listItem_colorChanged(
    MultivariateDataListItemWidget* item, QColor color) {
  if (this->dataChannels.contains(item)) {
    // entry with given key exists in dataChannels
    this->dataChannels[item].color = color;

  } else if (this->customChannels.contains(item)) {
    // entry with given key exists in customChannels
    this->customChannels[item].color = color;
  }

  this->setMixedColor();
  Q_EMIT multivariateDataVisChanged();
}

// ######## Slots for list items ########
// ######## Slots for data set ##########

void MultivariateDataWidget::data_channelsChanged() {
  removeData();
  setDataChannelsList();
}

// ######## Slots for data set ###################
// ######## Slots for own UI items ###############

void MultivariateDataWidget::listWidgetSelcetionsChanged() {
  int count = ui->Details_ListWidget->count();
  for (int i = 0; i < count; i++) {
    QListWidgetItem* row = ui->Details_ListWidget->item(i);
    MultivariateDataListItemWidget* customItem =
        (MultivariateDataListItemWidget*)ui->Details_ListWidget->itemWidget(
            row);
    customItem->setSelectionCheckBoxState(row->isSelected());
  }
}

void MultivariateDataWidget::listWidgetCheckBoxSelectionChanged() {
  int count = ui->Details_ListWidget->count();
  ui->Details_ListWidget->blockSignals(true);
  for (int i = 0; i < count; i++) {
    QListWidgetItem* row = ui->Details_ListWidget->item(i);
    MultivariateDataListItemWidget* customItem =
        (MultivariateDataListItemWidget*)ui->Details_ListWidget->itemWidget(
            row);

    row->setSelected(customItem->getSelectionCheckboxState());
  }
  ui->Details_ListWidget->blockSignals(false);
};

void MultivariateDataWidget::overview_Checkbox_clicked(bool checked) {
  if (checked) {
    this->activeMode = ActiveMode::overview;
    this->enableOverviewUI();
    this->disableDetailsUI();
    this->disableEffZUI();
  } else {
    this->activeMode = ActiveMode::none;
    this->disableOverviewUI();
  }
  Q_EMIT multivariateDataVisChanged();
}

void MultivariateDataWidget::overview_MappingStrategyDropdown_OptionChanged(
    int index) {
  if (index != -1) {
    this->overviewStrategy = OverviewStrategyOption(index);
    if (this->activeMode == ActiveMode::overview) {
      Q_EMIT multivariateDataVisChanged();
    }
  }
}

void MultivariateDataWidget::overview_AvgMin_TextChanged() {
  LineEdit_TextChanged(ui->Overview_AvgMin_LineEdit, &this->overviewAvgMin,
                       ActiveMode::overview);
}

void MultivariateDataWidget::overview_AvgMax_TextChanged() {
  LineEdit_TextChanged(ui->Overview_AvgMax_LineEdit, &this->overviewAvgMax,
                       ActiveMode::overview);
}

void MultivariateDataWidget::overview_StdDevMin_TextChanged() {
  LineEdit_TextChanged(ui->Overview_StdDevMin_LineEdit,
                       &this->overviewStdDevMin, ActiveMode::overview);
}

void MultivariateDataWidget::overview_StdDevMax_TextChanged() {
  LineEdit_TextChanged(ui->Overview_StdDevMax_LineEdit,
                       &this->overviewStdDevMax, ActiveMode::overview);
}

void MultivariateDataWidget::detailsCheckBox_clicked(bool checked) {
  if (checked) {
    this->activeMode = ActiveMode::details;
    this->enableDetailsUI();

    // gray-out range button if no spectrum data type is loaded
    QString dataType = sv->getLoadedDataType();
    if (!dataType.contains("Spectrum")) {
      ui->Details_Add_AddRage_PushButton->setEnabled(false);
    }

    this->disableOverviewUI();
    this->disableEffZUI();
  } else {
    this->activeMode = ActiveMode::none;
    this->disableDetailsUI();
  }
  Q_EMIT multivariateDataVisChanged();
}

void MultivariateDataWidget::addChannelPushButton_clicked() {
  // create dialog
  MultivariateDataAddChannel* addChannelDialog =
      new MultivariateDataAddChannel(this, &this->dataChannels);

  // execute Dialog and waits until closed
  if (addChannelDialog->exec() == QDialog::Accepted) {
    QList<QListWidgetItem*> selectedItems =
        addChannelDialog->getSelectedItems();

    for (QListWidgetItem* row : selectedItems) {
      MultivariateDataListItemWidget* item =
          addChannelDialog->getCustomWidgetFromListItem(row);
      dataChannels[item].active = true;
      dataChannels[item].isCustomChannel = false;

      minMaxDecider(&dataChannels[item].channelMin,
                    &dataChannels[item].channelMax);

      item->disconnect();

      this->addCustomWidgetToListWidget(item);
      this->connectChanneltoDataWidget(item);
      this->setMixedColor();
    }
    addChannelDialog->clearSelection();

    if (this->activeMode == ActiveMode::details) {
      Q_EMIT multivariateDataVisChanged();
    }
  }
}

void MultivariateDataWidget::addRangePushButton_clicked() {
  channelMetaData metaData;
  MultivariateDataAddRange* addRangeDialog = new MultivariateDataAddRange(
      this, this->dataChannels.size(), &this->customChannels, &metaData);

  // execute Dialog and waits until closed
  if (addRangeDialog->exec() == QDialog::Accepted) {
    // process "new" channel

    minMaxDecider(&metaData.channelMin, &metaData.channelMax);

    // create new listItem
    MultivariateDataListItemWidget* customWidget =
        new MultivariateDataListItemWidget(
            this, metaData.mappingValue, metaData.color,
            metaData.isCustomChannel, metaData.channelMin, metaData.channelMax,
            metaData.description);

    metaData.widItem = customWidget;
    metaData.active = true;
    metaData.isCustomChannel = true;

    customChannels.insert(customWidget, metaData);

    this->addCustomWidgetToListWidget(customWidget);
    this->connectChanneltoDataWidget(customWidget);
    this->setMixedColor();

    if (this->activeMode == ActiveMode::details) {
      Q_EMIT multivariateDataVisChanged();
    }
  }
}

void MultivariateDataWidget::removeSelectedPushButton_clicked() {
  // get al selected rows
  QList<QListWidgetItem*> selectedItems =
      this->ui->Details_ListWidget->selectedItems();

  for (QListWidgetItem* rowItem : selectedItems) {
    MultivariateDataListItemWidget* item =
        (MultivariateDataListItemWidget*)this->ui->Details_ListWidget
            ->itemWidget(rowItem);
    item->disconnect();

    if (this->customChannels.contains(item)) {
      this->customChannels.remove(item);
    } else if (this->dataChannels.contains(item)) {
      this->dataChannels[item].active = false;
    }

    rowItem->setHidden(true);

    if (this->activeMode == ActiveMode::details) {
      Q_EMIT multivariateDataVisChanged();
    }
  }
  ui->Details_ListWidget->clearSelection();
  this->setMixedColor();
}

void MultivariateDataWidget::effZ_Checkbox_clicked(bool checked) {
  if (checked) {
    this->activeMode = ActiveMode::effZ;
    this->enableEffZUI();
    this->disableOverviewUI();
    this->disableDetailsUI();
  } else {
    this->activeMode = ActiveMode::none;
    this->disableEffZUI();
  }
  Q_EMIT multivariateDataVisChanged();
}

void MultivariateDataWidget::effZ_MappingStrategyDropdown_OptionChanged(
    int index) {
  if (index != -1) {
    this->effZStrategy = EffZStrategyOption(index);
    if (this->activeMode == ActiveMode::effZ) {
      Q_EMIT multivariateDataVisChanged();
    }
  }
}

void MultivariateDataWidget::effZ_EffZMin_TextChanged() {
  LineEdit_TextChanged(ui->EffZ_EffZMin_LineEdit, &this->effZ_EffZMin,
                       ActiveMode::effZ);
}

void MultivariateDataWidget::effZ_EffZMax_TextChanged() {
  LineEdit_TextChanged(ui->EffZ_EffZMax_LineEdit, &this->effZ_EffZMax,
                       ActiveMode::effZ);
}

void MultivariateDataWidget::effZ_DensityMin_TextChanged() {
  LineEdit_TextChanged(ui->EffZ_DensityMin_LineEdit, &this->effZ_DensityMin,
                       ActiveMode::effZ);
}

void MultivariateDataWidget::effZ_DensityMax_TextChanged() {
  LineEdit_TextChanged(ui->EffZ_DensityMax_LineEdit, &this->effZ_DensityMax,
                       ActiveMode::effZ);
}

// ######## Slots for own UI items ###############
