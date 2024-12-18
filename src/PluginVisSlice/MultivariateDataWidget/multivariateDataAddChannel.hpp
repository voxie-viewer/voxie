#pragma once

#include <PluginVisSlice/MultivariateDataWidget/helpingStructures.hpp>
#include <PluginVisSlice/MultivariateDataWidget/multivariateDataListItem.hpp>

#include <QDialog>
#include <QHash>
#include <QListWidget>
#include <QListWidgetItem>
#include <QWidget>

namespace Ui {
class AddChannel_Dialog;
}

/**
 * @brief This class represents a dialog to add channels given by loaded
 * multivariate data.
 */
class MultivariateDataAddChannel : public QDialog {
  Q_OBJECT

 public:
  explicit MultivariateDataAddChannel(
      QWidget* parent,
      QHash<MultivariateDataListItemWidget*, channelMetaData>* dataChannels);
  ~MultivariateDataAddChannel();
  QList<QListWidgetItem*> getSelectedItems();

  /**
   * This methode returns a MultivariateDataListItemWidget by given list widget
   * "row"-item.
   */
  MultivariateDataListItemWidget* getCustomWidgetFromListItem(
      QListWidgetItem* row);
  void clearSelection();

 public Q_SLOTS:

  void listWidgetSelcetionsChanged();
  void listWidgetCheckBoxSelectionChanged();

  // Slots for list items
  void listItem_colorChanged(MultivariateDataListItemWidget* key, QColor color);

 private:
  Ui::AddChannel_Dialog* ui;
  QHash<MultivariateDataListItemWidget*, channelMetaData>* dataChannels;
};
