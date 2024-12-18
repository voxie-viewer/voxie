#pragma once

#include <PluginVisSlice/MultivariateDataWidget/helpingStructures.hpp>

#include <QCheckBox>
#include <QColor>
#include <QColorDialog>
#include <QDebug>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QString>
#include <QWidget>

namespace Ui {
class Multivariate_ListItem;
}

/**
 * @brief This class handle all non-parent-dependent behavior for multivariate
 * data list items.
 */
class MultivariateDataListItemWidget : public QWidget {
  Q_OBJECT
 public:
  explicit MultivariateDataListItemWidget(QWidget* parent, int mappingScale,
                                          QColor color, bool isCustomChannel,
                                          int channelMin, int channelMax,
                                          QString shortDescription,
                                          QString infoText = "");
  ~MultivariateDataListItemWidget();

  // Signal to parent
 Q_SIGNALS:
  void rangeChanged(MultivariateDataListItemWidget* self, int min, int max);
  void colorChanged(MultivariateDataListItemWidget* self, QColor color);
  void mappingValueChanged(MultivariateDataListItemWidget* self,
                           int mappingValue);
  void selectionChangedByCheckBox();

 private:
  int mappingScale;
  QColor color;
  bool isCustomChannel;
  int channelMin;
  int channelMax;
  QString shortDescription;
  QString infoText;

  Ui::Multivariate_ListItem* ui;

  void setColor(QColor color);

  void updateDescriptionLineEdit();
  void updateMappingValueLineEdit();

  QString getListItemRange();

 private Q_SLOTS:

  void descriptionLineEdit_TextChanged();
  void mappingValueLineEdit_TextChanged();

  void mappingColorButton_clicked();
  void infoButton_clicked();

 public:
  void setDescription_LineEdit_readOnly(bool readOnly);
  bool getSelectionCheckboxState();
  void setSelectionCheckBoxState(bool checked);
};
