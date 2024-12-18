#pragma once

#include <QColor>
#include <QColorDialog>
#include <QDebug>
#include <QListWidgetItem>
#include <QObject>
#include <VoxieBackend/Data/SeriesDimension.hpp>

class MultivariateDataListItemWidget;
// Static methods

/**
 * This static methode opens a modal color picker dialog.
 * By default the given start color is picked.
 * If dialog was accepted it returns the picked color.
 * Else it returns the start color.
 * @param QWidget* parent: Parent widget that gets the colorpicker gets stacked
 * on top.
 * @param QColor startcolor: Color that is default selected at opening.
 * @returns QColor: Picked color
 */
QColor getColorFromColorDialog(QWidget* parent, QColor startColor);

/**
 * This static methode builds a stylesheet for for a color button which displays
 * a color. This color button should be used to open a color picker and to
 * display the picked color after accepting the picker.
 * @param QColor color: Color that the button should display
 * @returns QString: Stylesheet that should be user for a color button.
 */
QString getStylesheetForColorButton(QColor color);

/**
 * This static methode switches values of the given variables if min > max.
 * @param int* min: Pointer to the variable that should be smaller after this.
 * @param int* max: Pointer to the variable that should be bigger after this.
 */
void minMaxDecider(int* min, int* max);

/**This static methode returns a value that is mapped from one scale to another.
 * @param double sourceScaleMin: Lower end of the source scale.
 * @param double sourceScaleMax: Higher end of the source scale.
 * @param double targetScaleMin: Lower end of the target scale.
 * @param double targetScaleMax: Higher end of the target scale.
 * @param double valueInSourceScale: Value that should be mapped.
 * @returns double: Mapped value in target scale.
 */
double mapValueFromSourceScaleToTargetScale(double sourceScaleMin,
                                            double sourceScaleMax,
                                            double targetScaleMin,
                                            double targetScaleMax,
                                            double valueInSourceScale);

extern const int QColorHueMin;
extern const int QColorHueMax;
extern const QString QColorHueUnit;
extern const int QColorSatMin;
extern const int QColorSatMax;
extern const int QColorValueMin;
extern const int QColorValueMax;

/**
 * This struct is meant to contain meta infomation for multivariate data
 * channels.
 */
struct channelMetaData {
  MultivariateDataListItemWidget* widItem = nullptr;
  bool active = false;
  bool isCustomChannel = false;

  int channelMin = 0;  // use channelMin only if isCustomChannel == true;
  int channelMax = 1;  // use channelMax only if isCustomChannel == true;

  QString description = "";
  QString infoText = "";
  vx::SeriesDimension::EntryKey entryIndex =
      0;  // use entryIndex only if isCustomChannel ==  false;

  int mappingValue = 50;
  QColor color = QColor(249, 240, 107);
};

/**
 * This struct is meant to be used to hold meta infomation intermediatly for
 * multivariate data channels received by backend.
 */
struct rawMetaData {
  QString description = "";
  QString infoText;
  vx::SeriesDimension::EntryKey entryIndex = 0;
};

/**
 * This enum hold all possible activie modes of the multivariate data widget.
 */
enum ActiveMode { overview, details, effZ, none };

/**
 * This struct holds infomation of a mapping strategy
 */
struct MappingStrategy {
  QString longname = "";
  QStringList targetScaling = {};
  QStringList sourceScaling = {};
};

// ## overview structures ####
/**
 * This enum holds all strategie options for multivariate data overview mode.
 * No indexes may be specified to enable iteration.
 */
enum OverviewStrategyOption {
  Avg2Hue_StdDev2Sat,
  StdDev2Hue_Avg2Sat,
  Avg2Value_StdDev2Sat,
  overview_COUNT
};

/**
 * This static map holds mapping strategies infomation.
 * For each OverviewStrategyOption one MappingStrategy is defined here.
 * This map should not be used directly. To retrive the contained infomation use
 * getOverviewStrategyDetails.
 */
extern const QMap<OverviewStrategyOption, MappingStrategy> overview_strategyMap;

/**
 * This static methode should be used to get the corresponding MappingStrategy
 * for a given OverviewStrategyOption.
 * @param OverviewStrategyOption enumOption: Option for multivariate data in
 * overview mode.
 * @returns MappingStrategy: The mapping strategy corresponding to the given
 * strategie option.
 */
inline MappingStrategy getOverviewStrategyDetails(
    OverviewStrategyOption enumOption) {
  return overview_strategyMap.value(enumOption);
};

// ## overview structures ####
// ## Eff.Z structures ####
/**
 * This enum holds all strategie options for multivariate data overview mode.
 * No indexes may be specified to enable iteration.
 */
enum EffZStrategyOption {
  EffZ2Hue_Density2Value,
  EffZ2Hue_Density2Sat,
  Density2Hue_EffZ2Sat,
  EffZ2Value_Density2Sat,
  Eff_COUNT
};

/**
 * This static map holds mapping strategies infomation.
 * For each EffZStrategyOption one MappingStrategy is defined here.
 * This map should not be used directly. To retrive the contained infomation use
 * getEffZStrategyDetails.
 */
extern const QMap<EffZStrategyOption, MappingStrategy> eff_strategyMap;

/**
 * This static methode should be used to get the corresponding MappingStrategy
 * for a given EffZStrategyOption.
 * @param EffZStrategyOption enumOption: Option for multivariate data in
 * effective Z (eff.Z) mode.
 * @returns MappingStrategy: The mapping strategy corresponding to the given
 * strategie option.
 */
inline MappingStrategy getEffZStrategyDetails(EffZStrategyOption enumOption) {
  return eff_strategyMap.value(enumOption);
};

// ## Eff.Z structures ####
