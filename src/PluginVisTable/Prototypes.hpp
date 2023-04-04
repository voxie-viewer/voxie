// This file was automatically generated by tools/update-node-prototypes.py
// All changes to this file will be lost

#pragma once

#include <QtCore/QJsonObject>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtGui/QQuaternion>
#include <QtGui/QVector3D>
#include <Voxie/Data/Color.hpp>
#include <Voxie/Data/ColorizerEntry.hpp>
#include <Voxie/Node/Node.hpp>
#include <Voxie/Node/Types.hpp>
#include <VoxieBackend/Data/DataType.hpp>

namespace vx {
#ifndef VOXIE_PROP_DEFINED_BucketCount
#define VOXIE_PROP_DEFINED_BucketCount
namespace PropType {
class BucketCount : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::BucketCount BucketCount = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_ColorMap
#define VOXIE_PROP_DEFINED_ColorMap
namespace PropType {
class ColorMap : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::ColorMap ColorMap = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_Column
#define VOXIE_PROP_DEFINED_Column
namespace PropType {
class Column : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::Column Column = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_ColumnColor
#define VOXIE_PROP_DEFINED_ColumnColor
namespace PropType {
class ColumnColor : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::ColumnColor ColumnColor = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_ColumnX
#define VOXIE_PROP_DEFINED_ColumnX
namespace PropType {
class ColumnX : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::ColumnX ColumnX = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_ColumnY
#define VOXIE_PROP_DEFINED_ColumnY
namespace PropType {
class ColumnY : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::ColumnY ColumnY = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_LogarithmicX
#define VOXIE_PROP_DEFINED_LogarithmicX
namespace PropType {
class LogarithmicX : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::LogarithmicX LogarithmicX = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_LogarithmicY
#define VOXIE_PROP_DEFINED_LogarithmicY
namespace PropType {
class LogarithmicY : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::LogarithmicY LogarithmicY = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_LowerBoundXFraction
#define VOXIE_PROP_DEFINED_LowerBoundXFraction
namespace PropType {
class LowerBoundXFraction : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::LowerBoundXFraction LowerBoundXFraction = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_Output
#define VOXIE_PROP_DEFINED_Output
namespace PropType {
class Output : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::Output Output = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_PointLimit
#define VOXIE_PROP_DEFINED_PointLimit
namespace PropType {
class PointLimit : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::PointLimit PointLimit = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_PointScale
#define VOXIE_PROP_DEFINED_PointScale
namespace PropType {
class PointScale : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::PointScale PointScale = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_RowLimit
#define VOXIE_PROP_DEFINED_RowLimit
namespace PropType {
class RowLimit : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::RowLimit RowLimit = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_SortColumn
#define VOXIE_PROP_DEFINED_SortColumn
namespace PropType {
class SortColumn : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::SortColumn SortColumn = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_SortOrder
#define VOXIE_PROP_DEFINED_SortOrder
namespace PropType {
class SortOrder : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::SortOrder SortOrder = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_Table
#define VOXIE_PROP_DEFINED_Table
namespace PropType {
class Table : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::Table Table = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_UpperBoundXFraction
#define VOXIE_PROP_DEFINED_UpperBoundXFraction
namespace PropType {
class UpperBoundXFraction : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::UpperBoundXFraction UpperBoundXFraction = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_ViewMargin
#define VOXIE_PROP_DEFINED_ViewMargin
namespace PropType {
class ViewMargin : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::ViewMargin ViewMargin = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_ViewPercentile
#define VOXIE_PROP_DEFINED_ViewPercentile
namespace PropType {
class ViewPercentile : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::ViewPercentile ViewPercentile = {};
}
#endif
inline namespace filter_prop {
class AutoScaleTableUnitsPropertiesEntry : public vx::PropertiesEntryBase {
  AutoScaleTableUnitsPropertiesEntry() = delete;

 public:
  ~AutoScaleTableUnitsPropertiesEntry();
  AutoScaleTableUnitsPropertiesEntry(vx::PropType::Table, vx::Node*);
  AutoScaleTableUnitsPropertiesEntry(vx::PropType::Output, vx::Node*);
};
class AutoScaleTableUnitsPropertiesBase {
 public:
  virtual ~AutoScaleTableUnitsPropertiesBase();
  virtual vx::Node* table() = 0;
  virtual QDBusObjectPath tableRaw() = 0;
  virtual vx::Node* output() = 0;
  virtual QDBusObjectPath outputRaw() = 0;
};
class AutoScaleTableUnitsPropertiesCopy
    : public AutoScaleTableUnitsPropertiesBase {
  QSharedPointer<const QMap<QString, QVariant>> _properties;

 public:
  AutoScaleTableUnitsPropertiesCopy(
      const QSharedPointer<const QMap<QString, QVariant>>& properties);
  vx::Node* table() override final;
  QDBusObjectPath tableRaw() override final;
  vx::Node* output() override final;
  QDBusObjectPath outputRaw() override final;
};
class AutoScaleTableUnitsProperties : public QObject,
                                      public AutoScaleTableUnitsPropertiesBase {
  Q_OBJECT
  vx::Node* _node;

 public:
  static const char* _getPrototypeJson();
  static QSharedPointer<vx::NodePrototype> getNodePrototype();
  AutoScaleTableUnitsProperties(vx::Node* parent);
  ~AutoScaleTableUnitsProperties();

  vx::Node* table() override final;
  QDBusObjectPath tableRaw() override final;
  static QSharedPointer<NodeProperty> tableProperty();
  static NodePropertyTyped<vx::types::NodeReference> tablePropertyTyped();
  void setTable(vx::Node* value);
 Q_SIGNALS:
  void tableChanged(vx::Node* value);

 public:
  // Q_PROPERTY(vx::Node* Table READ table WRITE setTable NOTIFY tableChanged)

  vx::Node* output() override final;
  QDBusObjectPath outputRaw() override final;
  static QSharedPointer<NodeProperty> outputProperty();
  static NodePropertyTyped<vx::types::OutputNodeReference>
  outputPropertyTyped();
  void setOutput(vx::Node* value);
 Q_SIGNALS:
  void outputChanged(vx::Node* value);

 public:
  // Q_PROPERTY(vx::Node* Output READ output WRITE setOutput NOTIFY
  // outputChanged)
};

}  // namespace filter_prop
inline namespace visualizer_prop {
class HistogramPropertiesEntry : public vx::PropertiesEntryBase {
  HistogramPropertiesEntry() = delete;

 public:
  ~HistogramPropertiesEntry();
  HistogramPropertiesEntry(vx::PropType::BucketCount, qint64);
  HistogramPropertiesEntry(vx::PropType::ColorMap, QList<vx::ColorizerEntry>);
  HistogramPropertiesEntry(vx::PropType::Column, QString);
  HistogramPropertiesEntry(vx::PropType::LogarithmicX, bool);
  HistogramPropertiesEntry(vx::PropType::LogarithmicY, bool);
  HistogramPropertiesEntry(vx::PropType::LowerBoundXFraction, double);
  HistogramPropertiesEntry(vx::PropType::Table, vx::Node*);
  HistogramPropertiesEntry(vx::PropType::UpperBoundXFraction, double);
};
class HistogramPropertiesBase {
 public:
  virtual ~HistogramPropertiesBase();
  virtual qint64 bucketCount() = 0;
  virtual qint64 bucketCountRaw() = 0;
  virtual QList<vx::ColorizerEntry> colorMap() = 0;
  virtual QList<
      std::tuple<double, std::tuple<double, double, double, double>, qint32>>
  colorMapRaw() = 0;
  virtual QString column() = 0;
  virtual QString columnRaw() = 0;
  virtual bool logarithmicX() = 0;
  virtual bool logarithmicXRaw() = 0;
  virtual bool logarithmicY() = 0;
  virtual bool logarithmicYRaw() = 0;
  virtual double lowerBoundXFraction() = 0;
  virtual double lowerBoundXFractionRaw() = 0;
  virtual vx::Node* table() = 0;
  virtual QDBusObjectPath tableRaw() = 0;
  virtual double upperBoundXFraction() = 0;
  virtual double upperBoundXFractionRaw() = 0;
};
class HistogramPropertiesCopy : public HistogramPropertiesBase {
  QSharedPointer<const QMap<QString, QVariant>> _properties;

 public:
  HistogramPropertiesCopy(
      const QSharedPointer<const QMap<QString, QVariant>>& properties);
  qint64 bucketCount() override final;
  qint64 bucketCountRaw() override final;
  QList<vx::ColorizerEntry> colorMap() override final;
  QList<std::tuple<double, std::tuple<double, double, double, double>, qint32>>
  colorMapRaw() override final;
  QString column() override final;
  QString columnRaw() override final;
  bool logarithmicX() override final;
  bool logarithmicXRaw() override final;
  bool logarithmicY() override final;
  bool logarithmicYRaw() override final;
  double lowerBoundXFraction() override final;
  double lowerBoundXFractionRaw() override final;
  vx::Node* table() override final;
  QDBusObjectPath tableRaw() override final;
  double upperBoundXFraction() override final;
  double upperBoundXFractionRaw() override final;
};
class HistogramProperties : public QObject, public HistogramPropertiesBase {
  Q_OBJECT
  vx::Node* _node;

 public:
  static const char* _getPrototypeJson();
  static QSharedPointer<vx::NodePrototype> getNodePrototype();
  HistogramProperties(vx::Node* parent);
  ~HistogramProperties();

  qint64 bucketCount() override final;
  qint64 bucketCountRaw() override final;
  static QSharedPointer<NodeProperty> bucketCountProperty();
  static NodePropertyTyped<vx::types::Int> bucketCountPropertyTyped();
  void setBucketCount(qint64 value);
 Q_SIGNALS:
  void bucketCountChanged(qint64 value);

 public:
  // Q_PROPERTY(qint64 BucketCount READ bucketCount WRITE setBucketCount NOTIFY
  // bucketCountChanged)

  QList<vx::ColorizerEntry> colorMap() override final;
  QList<std::tuple<double, std::tuple<double, double, double, double>, qint32>>
  colorMapRaw() override final;
  static QSharedPointer<NodeProperty> colorMapProperty();
  static NodePropertyTyped<vx::types::ValueColorMapping>
  colorMapPropertyTyped();
  void setColorMap(QList<vx::ColorizerEntry> value);
 Q_SIGNALS:
  void colorMapChanged(QList<vx::ColorizerEntry> value);

 public:
  // Q_PROPERTY(QList<vx::ColorizerEntry> ColorMap READ colorMap WRITE
  // setColorMap NOTIFY colorMapChanged)

  QString column() override final;
  QString columnRaw() override final;
  static QSharedPointer<NodeProperty> columnProperty();
  static NodePropertyTyped<vx::types::String> columnPropertyTyped();
  void setColumn(QString value);
 Q_SIGNALS:
  void columnChanged(QString value);

 public:
  // Q_PROPERTY(QString Column READ column WRITE setColumn NOTIFY columnChanged)

  bool logarithmicX() override final;
  bool logarithmicXRaw() override final;
  static QSharedPointer<NodeProperty> logarithmicXProperty();
  static NodePropertyTyped<vx::types::Boolean> logarithmicXPropertyTyped();
  void setLogarithmicX(bool value);
 Q_SIGNALS:
  void logarithmicXChanged(bool value);

 public:
  // Q_PROPERTY(bool LogarithmicX READ logarithmicX WRITE setLogarithmicX NOTIFY
  // logarithmicXChanged)

  bool logarithmicY() override final;
  bool logarithmicYRaw() override final;
  static QSharedPointer<NodeProperty> logarithmicYProperty();
  static NodePropertyTyped<vx::types::Boolean> logarithmicYPropertyTyped();
  void setLogarithmicY(bool value);
 Q_SIGNALS:
  void logarithmicYChanged(bool value);

 public:
  // Q_PROPERTY(bool LogarithmicY READ logarithmicY WRITE setLogarithmicY NOTIFY
  // logarithmicYChanged)

  double lowerBoundXFraction() override final;
  double lowerBoundXFractionRaw() override final;
  static QSharedPointer<NodeProperty> lowerBoundXFractionProperty();
  static NodePropertyTyped<vx::types::Float> lowerBoundXFractionPropertyTyped();
  void setLowerBoundXFraction(double value);
 Q_SIGNALS:
  void lowerBoundXFractionChanged(double value);

 public:
  // Q_PROPERTY(double LowerBoundXFraction READ lowerBoundXFraction WRITE
  // setLowerBoundXFraction NOTIFY lowerBoundXFractionChanged)

  vx::Node* table() override final;
  QDBusObjectPath tableRaw() override final;
  static QSharedPointer<NodeProperty> tableProperty();
  static NodePropertyTyped<vx::types::NodeReference> tablePropertyTyped();
  void setTable(vx::Node* value);
 Q_SIGNALS:
  void tableChanged(vx::Node* value);

 public:
  // Q_PROPERTY(vx::Node* Table READ table WRITE setTable NOTIFY tableChanged)

  double upperBoundXFraction() override final;
  double upperBoundXFractionRaw() override final;
  static QSharedPointer<NodeProperty> upperBoundXFractionProperty();
  static NodePropertyTyped<vx::types::Float> upperBoundXFractionPropertyTyped();
  void setUpperBoundXFraction(double value);
 Q_SIGNALS:
  void upperBoundXFractionChanged(double value);

 public:
  // Q_PROPERTY(double UpperBoundXFraction READ upperBoundXFraction WRITE
  // setUpperBoundXFraction NOTIFY upperBoundXFractionChanged)
};

}  // namespace visualizer_prop
inline namespace visualizer_prop {
class ScatterPlotPropertiesEntry : public vx::PropertiesEntryBase {
  ScatterPlotPropertiesEntry() = delete;

 public:
  ~ScatterPlotPropertiesEntry();
  ScatterPlotPropertiesEntry(vx::PropType::ColorMap, QList<vx::ColorizerEntry>);
  ScatterPlotPropertiesEntry(vx::PropType::ColumnColor, QString);
  ScatterPlotPropertiesEntry(vx::PropType::ColumnX, QString);
  ScatterPlotPropertiesEntry(vx::PropType::ColumnY, QString);
  ScatterPlotPropertiesEntry(vx::PropType::LogarithmicX, bool);
  ScatterPlotPropertiesEntry(vx::PropType::LogarithmicY, bool);
  ScatterPlotPropertiesEntry(vx::PropType::PointLimit, qint64);
  ScatterPlotPropertiesEntry(vx::PropType::PointScale, double);
  ScatterPlotPropertiesEntry(vx::PropType::Table, vx::Node*);
  ScatterPlotPropertiesEntry(vx::PropType::ViewMargin, double);
  ScatterPlotPropertiesEntry(vx::PropType::ViewPercentile, double);
};
class ScatterPlotPropertiesBase {
 public:
  virtual ~ScatterPlotPropertiesBase();
  virtual QList<vx::ColorizerEntry> colorMap() = 0;
  virtual QList<
      std::tuple<double, std::tuple<double, double, double, double>, qint32>>
  colorMapRaw() = 0;
  virtual QString columnColor() = 0;
  virtual QString columnColorRaw() = 0;
  virtual QString columnX() = 0;
  virtual QString columnXRaw() = 0;
  virtual QString columnY() = 0;
  virtual QString columnYRaw() = 0;
  virtual bool logarithmicX() = 0;
  virtual bool logarithmicXRaw() = 0;
  virtual bool logarithmicY() = 0;
  virtual bool logarithmicYRaw() = 0;
  virtual qint64 pointLimit() = 0;
  virtual qint64 pointLimitRaw() = 0;
  virtual double pointScale() = 0;
  virtual double pointScaleRaw() = 0;
  virtual vx::Node* table() = 0;
  virtual QDBusObjectPath tableRaw() = 0;
  virtual double viewMargin() = 0;
  virtual double viewMarginRaw() = 0;
  virtual double viewPercentile() = 0;
  virtual double viewPercentileRaw() = 0;
};
class ScatterPlotPropertiesCopy : public ScatterPlotPropertiesBase {
  QSharedPointer<const QMap<QString, QVariant>> _properties;

 public:
  ScatterPlotPropertiesCopy(
      const QSharedPointer<const QMap<QString, QVariant>>& properties);
  QList<vx::ColorizerEntry> colorMap() override final;
  QList<std::tuple<double, std::tuple<double, double, double, double>, qint32>>
  colorMapRaw() override final;
  QString columnColor() override final;
  QString columnColorRaw() override final;
  QString columnX() override final;
  QString columnXRaw() override final;
  QString columnY() override final;
  QString columnYRaw() override final;
  bool logarithmicX() override final;
  bool logarithmicXRaw() override final;
  bool logarithmicY() override final;
  bool logarithmicYRaw() override final;
  qint64 pointLimit() override final;
  qint64 pointLimitRaw() override final;
  double pointScale() override final;
  double pointScaleRaw() override final;
  vx::Node* table() override final;
  QDBusObjectPath tableRaw() override final;
  double viewMargin() override final;
  double viewMarginRaw() override final;
  double viewPercentile() override final;
  double viewPercentileRaw() override final;
};
class ScatterPlotProperties : public QObject, public ScatterPlotPropertiesBase {
  Q_OBJECT
  vx::Node* _node;

 public:
  static const char* _getPrototypeJson();
  static QSharedPointer<vx::NodePrototype> getNodePrototype();
  ScatterPlotProperties(vx::Node* parent);
  ~ScatterPlotProperties();

  QList<vx::ColorizerEntry> colorMap() override final;
  QList<std::tuple<double, std::tuple<double, double, double, double>, qint32>>
  colorMapRaw() override final;
  static QSharedPointer<NodeProperty> colorMapProperty();
  static NodePropertyTyped<vx::types::ValueColorMapping>
  colorMapPropertyTyped();
  void setColorMap(QList<vx::ColorizerEntry> value);
 Q_SIGNALS:
  void colorMapChanged(QList<vx::ColorizerEntry> value);

 public:
  // Q_PROPERTY(QList<vx::ColorizerEntry> ColorMap READ colorMap WRITE
  // setColorMap NOTIFY colorMapChanged)

  QString columnColor() override final;
  QString columnColorRaw() override final;
  static QSharedPointer<NodeProperty> columnColorProperty();
  static NodePropertyTyped<vx::types::String> columnColorPropertyTyped();
  void setColumnColor(QString value);
 Q_SIGNALS:
  void columnColorChanged(QString value);

 public:
  // Q_PROPERTY(QString ColumnColor READ columnColor WRITE setColumnColor NOTIFY
  // columnColorChanged)

  QString columnX() override final;
  QString columnXRaw() override final;
  static QSharedPointer<NodeProperty> columnXProperty();
  static NodePropertyTyped<vx::types::String> columnXPropertyTyped();
  void setColumnX(QString value);
 Q_SIGNALS:
  void columnXChanged(QString value);

 public:
  // Q_PROPERTY(QString ColumnX READ columnX WRITE setColumnX NOTIFY
  // columnXChanged)

  QString columnY() override final;
  QString columnYRaw() override final;
  static QSharedPointer<NodeProperty> columnYProperty();
  static NodePropertyTyped<vx::types::String> columnYPropertyTyped();
  void setColumnY(QString value);
 Q_SIGNALS:
  void columnYChanged(QString value);

 public:
  // Q_PROPERTY(QString ColumnY READ columnY WRITE setColumnY NOTIFY
  // columnYChanged)

  bool logarithmicX() override final;
  bool logarithmicXRaw() override final;
  static QSharedPointer<NodeProperty> logarithmicXProperty();
  static NodePropertyTyped<vx::types::Boolean> logarithmicXPropertyTyped();
  void setLogarithmicX(bool value);
 Q_SIGNALS:
  void logarithmicXChanged(bool value);

 public:
  // Q_PROPERTY(bool LogarithmicX READ logarithmicX WRITE setLogarithmicX NOTIFY
  // logarithmicXChanged)

  bool logarithmicY() override final;
  bool logarithmicYRaw() override final;
  static QSharedPointer<NodeProperty> logarithmicYProperty();
  static NodePropertyTyped<vx::types::Boolean> logarithmicYPropertyTyped();
  void setLogarithmicY(bool value);
 Q_SIGNALS:
  void logarithmicYChanged(bool value);

 public:
  // Q_PROPERTY(bool LogarithmicY READ logarithmicY WRITE setLogarithmicY NOTIFY
  // logarithmicYChanged)

  qint64 pointLimit() override final;
  qint64 pointLimitRaw() override final;
  static QSharedPointer<NodeProperty> pointLimitProperty();
  static NodePropertyTyped<vx::types::Int> pointLimitPropertyTyped();
  void setPointLimit(qint64 value);
 Q_SIGNALS:
  void pointLimitChanged(qint64 value);

 public:
  // Q_PROPERTY(qint64 PointLimit READ pointLimit WRITE setPointLimit NOTIFY
  // pointLimitChanged)

  double pointScale() override final;
  double pointScaleRaw() override final;
  static QSharedPointer<NodeProperty> pointScaleProperty();
  static NodePropertyTyped<vx::types::Float> pointScalePropertyTyped();
  void setPointScale(double value);
 Q_SIGNALS:
  void pointScaleChanged(double value);

 public:
  // Q_PROPERTY(double PointScale READ pointScale WRITE setPointScale NOTIFY
  // pointScaleChanged)

  vx::Node* table() override final;
  QDBusObjectPath tableRaw() override final;
  static QSharedPointer<NodeProperty> tableProperty();
  static NodePropertyTyped<vx::types::NodeReference> tablePropertyTyped();
  void setTable(vx::Node* value);
 Q_SIGNALS:
  void tableChanged(vx::Node* value);

 public:
  // Q_PROPERTY(vx::Node* Table READ table WRITE setTable NOTIFY tableChanged)

  double viewMargin() override final;
  double viewMarginRaw() override final;
  static QSharedPointer<NodeProperty> viewMarginProperty();
  static NodePropertyTyped<vx::types::Float> viewMarginPropertyTyped();
  void setViewMargin(double value);
 Q_SIGNALS:
  void viewMarginChanged(double value);

 public:
  // Q_PROPERTY(double ViewMargin READ viewMargin WRITE setViewMargin NOTIFY
  // viewMarginChanged)

  double viewPercentile() override final;
  double viewPercentileRaw() override final;
  static QSharedPointer<NodeProperty> viewPercentileProperty();
  static NodePropertyTyped<vx::types::Float> viewPercentilePropertyTyped();
  void setViewPercentile(double value);
 Q_SIGNALS:
  void viewPercentileChanged(double value);

 public:
  // Q_PROPERTY(double ViewPercentile READ viewPercentile WRITE
  // setViewPercentile NOTIFY viewPercentileChanged)
};

}  // namespace visualizer_prop
inline namespace visualizer_prop {
class TablePropertiesEntry : public vx::PropertiesEntryBase {
  TablePropertiesEntry() = delete;

 public:
  ~TablePropertiesEntry();
  TablePropertiesEntry(vx::PropType::RowLimit, qint64);
  TablePropertiesEntry(vx::PropType::SortColumn, QString);
  TablePropertiesEntry(vx::PropType::SortOrder, QString);
  TablePropertiesEntry(vx::PropType::Table, vx::Node*);
};
class TablePropertiesBase {
 public:
  virtual ~TablePropertiesBase();
  virtual qint64 rowLimit() = 0;
  virtual qint64 rowLimitRaw() = 0;
  virtual QString sortColumn() = 0;
  virtual QString sortColumnRaw() = 0;
  virtual QString sortOrder() = 0;
  virtual QString sortOrderRaw() = 0;
  virtual vx::Node* table() = 0;
  virtual QDBusObjectPath tableRaw() = 0;
};
class TablePropertiesCopy : public TablePropertiesBase {
  QSharedPointer<const QMap<QString, QVariant>> _properties;

 public:
  TablePropertiesCopy(
      const QSharedPointer<const QMap<QString, QVariant>>& properties);
  qint64 rowLimit() override final;
  qint64 rowLimitRaw() override final;
  QString sortColumn() override final;
  QString sortColumnRaw() override final;
  QString sortOrder() override final;
  QString sortOrderRaw() override final;
  vx::Node* table() override final;
  QDBusObjectPath tableRaw() override final;
};
class TableProperties : public QObject, public TablePropertiesBase {
  Q_OBJECT
  vx::Node* _node;

 public:
  static const char* _getPrototypeJson();
  static QSharedPointer<vx::NodePrototype> getNodePrototype();
  TableProperties(vx::Node* parent);
  ~TableProperties();

  qint64 rowLimit() override final;
  qint64 rowLimitRaw() override final;
  static QSharedPointer<NodeProperty> rowLimitProperty();
  static NodePropertyTyped<vx::types::Int> rowLimitPropertyTyped();
  void setRowLimit(qint64 value);
 Q_SIGNALS:
  void rowLimitChanged(qint64 value);

 public:
  // Q_PROPERTY(qint64 RowLimit READ rowLimit WRITE setRowLimit NOTIFY
  // rowLimitChanged)

  QString sortColumn() override final;
  QString sortColumnRaw() override final;
  static QSharedPointer<NodeProperty> sortColumnProperty();
  static NodePropertyTyped<vx::types::String> sortColumnPropertyTyped();
  void setSortColumn(QString value);
 Q_SIGNALS:
  void sortColumnChanged(QString value);

 public:
  // Q_PROPERTY(QString SortColumn READ sortColumn WRITE setSortColumn NOTIFY
  // sortColumnChanged)

  QString sortOrder() override final;
  QString sortOrderRaw() override final;
  static QSharedPointer<NodeProperty> sortOrderProperty();
  static NodePropertyTyped<vx::types::Enumeration> sortOrderPropertyTyped();
  void setSortOrder(QString value);
 Q_SIGNALS:
  void sortOrderChanged(QString value);

 public:
  // Q_PROPERTY(QString SortOrder READ sortOrder WRITE setSortOrder NOTIFY
  // sortOrderChanged)

  vx::Node* table() override final;
  QDBusObjectPath tableRaw() override final;
  static QSharedPointer<NodeProperty> tableProperty();
  static NodePropertyTyped<vx::types::NodeReference> tablePropertyTyped();
  void setTable(vx::Node* value);
 Q_SIGNALS:
  void tableChanged(vx::Node* value);

 public:
  // Q_PROPERTY(vx::Node* Table READ table WRITE setTable NOTIFY tableChanged)
};

}  // namespace visualizer_prop
}  // namespace vx
