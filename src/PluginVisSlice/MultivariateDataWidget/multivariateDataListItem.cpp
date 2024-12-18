#include "multivariateDataListItem.hpp"
#include "ui_multivariateDataListItem.h"

MultivariateDataListItemWidget::MultivariateDataListItemWidget(
    QWidget* parent, int mappingScale, QColor color, bool isCustomChannel,
    int channelMin, int channelMax, QString shortDescription, QString infoText)
    : QWidget(parent), ui(new Ui::Multivariate_ListItem) {
  ui->setupUi(this);

  this->mappingScale = mappingScale;
  this->color = color;
  this->isCustomChannel = isCustomChannel;

  this->channelMin = channelMin;
  this->channelMax = channelMax;
  this->shortDescription = shortDescription;
  this->infoText = infoText;

  setColor(color);
  updateDescriptionLineEdit();
  updateMappingValueLineEdit();

  // Slots connected to own Widget (Callback Functions)
  connect(ui->Description_LineEdit, &QLineEdit::editingFinished, this,
          &MultivariateDataListItemWidget::descriptionLineEdit_TextChanged);

  connect(ui->MappingValue_LineEdit, &QLineEdit::editingFinished, this,
          &MultivariateDataListItemWidget::mappingValueLineEdit_TextChanged);

  connect(ui->Mapping_ColorButton, &QPushButton::clicked, this,
          &MultivariateDataListItemWidget::mappingColorButton_clicked);

  connect(ui->DescriptionInfo_Button, &QPushButton::clicked, this,
          &MultivariateDataListItemWidget::infoButton_clicked);

  connect(ui->Selection_checkBox, &QCheckBox::stateChanged,
          [this](bool checked) {
            Q_UNUSED(checked)
            Q_EMIT this->selectionChangedByCheckBox();
          });
};

MultivariateDataListItemWidget::~MultivariateDataListItemWidget() { delete ui; }

void MultivariateDataListItemWidget::descriptionLineEdit_TextChanged() {
  if (this->isCustomChannel) {
    // Matches int min and int max
    // e.g."100 - 9000" ->Matchlist[ (int) 100,(int) 9000 ]
    QRegularExpression regExpText("(\\d+,?)(?:\\W)+(\\d+,?)");
    QRegularExpressionMatch match;

    QString text = this->getListItemRange();
    text = text.trimmed();

    match = regExpText.match(text);

    if (!match.hasMatch()) {
      qWarning() << "ListItem - Wrong Syntax in range Input";
      this->updateDescriptionLineEdit();
    } else {
      QStringList matchList = match.capturedTexts();

      if (matchList.size() == 3) {
        // first match is ignored
        bool minFlag, maxFlag;
        int min = ((QString)matchList.at(1)).remove(',').toInt(&minFlag);
        int max = ((QString)matchList.at(2)).remove(',').toInt(&maxFlag);

        if (minFlag && maxFlag) {
          this->channelMin = min;
          this->channelMax = max;

          // Send Range to Parent (Signal)
          Q_EMIT rangeChanged(this, min, max);
        } else {
          qWarning() << "ListItem - Cant convert energie boundries to Int";
          this->updateDescriptionLineEdit();
        }
      }
    }
  } else {
    this->shortDescription = this->ui->Description_LineEdit->text();
  }
}

void MultivariateDataListItemWidget::mappingValueLineEdit_TextChanged() {
  QString text = ui->MappingValue_LineEdit->text();
  text = text.trimmed();

  bool mappingScaleFlag;
  int mappingValue = text.toInt(&mappingScaleFlag);

  if (mappingScaleFlag) {
    this->mappingScale = mappingValue;
    Q_EMIT mappingValueChanged(this, mappingValue);
  } else {
    qWarning() << "ListItem - Cant convert mapping value to Int";
    this->updateMappingValueLineEdit();
  }
}

void MultivariateDataListItemWidget::mappingColorButton_clicked() {
  QPushButton* clButt = ui->Mapping_ColorButton;

  QColor color =
      getColorFromColorDialog(this, clButt->palette().window().color());

  clButt->setStyleSheet(getStylesheetForColorButton(color));
  clButt->update();

  setColor(color);

  // Send Color to Parent (Signal)
  Q_EMIT colorChanged(this, color);
}

void MultivariateDataListItemWidget::infoButton_clicked() {
  QMessageBox* msgBox = new QMessageBox(this);
  msgBox->setModal(false);
  msgBox->setIcon(QMessageBox::Information);
  msgBox->setWindowTitle("Channel Info " + this->shortDescription);
  msgBox->setText(this->infoText);
  msgBox->setStandardButtons(QMessageBox::Close);
  msgBox->setDefaultButton(QMessageBox::Close);
  msgBox->show();
};

void MultivariateDataListItemWidget::updateDescriptionLineEdit() {
  QLineEdit* lineEdit = ui->Description_LineEdit;
  if (this->isCustomChannel) {
    QString minText = QString::number(this->channelMin);
    QString maxText = QString::number(this->channelMax);
    QString channelText = minText + " - " + maxText;
    lineEdit->setText(channelText);
  } else {
    lineEdit->setText(this->shortDescription);
  }
};

void MultivariateDataListItemWidget::updateMappingValueLineEdit() {
  ui->MappingValue_LineEdit->setText(QString::number(this->mappingScale));
}

QString MultivariateDataListItemWidget::getListItemRange() {
  return this->ui->Description_LineEdit->text();
}

void MultivariateDataListItemWidget::setColor(QColor color) {
  // Build Style Sheet
  QString clStr = "background-color:";
  clStr.operator+=(color.name());
  clStr.operator+=(" ; border: 1px solid black; ");

  QPushButton* clButt = ui->Mapping_ColorButton;
  clButt->setStyleSheet(clStr);
  clButt->update();
}

void MultivariateDataListItemWidget::setDescription_LineEdit_readOnly(
    bool readOnly) {
  this->ui->Description_LineEdit->setReadOnly(readOnly);
};

bool MultivariateDataListItemWidget::getSelectionCheckboxState() {
  return ui->Selection_checkBox->isChecked();
}

void MultivariateDataListItemWidget::setSelectionCheckBoxState(bool checked) {
  ui->Selection_checkBox->blockSignals(true);
  ui->Selection_checkBox->setChecked(checked);
  ui->Selection_checkBox->blockSignals(false);
};
