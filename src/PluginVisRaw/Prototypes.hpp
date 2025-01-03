// This file was automatically generated by tools/update-node-prototypes.py
// All changes to this file will be lost

#pragma once

#include "Prototypes.forward.hpp"

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

class NodeNodeProperty;  // In Voxie/Node/NodeNodeProperty.hpp

namespace vx {
#ifndef VOXIE_PROP_DEFINED_CenterPoint
#define VOXIE_PROP_DEFINED_CenterPoint
namespace PropType {
class CenterPoint : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::CenterPoint CenterPoint = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_CurrentImage
#define VOXIE_PROP_DEFINED_CurrentImage
namespace PropType {
class CurrentImage : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::CurrentImage CurrentImage = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_CurrentImageList
#define VOXIE_PROP_DEFINED_CurrentImageList
namespace PropType {
class CurrentImageList : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::CurrentImageList CurrentImageList = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_ImageKind
#define VOXIE_PROP_DEFINED_ImageKind
namespace PropType {
class ImageKind : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::ImageKind ImageKind = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_ImagesPerSecond
#define VOXIE_PROP_DEFINED_ImagesPerSecond
namespace PropType {
class ImagesPerSecond : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::ImagesPerSecond ImagesPerSecond = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_Interpolation
#define VOXIE_PROP_DEFINED_Interpolation
namespace PropType {
class Interpolation : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::Interpolation Interpolation = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_RawData
#define VOXIE_PROP_DEFINED_RawData
namespace PropType {
class RawData : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::RawData RawData = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_ValueColorMapping
#define VOXIE_PROP_DEFINED_ValueColorMapping
namespace PropType {
class ValueColorMapping : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::ValueColorMapping ValueColorMapping = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_VerticalSize
#define VOXIE_PROP_DEFINED_VerticalSize
namespace PropType {
class VerticalSize : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::VerticalSize VerticalSize = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_WaitForImages
#define VOXIE_PROP_DEFINED_WaitForImages
namespace PropType {
class WaitForImages : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::WaitForImages WaitForImages = {};
}
#endif
inline namespace visualizer_prop {
class TomographyRawDataPropertiesEntry : public vx::PropertiesEntryBase {
  TomographyRawDataPropertiesEntry() = delete;

 public:
  ~TomographyRawDataPropertiesEntry();
  TomographyRawDataPropertiesEntry(vx::PropType::CenterPoint, QPointF);
  TomographyRawDataPropertiesEntry(vx::PropType::VerticalSize, double);
  TomographyRawDataPropertiesEntry(vx::PropType::CurrentImage, qint64);
  TomographyRawDataPropertiesEntry(vx::PropType::CurrentImageList,
                                   std::tuple<QString, QJsonObject>);
  TomographyRawDataPropertiesEntry(vx::PropType::ImageKind, QJsonObject);
  TomographyRawDataPropertiesEntry(vx::PropType::ImagesPerSecond, double);
  TomographyRawDataPropertiesEntry(vx::PropType::Interpolation, QString);
  TomographyRawDataPropertiesEntry(vx::PropType::RawData, vx::Node*);
  TomographyRawDataPropertiesEntry(vx::PropType::ValueColorMapping,
                                   QList<vx::ColorizerEntry>);
  TomographyRawDataPropertiesEntry(vx::PropType::WaitForImages, bool);
};
class TomographyRawDataPropertiesBase {
 public:
  virtual ~TomographyRawDataPropertiesBase();
  virtual QPointF centerPoint() = 0;
  virtual std::tuple<double, double> centerPointRaw() = 0;
  virtual double verticalSize() = 0;
  virtual double verticalSizeRaw() = 0;
  virtual qint64 currentImage() = 0;
  virtual qint64 currentImageRaw() = 0;
  virtual std::tuple<QString, QJsonObject> currentImageList() = 0;
  virtual std::tuple<QString, QJsonObject> currentImageListRaw() = 0;
  virtual QJsonObject imageKind() = 0;
  virtual QJsonObject imageKindRaw() = 0;
  virtual double imagesPerSecond() = 0;
  virtual double imagesPerSecondRaw() = 0;
  virtual QString interpolation() = 0;
  virtual QString interpolationRaw() = 0;
  virtual QDBusObjectPath rawDataRaw() = 0;
  virtual QList<vx::ColorizerEntry> valueColorMapping() = 0;
  virtual QList<
      std::tuple<double, std::tuple<double, double, double, double>, qint32>>
  valueColorMappingRaw() = 0;
  virtual bool waitForImages() = 0;
  virtual bool waitForImagesRaw() = 0;
};
class TomographyRawDataPropertiesCopy : public TomographyRawDataPropertiesBase {
  QSharedPointer<const QMap<QString, QVariant>> _properties;

 public:
  TomographyRawDataPropertiesCopy(
      const QSharedPointer<const QMap<QString, QVariant>>& properties);
  QPointF centerPoint() override final;
  std::tuple<double, double> centerPointRaw() override final;
  double verticalSize() override final;
  double verticalSizeRaw() override final;
  qint64 currentImage() override final;
  qint64 currentImageRaw() override final;
  std::tuple<QString, QJsonObject> currentImageList() override final;
  std::tuple<QString, QJsonObject> currentImageListRaw() override final;
  QJsonObject imageKind() override final;
  QJsonObject imageKindRaw() override final;
  double imagesPerSecond() override final;
  double imagesPerSecondRaw() override final;
  QString interpolation() override final;
  QString interpolationRaw() override final;
  QDBusObjectPath rawDataRaw() override final;
  QList<vx::ColorizerEntry> valueColorMapping() override final;
  QList<std::tuple<double, std::tuple<double, double, double, double>, qint32>>
  valueColorMappingRaw() override final;
  bool waitForImages() override final;
  bool waitForImagesRaw() override final;
};
class TomographyRawDataProperties : public QObject,
                                    public TomographyRawDataPropertiesBase {
  Q_OBJECT
  vx::Node* _node;

 public:
  static const char* _getPrototypeJson();
  static QSharedPointer<vx::NodePrototype> getNodePrototype();
  TomographyRawDataProperties(vx::Node* parent);
  ~TomographyRawDataProperties();

  QPointF centerPoint() override final;
  std::tuple<double, double> centerPointRaw() override final;
  static QSharedPointer<NodeProperty> centerPointProperty();
  static NodePropertyTyped<vx::types::Point2D> centerPointPropertyTyped();
  NodeNodeProperty centerPointInstance();
  void setCenterPoint(QPointF value);
 Q_SIGNALS:
  void centerPointChanged(QPointF value);

 public:
  // Q_PROPERTY(QPointF CenterPoint READ centerPoint WRITE setCenterPoint NOTIFY
  // centerPointChanged)

  double verticalSize() override final;
  double verticalSizeRaw() override final;
  static QSharedPointer<NodeProperty> verticalSizeProperty();
  static NodePropertyTyped<vx::types::Float> verticalSizePropertyTyped();
  NodeNodeProperty verticalSizeInstance();
  void setVerticalSize(double value);
 Q_SIGNALS:
  void verticalSizeChanged(double value);

 public:
  // Q_PROPERTY(double VerticalSize READ verticalSize WRITE setVerticalSize
  // NOTIFY verticalSizeChanged)

  qint64 currentImage() override final;
  qint64 currentImageRaw() override final;
  static QSharedPointer<NodeProperty> currentImageProperty();
  static NodePropertyTyped<vx::types::Int> currentImagePropertyTyped();
  NodeNodeProperty currentImageInstance();
  void setCurrentImage(qint64 value);
 Q_SIGNALS:
  void currentImageChanged(qint64 value);

 public:
  // Q_PROPERTY(qint64 CurrentImage READ currentImage WRITE setCurrentImage
  // NOTIFY currentImageChanged)

  std::tuple<QString, QJsonObject> currentImageList() override final;
  std::tuple<QString, QJsonObject> currentImageListRaw() override final;
  static QSharedPointer<NodeProperty> currentImageListProperty();
  static NodePropertyTyped<vx::types::TomographyRawDataImageList>
  currentImageListPropertyTyped();
  NodeNodeProperty currentImageListInstance();
  void setCurrentImageList(std::tuple<QString, QJsonObject> value);
 Q_SIGNALS:
  void currentImageListChanged(std::tuple<QString, QJsonObject> value);

 public:
  // Q_PROPERTY(std::tuple<QString, QJsonObject> CurrentImageList READ
  // currentImageList WRITE setCurrentImageList NOTIFY currentImageListChanged)

  QJsonObject imageKind() override final;
  QJsonObject imageKindRaw() override final;
  static QSharedPointer<NodeProperty> imageKindProperty();
  static NodePropertyTyped<vx::types::TomographyRawDataImageKind>
  imageKindPropertyTyped();
  NodeNodeProperty imageKindInstance();
  void setImageKind(QJsonObject value);
 Q_SIGNALS:
  void imageKindChanged(QJsonObject value);

 public:
  // Q_PROPERTY(QJsonObject ImageKind READ imageKind WRITE setImageKind NOTIFY
  // imageKindChanged)

  double imagesPerSecond() override final;
  double imagesPerSecondRaw() override final;
  static QSharedPointer<NodeProperty> imagesPerSecondProperty();
  static NodePropertyTyped<vx::types::Float> imagesPerSecondPropertyTyped();
  NodeNodeProperty imagesPerSecondInstance();
  void setImagesPerSecond(double value);
 Q_SIGNALS:
  void imagesPerSecondChanged(double value);

 public:
  // Q_PROPERTY(double ImagesPerSecond READ imagesPerSecond WRITE
  // setImagesPerSecond NOTIFY imagesPerSecondChanged)

  QString interpolation() override final;
  QString interpolationRaw() override final;
  static QSharedPointer<NodeProperty> interpolationProperty();
  static NodePropertyTyped<vx::types::Enumeration> interpolationPropertyTyped();
  NodeNodeProperty interpolationInstance();
  void setInterpolation(QString value);
 Q_SIGNALS:
  void interpolationChanged(QString value);

 public:
  // Q_PROPERTY(QString Interpolation READ interpolation WRITE setInterpolation
  // NOTIFY interpolationChanged)

  vx::Node* rawData();
  QDBusObjectPath rawDataRaw() override final;
  static QSharedPointer<NodeProperty> rawDataProperty();
  static NodePropertyTyped<vx::types::NodeReference> rawDataPropertyTyped();
  NodeNodeProperty rawDataInstance();
  void setRawData(vx::Node* value);
 Q_SIGNALS:
  void rawDataChanged(vx::Node* value);

 public:
  // Q_PROPERTY(vx::Node* RawData READ rawData WRITE setRawData NOTIFY
  // rawDataChanged)

  QList<vx::ColorizerEntry> valueColorMapping() override final;
  QList<std::tuple<double, std::tuple<double, double, double, double>, qint32>>
  valueColorMappingRaw() override final;
  static QSharedPointer<NodeProperty> valueColorMappingProperty();
  static NodePropertyTyped<vx::types::ValueColorMapping>
  valueColorMappingPropertyTyped();
  NodeNodeProperty valueColorMappingInstance();
  void setValueColorMapping(QList<vx::ColorizerEntry> value);
 Q_SIGNALS:
  void valueColorMappingChanged(QList<vx::ColorizerEntry> value);

 public:
  // Q_PROPERTY(QList<vx::ColorizerEntry> ValueColorMapping READ
  // valueColorMapping WRITE setValueColorMapping NOTIFY
  // valueColorMappingChanged)

  bool waitForImages() override final;
  bool waitForImagesRaw() override final;
  static QSharedPointer<NodeProperty> waitForImagesProperty();
  static NodePropertyTyped<vx::types::Boolean> waitForImagesPropertyTyped();
  NodeNodeProperty waitForImagesInstance();
  void setWaitForImages(bool value);
 Q_SIGNALS:
  void waitForImagesChanged(bool value);

 public:
  // Q_PROPERTY(bool WaitForImages READ waitForImages WRITE setWaitForImages
  // NOTIFY waitForImagesChanged)
};

}  // namespace visualizer_prop
}  // namespace vx
