#pragma once

#include <PluginVisSlice/MultivariateDataWidget/helpingStructures.hpp>
#include <PluginVisSlice/MultivariateDataWidget/multivariateDataListItem.hpp>

#include <QColor>
#include <QDialog>
#include <QHash>
#include <QLineEdit>
#include <QPair>
#include <QPushButton>
#include <QWidget>

namespace Ui {
class AddRange_Dialog;
}

/**
 * @brief This class is a dialog to add custom channels.
 */
class MultivariateDataAddRange : public QDialog {
  Q_OBJECT

 public:
  explicit MultivariateDataAddRange(
      QWidget* parent, int dataChannelsCount,
      QHash<MultivariateDataListItemWidget*, channelMetaData>* customChannels,
      channelMetaData* channel);
  ~MultivariateDataAddRange();

 private:
  Ui::AddRange_Dialog* ui;
  QHash<MultivariateDataListItemWidget*, channelMetaData>* customChannels;
  channelMetaData* channel;

  QString getMaxRange();
  QString getMinRange();

 private Q_SLOTS:

  void minRangeLineEdit_TextChanged();
  void maxRangeLineEdit_TextChanged();
  void selectionColorButton_clicked();
};
