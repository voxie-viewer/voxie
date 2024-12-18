#include "multivariateDataAddRange.hpp"

// automatacly created by Qt preprocessor
#include "ui_multivariateDataAddRange.h"

MultivariateDataAddRange::MultivariateDataAddRange(
    QWidget* parent, int dataChannelsCount,
    QHash<MultivariateDataListItemWidget*, channelMetaData>* customChannels,
    channelMetaData* channel)
    : QDialog(parent), ui(new Ui::AddRange_Dialog) {
  ui->setupUi(this);

  this->channel = channel;
  this->channel->isCustomChannel = true;
  this->customChannels = customChannels;

  ui->Count_LineEdit->setText(QString::number(dataChannelsCount));

  connect(ui->SelectionMin_LineEdit, &QLineEdit::editingFinished, this,
          &MultivariateDataAddRange::minRangeLineEdit_TextChanged);

  connect(ui->SelectionMax_LineEdit, &QLineEdit::editingFinished, this,
          &MultivariateDataAddRange::maxRangeLineEdit_TextChanged);

  connect(ui->Selection_ColorButton, &QPushButton::clicked, this,
          &MultivariateDataAddRange::selectionColorButton_clicked);
}

MultivariateDataAddRange::~MultivariateDataAddRange() { delete ui; }

void MultivariateDataAddRange::minRangeLineEdit_TextChanged() {
  QString text = this->getMinRange();
  bool flag;
  int number = text.toInt(&flag);
  if (flag) {
    this->channel->channelMin = number;
  }
}

void MultivariateDataAddRange::maxRangeLineEdit_TextChanged() {
  QString text = this->getMaxRange();
  bool flag;
  int number = text.toInt(&flag);
  if (flag) {
    this->channel->channelMax = number;
  }
}

void MultivariateDataAddRange::selectionColorButton_clicked() {
  QPushButton* clButt = ui->Selection_ColorButton;

  QColor color =
      getColorFromColorDialog(this, clButt->palette().window().color());

  clButt->setStyleSheet(getStylesheetForColorButton(color));
  clButt->update();

  this->channel->color = color;
}

QString MultivariateDataAddRange::getMaxRange() {
  return this->ui->SelectionMax_LineEdit->text();
}

QString MultivariateDataAddRange::getMinRange() {
  return this->ui->SelectionMin_LineEdit->text();
}
