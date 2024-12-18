#pragma once

// QDBusConnection should be included as early as possible:
// https://bugreports.qt.io/browse/QTBUG-48351 /
// https://bugreports.qt.io/browse/QTBUG-48377
#include <QtDBus/QDBusConnection>

#include <PluginVisSlice/MultivariateDataWidget/helpingStructures.hpp>
#include <PluginVisSlice/SliceVisualizer.hpp>
#include <VoxieBackend/Data/VolumeSeriesData.hpp>

// Cusstom UI Elements
#include <PluginVisSlice/MultivariateDataWidget/multivariateDataAddChannel.hpp>
#include <PluginVisSlice/MultivariateDataWidget/multivariateDataAddRange.hpp>
#include <PluginVisSlice/MultivariateDataWidget/multivariateDataListItem.hpp>

// Standart Qt UI Elements
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDialog>
#include <QHash>
#include <QList>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QWidget>

namespace Ui {
class MultivariateData_Widget;
}

class ImageLayer;
class Layer;

/**
 * This class is the main multivariate data class.
 * It handles some requests for its childs, for its own UI elements and
 * coordinates its childs.
 */
class MultivariateDataWidget : public QWidget {
  Q_OBJECT

 public:
  explicit MultivariateDataWidget(QWidget* parent, SliceVisualizer* sv);
  ~MultivariateDataWidget();
  ActiveMode getActiveMode();

  // getter for overview storage
  OverviewStrategyOption getOverviewStrategy();
  float getOverviewAvgMin();
  float getOverviewAvgMax();
  float getOverviewStdDevMin();
  float getOverviewStdDevMax();

  EffZStrategyOption getEffZStrategy();
  float getEffZ_EffZMin();
  float getEffZ_EffZMax();
  float getEffZ_DensityMin();
  float getEffZ_DensityMax();

  QList<channelMetaData> getActiveChannels();

 Q_SIGNALS:
  void multivariateDataVisChanged();

 private:
  ActiveMode activeMode;
  SliceVisualizer* sv;
  Ui::MultivariateData_Widget* ui;

  // overview data for reset
  OverviewStrategyOption overviewStrategy;
  float overviewAvgMin;
  float overviewAvgMax;
  float overviewStdDevMin;
  float overviewStdDevMax;

  // eff.Z data for reset
  EffZStrategyOption effZStrategy;
  float effZ_EffZMin;
  float effZ_EffZMax;
  float effZ_DensityMin;
  float effZ_DensityMax;

  /**
   * This hash map holds all channelMetaData for all multivariate data channels
   * that are given by loaded data. Each channelMetaData in this map coresponds
   * to exact one MultivariateDataListItemWidget. As map key the pointer to the
   * coresponding MultivariateDataListItemWidget is used.
   */
  QHash<MultivariateDataListItemWidget*, channelMetaData> dataChannels;

  /**
   * This hash map holds all channelMetaData for all multivariate data custom
   * range channels that are created by the user. Each channelMetaData in this
   * map coresponds to exact one MultivariateDataListItemWidget. As map key the
   * pointer to the coresponding MultivariateDataListItemWidget is used.
   */
  QHash<MultivariateDataListItemWidget*, channelMetaData> customChannels;

  /**
   * This methode deletes all channels from maps and list widget.
   */
  void removeData();

  /**
   * This methode retrieves all meta information from back end about loaded
   * channels, creates corresponding MultivariateDataListItemWidget and saves
   * them in dataChannels map.
   */
  void setDataChannelsList();

  /**
   * Connect parent dependent behavior to parent.
   * @param MultivariateDataListItemWidget* listWidgetItem: Custom list widget
   * item that should be connected
   */
  void connectChanneltoDataWidget(
      MultivariateDataListItemWidget* listWidgetItem);

  /**
   * This methode adds a MultivariateDataListItemWidget to the list widget so
   * that it is displayed there.
   * @param MultivariateDataListItemWidget* customWidget: item widget that
   * should be added to list widget.
   */
  void addCustomWidgetToListWidget(
      MultivariateDataListItemWidget* customWidget);

  /**
   * This methode adds an QListWidgetItem to the list widget.
   * This is useful to transfer a QListWidgetItem from a list widget to another.
   */
  void addListItemToListWidget(QListWidgetItem* item);

  /**
   * Iterrates over all active channels and calculates mixed color.
   * It sets the color of the mixed color button.
   */
  void setMixedColor();

  /**
   * Sets the mixed color button of the details mode.
   * @param QColor color: Color to which the mixed color button should be
   * changed.
   */
  void setColor(QColor color);

  void enableOverviewUI();
  void disableOverviewUI();

  void enableDetailsUI();
  void disableDetailsUI();

  void enableEffZUI();
  void disableEffZUI();

  void setOverviewOptions();
  void setEffZOptions();

  void setInitUIValues();
  void setInitUIOverviewValues();
  void setInitUIEffZValues();

  /**
   * Generic methode to set text of the given QLineEdit by a given floatvalue.
   * @param QLineEdit* lEdit: Pointer to a lineEdit UI element which its text
   * should be changed.
   * @param float* thisValue: Pointer to the variable that contains the value to
   * be used.
   */
  void setInitUILineEdit(QLineEdit* lEdit, float* thisValue);

  /**
   * Generic methode to change text of the given QLineEdit to the value that is
   * stored in this widgets backend. If the given ActiveModeis the actual active
   * mode multivariateDataVisChanged is emited.
   * @param QLineEdit* lEdit: Pointer to a lineEdit UI element which its text
   * should be saved in this widgets backend.
   * @param float* thisValue: Pointer to the Variable that the value should be
   * saved in.
   * @param ActiveMode act: mode within this action is occurred
   */
  void LineEdit_TextChanged(QLineEdit* lEdit, float* thisValue, ActiveMode act);

 public Q_SLOTS:

  // Slots for list items
  // uses listItem Pointer as hash key

  void listItem_rangeChanged(MultivariateDataListItemWidget* item, int min,
                             int max);
  void listItem_mappingValueChanged(MultivariateDataListItemWidget* item,
                                    int mappingValue);
  void listItem_colorChanged(MultivariateDataListItemWidget* item,
                             QColor color);

  // Slots for data set

  void data_channelsChanged();

  // ## Slots for overview view

  void listWidgetSelcetionsChanged();
  void listWidgetCheckBoxSelectionChanged();

  void overview_Checkbox_clicked(bool checked);
  void overview_MappingStrategyDropdown_OptionChanged(int index);
  void overview_AvgMin_TextChanged();
  void overview_AvgMax_TextChanged();
  void overview_StdDevMin_TextChanged();
  void overview_StdDevMax_TextChanged();

  // ## Slots for Details view
  void detailsCheckBox_clicked(bool checked);
  void addChannelPushButton_clicked();
  void addRangePushButton_clicked();
  void removeSelectedPushButton_clicked();

  // ## Slots for Eff.z & Density view
  void effZ_Checkbox_clicked(bool checked);
  void effZ_MappingStrategyDropdown_OptionChanged(int index);
  void effZ_EffZMin_TextChanged();
  void effZ_EffZMax_TextChanged();
  void effZ_DensityMin_TextChanged();
  void effZ_DensityMax_TextChanged();
};
