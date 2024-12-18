#include "helpingStructures.hpp"

QColor getColorFromColorDialog(QWidget* parent, QColor startColor) {
  QColorDialog* colorPicker = new QColorDialog();
  colorPicker->setOption(QColorDialog::DontUseNativeDialog);
  colorPicker->setOption(QColorDialog::ShowAlphaChannel);
  colorPicker->setCurrentColor(startColor);
  QObject::connect(parent, &QObject::destroyed, colorPicker,
                   &QObject::deleteLater);
  if (colorPicker->exec() == QDialog::Accepted) {
    return colorPicker->selectedColor();
  } else {
    return startColor;
  }
}

QString getStylesheetForColorButton(QColor color) {
  if (color.isValid()) {
    QString clStr = "background-color:";
    clStr.operator+=(color.name());
    clStr.operator+=(" ; border: 1px solid black; ");
    return clStr;
  } else {
    qDebug() << "Color not valid";
    return "";
  }
}

void minMaxDecider(int* min, int* max) {
  if (*min > *max) {
    int temp = *max;
    *max = *min;
    *min = temp;
  }
}

// Min and Max are given by QColor class see: https://doc.qt.io/qt-5/qcolor.html
const int QColorHueMin = 0;
const int QColorHueMax = 330;
const QString QColorHueUnit = "°";
const int QColorSatMin = 0;
const int QColorSatMax = 255;
const int QColorValueMin = 0;
const int QColorValueMax = 255;

double mapValueFromSourceScaleToTargetScale(double sourceScaleMin,
                                            double sourceScaleMax,
                                            double targetScaleMin,
                                            double targetScaleMax,
                                            double valueInSourceScale) {
  return (valueInSourceScale - sourceScaleMin) *
             (targetScaleMax - targetScaleMin) /
             (sourceScaleMax - sourceScaleMin) +
         targetScaleMin;
}

const QMap<OverviewStrategyOption, MappingStrategy> overview_strategyMap{
    {OverviewStrategyOption::Avg2Hue_StdDev2Sat,
     {"Hue<-Average, Saturation<-Std. Dev.",
      {"Hue", "Saturation"},
      {"Average", "Std. Dev."}}},
    {OverviewStrategyOption::StdDev2Hue_Avg2Sat,
     {"Hue<-Std. Dev., Saturation<-Average",
      {"Hue", "Saturation"},
      {"Std. Dev.", "Average"}}},
    {OverviewStrategyOption::Avg2Value_StdDev2Sat,
     {"Value<-Average, Saturation<-Std. Dev.",
      {"Value", "Saturation"},
      {"Average", "Std. Dev."}}}};

const QMap<EffZStrategyOption, MappingStrategy> eff_strategyMap{
    {EffZStrategyOption::EffZ2Hue_Density2Value,
     {"Hue<-Eff.Z, Value<-Density", {"Hue", "Value"}, {"Eff.Z", "Density"}}},
    {EffZStrategyOption::EffZ2Hue_Density2Sat,
     {"Hue<-Eff.Z, Saturation<-Density",
      {"Hue", "Saturation"},
      {"Eff.Z", "Density"}}},
    {EffZStrategyOption::Density2Hue_EffZ2Sat,
     {"Hue<-Density, Saturation<-Eff.Z",
      {"Hue", "Saturation"},
      {"Density", "Eff.Z"}}},
    {EffZStrategyOption::EffZ2Value_Density2Sat,
     {"Value<-Eff.Z, Saturation<-Density",
      {"Value", "Saturation"},
      {"Eff.Z", "Density"}}},
};
