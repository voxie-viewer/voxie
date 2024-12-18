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
#ifndef VOXIE_PROP_DEFINED_BackColor
#define VOXIE_PROP_DEFINED_BackColor
namespace PropType {
class BackColor : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::BackColor BackColor = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_ClippingDirection
#define VOXIE_PROP_DEFINED_ClippingDirection
namespace PropType {
class ClippingDirection : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::ClippingDirection ClippingDirection = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_Color
#define VOXIE_PROP_DEFINED_Color
namespace PropType {
class Color : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::Color Color = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_DefaultSize
#define VOXIE_PROP_DEFINED_DefaultSize
namespace PropType {
class DefaultSize : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::DefaultSize DefaultSize = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_DrawAxisArrows
#define VOXIE_PROP_DEFINED_DrawAxisArrows
namespace PropType {
class DrawAxisArrows : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::DrawAxisArrows DrawAxisArrows = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_DrawBoundingBox
#define VOXIE_PROP_DEFINED_DrawBoundingBox
namespace PropType {
class DrawBoundingBox : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::DrawBoundingBox DrawBoundingBox = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_DrawOrigin
#define VOXIE_PROP_DEFINED_DrawOrigin
namespace PropType {
class DrawOrigin : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::DrawOrigin DrawOrigin = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_FaceCulling
#define VOXIE_PROP_DEFINED_FaceCulling
namespace PropType {
class FaceCulling : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::FaceCulling FaceCulling = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_FieldOfView
#define VOXIE_PROP_DEFINED_FieldOfView
namespace PropType {
class FieldOfView : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::FieldOfView FieldOfView = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_FrontColor
#define VOXIE_PROP_DEFINED_FrontColor
namespace PropType {
class FrontColor : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::FrontColor FrontColor = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_GeometricPrimitive
#define VOXIE_PROP_DEFINED_GeometricPrimitive
namespace PropType {
class GeometricPrimitive : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::GeometricPrimitive GeometricPrimitive = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_HighlightCurrentTriangle
#define VOXIE_PROP_DEFINED_HighlightCurrentTriangle
namespace PropType {
class HighlightCurrentTriangle : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::HighlightCurrentTriangle HighlightCurrentTriangle = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_Input
#define VOXIE_PROP_DEFINED_Input
namespace PropType {
class Input : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::Input Input = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_Length
#define VOXIE_PROP_DEFINED_Length
namespace PropType {
class Length : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::Length Length = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_LookAt
#define VOXIE_PROP_DEFINED_LookAt
namespace PropType {
class LookAt : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::LookAt LookAt = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_Objects
#define VOXIE_PROP_DEFINED_Objects
namespace PropType {
class Objects : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::Objects Objects = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_Orientation
#define VOXIE_PROP_DEFINED_Orientation
namespace PropType {
class Orientation : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::Orientation Orientation = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_Plane
#define VOXIE_PROP_DEFINED_Plane
namespace PropType {
class Plane : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::Plane Plane = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_ShadingTechnique
#define VOXIE_PROP_DEFINED_ShadingTechnique
namespace PropType {
class ShadingTechnique : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::ShadingTechnique ShadingTechnique = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_ShowViewCenter
#define VOXIE_PROP_DEFINED_ShowViewCenter
namespace PropType {
class ShowViewCenter : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::ShowViewCenter ShowViewCenter = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_ShowVolumeSlice
#define VOXIE_PROP_DEFINED_ShowVolumeSlice
namespace PropType {
class ShowVolumeSlice : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::ShowVolumeSlice ShowVolumeSlice = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_SliceTextureResolution
#define VOXIE_PROP_DEFINED_SliceTextureResolution
namespace PropType {
class SliceTextureResolution : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::SliceTextureResolution SliceTextureResolution = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_SliceValueColorMapping
#define VOXIE_PROP_DEFINED_SliceValueColorMapping
namespace PropType {
class SliceValueColorMapping : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::SliceValueColorMapping SliceValueColorMapping = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_SliceVolume
#define VOXIE_PROP_DEFINED_SliceVolume
namespace PropType {
class SliceVolume : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::SliceVolume SliceVolume = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_Surface
#define VOXIE_PROP_DEFINED_Surface
namespace PropType {
class Surface : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::Surface Surface = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_ViewSizeUnzoomed
#define VOXIE_PROP_DEFINED_ViewSizeUnzoomed
namespace PropType {
class ViewSizeUnzoomed : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::ViewSizeUnzoomed ViewSizeUnzoomed = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_Visible
#define VOXIE_PROP_DEFINED_Visible
namespace PropType {
class Visible : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::Visible Visible = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_ZoomLog
#define VOXIE_PROP_DEFINED_ZoomLog
namespace PropType {
class ZoomLog : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::ZoomLog ZoomLog = {};
}
#endif
inline namespace object3d_prop {
class Test3DObjectPropertiesEntry : public vx::PropertiesEntryBase {
  Test3DObjectPropertiesEntry() = delete;

 public:
  ~Test3DObjectPropertiesEntry();
  Test3DObjectPropertiesEntry(vx::PropType::Length, double);
};
class Test3DObjectPropertiesBase {
 public:
  virtual ~Test3DObjectPropertiesBase();
  virtual double length() = 0;
  virtual double lengthRaw() = 0;
};
class Test3DObjectPropertiesCopy : public Test3DObjectPropertiesBase {
  QSharedPointer<const QMap<QString, QVariant>> _properties;

 public:
  Test3DObjectPropertiesCopy(
      const QSharedPointer<const QMap<QString, QVariant>>& properties);
  double length() override final;
  double lengthRaw() override final;
};
class Test3DObjectProperties : public QObject,
                               public Test3DObjectPropertiesBase {
  Q_OBJECT
  vx::Node* _node;

 public:
  static const char* _getPrototypeJson();
  static QSharedPointer<vx::NodePrototype> getNodePrototype();
  Test3DObjectProperties(vx::Node* parent);
  ~Test3DObjectProperties();

  double length() override final;
  double lengthRaw() override final;
  static QSharedPointer<NodeProperty> lengthProperty();
  static NodePropertyTyped<vx::types::Float> lengthPropertyTyped();
  NodeNodeProperty lengthInstance();
  void setLength(double value);
 Q_SIGNALS:
  void lengthChanged(double value);

 public:
  // Q_PROPERTY(double Length READ length WRITE setLength NOTIFY lengthChanged)
};

}  // namespace object3d_prop
inline namespace object3d_prop {
class GeometricPrimitivePropertiesEntry : public vx::PropertiesEntryBase {
  GeometricPrimitivePropertiesEntry() = delete;

 public:
  ~GeometricPrimitivePropertiesEntry();
  GeometricPrimitivePropertiesEntry(vx::PropType::Visible, bool);
  GeometricPrimitivePropertiesEntry(vx::PropType::GeometricPrimitive,
                                    vx::Node*);
};
class GeometricPrimitivePropertiesBase {
 public:
  virtual ~GeometricPrimitivePropertiesBase();
  virtual bool visible() = 0;
  virtual bool visibleRaw() = 0;
  virtual QDBusObjectPath geometricPrimitiveRaw() = 0;
};
class GeometricPrimitivePropertiesCopy
    : public GeometricPrimitivePropertiesBase {
  QSharedPointer<const QMap<QString, QVariant>> _properties;

 public:
  GeometricPrimitivePropertiesCopy(
      const QSharedPointer<const QMap<QString, QVariant>>& properties);
  bool visible() override final;
  bool visibleRaw() override final;
  QDBusObjectPath geometricPrimitiveRaw() override final;
};
class GeometricPrimitiveProperties : public QObject,
                                     public GeometricPrimitivePropertiesBase {
  Q_OBJECT
  vx::Node* _node;

 public:
  static const char* _getPrototypeJson();
  static QSharedPointer<vx::NodePrototype> getNodePrototype();
  GeometricPrimitiveProperties(vx::Node* parent);
  ~GeometricPrimitiveProperties();

  bool visible() override final;
  bool visibleRaw() override final;
  static QSharedPointer<NodeProperty> visibleProperty();
  static NodePropertyTyped<vx::types::Boolean> visiblePropertyTyped();
  NodeNodeProperty visibleInstance();
  void setVisible(bool value);
 Q_SIGNALS:
  void visibleChanged(bool value);

 public:
  // Q_PROPERTY(bool Visible READ visible WRITE setVisible NOTIFY
  // visibleChanged)

  vx::Node* geometricPrimitive();
  QDBusObjectPath geometricPrimitiveRaw() override final;
  static QSharedPointer<NodeProperty> geometricPrimitiveProperty();
  static NodePropertyTyped<vx::types::NodeReference>
  geometricPrimitivePropertyTyped();
  NodeNodeProperty geometricPrimitiveInstance();
  void setGeometricPrimitive(vx::Node* value);
 Q_SIGNALS:
  void geometricPrimitiveChanged(vx::Node* value);

 public:
  // Q_PROPERTY(vx::Node* GeometricPrimitive READ geometricPrimitive WRITE
  // setGeometricPrimitive NOTIFY geometricPrimitiveChanged)
};

}  // namespace object3d_prop
inline namespace object3d_prop {
class GridPropertiesEntry : public vx::PropertiesEntryBase {
  GridPropertiesEntry() = delete;

 public:
  ~GridPropertiesEntry();
};
class GridPropertiesBase {
 public:
  virtual ~GridPropertiesBase();
};
class GridPropertiesCopy : public GridPropertiesBase {
  QSharedPointer<const QMap<QString, QVariant>> _properties;

 public:
  GridPropertiesCopy(
      const QSharedPointer<const QMap<QString, QVariant>>& properties);
};
class GridProperties : public QObject, public GridPropertiesBase {
  Q_OBJECT
  vx::Node* _node;

 public:
  static const char* _getPrototypeJson();
  static QSharedPointer<vx::NodePrototype> getNodePrototype();
  GridProperties(vx::Node* parent);
  ~GridProperties();
};

}  // namespace object3d_prop
inline namespace object3d_prop {
class PlanePropertiesEntry : public vx::PropertiesEntryBase {
  PlanePropertiesEntry() = delete;

 public:
  ~PlanePropertiesEntry();
  PlanePropertiesEntry(vx::PropType::ClippingDirection, QString);
  PlanePropertiesEntry(vx::PropType::Color, vx::Color);
  PlanePropertiesEntry(vx::PropType::DefaultSize, double);
  PlanePropertiesEntry(vx::PropType::Plane, vx::Node*);
  PlanePropertiesEntry(vx::PropType::ShowVolumeSlice, bool);
  PlanePropertiesEntry(vx::PropType::SliceTextureResolution, QString);
  PlanePropertiesEntry(vx::PropType::SliceValueColorMapping,
                       QList<vx::ColorizerEntry>);
  PlanePropertiesEntry(vx::PropType::SliceVolume, vx::Node*);
};
class PlanePropertiesBase {
 public:
  virtual ~PlanePropertiesBase();
  virtual QString clippingDirection() = 0;
  virtual QString clippingDirectionRaw() = 0;
  virtual vx::Color color() = 0;
  virtual std::tuple<double, double, double, double> colorRaw() = 0;
  virtual double defaultSize() = 0;
  virtual double defaultSizeRaw() = 0;
  virtual QDBusObjectPath planeRaw() = 0;
  virtual bool showVolumeSlice() = 0;
  virtual bool showVolumeSliceRaw() = 0;
  virtual QString sliceTextureResolution() = 0;
  virtual QString sliceTextureResolutionRaw() = 0;
  virtual QList<vx::ColorizerEntry> sliceValueColorMapping() = 0;
  virtual QList<
      std::tuple<double, std::tuple<double, double, double, double>, qint32>>
  sliceValueColorMappingRaw() = 0;
  virtual QDBusObjectPath sliceVolumeRaw() = 0;
};
class PlanePropertiesCopy : public PlanePropertiesBase {
  QSharedPointer<const QMap<QString, QVariant>> _properties;

 public:
  PlanePropertiesCopy(
      const QSharedPointer<const QMap<QString, QVariant>>& properties);
  QString clippingDirection() override final;
  QString clippingDirectionRaw() override final;
  vx::Color color() override final;
  std::tuple<double, double, double, double> colorRaw() override final;
  double defaultSize() override final;
  double defaultSizeRaw() override final;
  QDBusObjectPath planeRaw() override final;
  bool showVolumeSlice() override final;
  bool showVolumeSliceRaw() override final;
  QString sliceTextureResolution() override final;
  QString sliceTextureResolutionRaw() override final;
  QList<vx::ColorizerEntry> sliceValueColorMapping() override final;
  QList<std::tuple<double, std::tuple<double, double, double, double>, qint32>>
  sliceValueColorMappingRaw() override final;
  QDBusObjectPath sliceVolumeRaw() override final;
};
class PlaneProperties : public QObject, public PlanePropertiesBase {
  Q_OBJECT
  vx::Node* _node;

 public:
  static const char* _getPrototypeJson();
  static QSharedPointer<vx::NodePrototype> getNodePrototype();
  PlaneProperties(vx::Node* parent);
  ~PlaneProperties();

  QString clippingDirection() override final;
  QString clippingDirectionRaw() override final;
  static QSharedPointer<NodeProperty> clippingDirectionProperty();
  static NodePropertyTyped<vx::types::Enumeration>
  clippingDirectionPropertyTyped();
  NodeNodeProperty clippingDirectionInstance();
  void setClippingDirection(QString value);
 Q_SIGNALS:
  void clippingDirectionChanged(QString value);

 public:
  // Q_PROPERTY(QString ClippingDirection READ clippingDirection WRITE
  // setClippingDirection NOTIFY clippingDirectionChanged)

  vx::Color color() override final;
  std::tuple<double, double, double, double> colorRaw() override final;
  static QSharedPointer<NodeProperty> colorProperty();
  static NodePropertyTyped<vx::types::Color> colorPropertyTyped();
  NodeNodeProperty colorInstance();
  void setColor(vx::Color value);
 Q_SIGNALS:
  void colorChanged(vx::Color value);

 public:
  // Q_PROPERTY(vx::Color Color READ color WRITE setColor NOTIFY colorChanged)

  double defaultSize() override final;
  double defaultSizeRaw() override final;
  static QSharedPointer<NodeProperty> defaultSizeProperty();
  static NodePropertyTyped<vx::types::Float> defaultSizePropertyTyped();
  NodeNodeProperty defaultSizeInstance();
  void setDefaultSize(double value);
 Q_SIGNALS:
  void defaultSizeChanged(double value);

 public:
  // Q_PROPERTY(double DefaultSize READ defaultSize WRITE setDefaultSize NOTIFY
  // defaultSizeChanged)

  vx::Node* plane();
  QDBusObjectPath planeRaw() override final;
  static QSharedPointer<NodeProperty> planeProperty();
  static NodePropertyTyped<vx::types::NodeReference> planePropertyTyped();
  NodeNodeProperty planeInstance();
  void setPlane(vx::Node* value);
 Q_SIGNALS:
  void planeChanged(vx::Node* value);

 public:
  // Q_PROPERTY(vx::Node* Plane READ plane WRITE setPlane NOTIFY planeChanged)

  bool showVolumeSlice() override final;
  bool showVolumeSliceRaw() override final;
  static QSharedPointer<NodeProperty> showVolumeSliceProperty();
  static NodePropertyTyped<vx::types::Boolean> showVolumeSlicePropertyTyped();
  NodeNodeProperty showVolumeSliceInstance();
  void setShowVolumeSlice(bool value);
 Q_SIGNALS:
  void showVolumeSliceChanged(bool value);

 public:
  // Q_PROPERTY(bool ShowVolumeSlice READ showVolumeSlice WRITE
  // setShowVolumeSlice NOTIFY showVolumeSliceChanged)

  QString sliceTextureResolution() override final;
  QString sliceTextureResolutionRaw() override final;
  static QSharedPointer<NodeProperty> sliceTextureResolutionProperty();
  static NodePropertyTyped<vx::types::Enumeration>
  sliceTextureResolutionPropertyTyped();
  NodeNodeProperty sliceTextureResolutionInstance();
  void setSliceTextureResolution(QString value);
 Q_SIGNALS:
  void sliceTextureResolutionChanged(QString value);

 public:
  // Q_PROPERTY(QString SliceTextureResolution READ sliceTextureResolution WRITE
  // setSliceTextureResolution NOTIFY sliceTextureResolutionChanged)

  QList<vx::ColorizerEntry> sliceValueColorMapping() override final;
  QList<std::tuple<double, std::tuple<double, double, double, double>, qint32>>
  sliceValueColorMappingRaw() override final;
  static QSharedPointer<NodeProperty> sliceValueColorMappingProperty();
  static NodePropertyTyped<vx::types::ValueColorMapping>
  sliceValueColorMappingPropertyTyped();
  NodeNodeProperty sliceValueColorMappingInstance();
  void setSliceValueColorMapping(QList<vx::ColorizerEntry> value);
 Q_SIGNALS:
  void sliceValueColorMappingChanged(QList<vx::ColorizerEntry> value);

 public:
  // Q_PROPERTY(QList<vx::ColorizerEntry> SliceValueColorMapping READ
  // sliceValueColorMapping WRITE setSliceValueColorMapping NOTIFY
  // sliceValueColorMappingChanged)

  vx::Node* sliceVolume();
  QDBusObjectPath sliceVolumeRaw() override final;
  static QSharedPointer<NodeProperty> sliceVolumeProperty();
  static NodePropertyTyped<vx::types::NodeReference> sliceVolumePropertyTyped();
  NodeNodeProperty sliceVolumeInstance();
  void setSliceVolume(vx::Node* value);
 Q_SIGNALS:
  void sliceVolumeChanged(vx::Node* value);

 public:
  // Q_PROPERTY(vx::Node* SliceVolume READ sliceVolume WRITE setSliceVolume
  // NOTIFY sliceVolumeChanged)
};

}  // namespace object3d_prop
inline namespace object3d_prop {
class SurfacePropertiesEntry : public vx::PropertiesEntryBase {
  SurfacePropertiesEntry() = delete;

 public:
  ~SurfacePropertiesEntry();
  SurfacePropertiesEntry(vx::PropType::Visible, bool);
  SurfacePropertiesEntry(vx::PropType::BackColor, vx::Color);
  SurfacePropertiesEntry(vx::PropType::DrawAxisArrows, bool);
  SurfacePropertiesEntry(vx::PropType::DrawBoundingBox, bool);
  SurfacePropertiesEntry(vx::PropType::DrawOrigin, bool);
  SurfacePropertiesEntry(vx::PropType::FaceCulling, QString);
  SurfacePropertiesEntry(vx::PropType::FrontColor, vx::Color);
  SurfacePropertiesEntry(vx::PropType::HighlightCurrentTriangle, bool);
  SurfacePropertiesEntry(vx::PropType::ShadingTechnique, QString);
  SurfacePropertiesEntry(vx::PropType::Surface, vx::Node*);
};
class SurfacePropertiesBase {
 public:
  virtual ~SurfacePropertiesBase();
  virtual bool visible() = 0;
  virtual bool visibleRaw() = 0;
  virtual vx::Color backColor() = 0;
  virtual std::tuple<double, double, double, double> backColorRaw() = 0;
  virtual bool drawAxisArrows() = 0;
  virtual bool drawAxisArrowsRaw() = 0;
  virtual bool drawBoundingBox() = 0;
  virtual bool drawBoundingBoxRaw() = 0;
  virtual bool drawOrigin() = 0;
  virtual bool drawOriginRaw() = 0;
  virtual QString faceCulling() = 0;
  virtual QString faceCullingRaw() = 0;
  virtual vx::Color frontColor() = 0;
  virtual std::tuple<double, double, double, double> frontColorRaw() = 0;
  virtual bool highlightCurrentTriangle() = 0;
  virtual bool highlightCurrentTriangleRaw() = 0;
  virtual QString shadingTechnique() = 0;
  virtual QString shadingTechniqueRaw() = 0;
  virtual QDBusObjectPath surfaceRaw() = 0;
};
class SurfacePropertiesCopy : public SurfacePropertiesBase {
  QSharedPointer<const QMap<QString, QVariant>> _properties;

 public:
  SurfacePropertiesCopy(
      const QSharedPointer<const QMap<QString, QVariant>>& properties);
  bool visible() override final;
  bool visibleRaw() override final;
  vx::Color backColor() override final;
  std::tuple<double, double, double, double> backColorRaw() override final;
  bool drawAxisArrows() override final;
  bool drawAxisArrowsRaw() override final;
  bool drawBoundingBox() override final;
  bool drawBoundingBoxRaw() override final;
  bool drawOrigin() override final;
  bool drawOriginRaw() override final;
  QString faceCulling() override final;
  QString faceCullingRaw() override final;
  vx::Color frontColor() override final;
  std::tuple<double, double, double, double> frontColorRaw() override final;
  bool highlightCurrentTriangle() override final;
  bool highlightCurrentTriangleRaw() override final;
  QString shadingTechnique() override final;
  QString shadingTechniqueRaw() override final;
  QDBusObjectPath surfaceRaw() override final;
};
class SurfaceProperties : public QObject, public SurfacePropertiesBase {
  Q_OBJECT
  vx::Node* _node;

 public:
  static const char* _getPrototypeJson();
  static QSharedPointer<vx::NodePrototype> getNodePrototype();
  SurfaceProperties(vx::Node* parent);
  ~SurfaceProperties();

  bool visible() override final;
  bool visibleRaw() override final;
  static QSharedPointer<NodeProperty> visibleProperty();
  static NodePropertyTyped<vx::types::Boolean> visiblePropertyTyped();
  NodeNodeProperty visibleInstance();
  void setVisible(bool value);
 Q_SIGNALS:
  void visibleChanged(bool value);

 public:
  // Q_PROPERTY(bool Visible READ visible WRITE setVisible NOTIFY
  // visibleChanged)

  vx::Color backColor() override final;
  std::tuple<double, double, double, double> backColorRaw() override final;
  static QSharedPointer<NodeProperty> backColorProperty();
  static NodePropertyTyped<vx::types::Color> backColorPropertyTyped();
  NodeNodeProperty backColorInstance();
  void setBackColor(vx::Color value);
 Q_SIGNALS:
  void backColorChanged(vx::Color value);

 public:
  // Q_PROPERTY(vx::Color BackColor READ backColor WRITE setBackColor NOTIFY
  // backColorChanged)

  bool drawAxisArrows() override final;
  bool drawAxisArrowsRaw() override final;
  static QSharedPointer<NodeProperty> drawAxisArrowsProperty();
  static NodePropertyTyped<vx::types::Boolean> drawAxisArrowsPropertyTyped();
  NodeNodeProperty drawAxisArrowsInstance();
  void setDrawAxisArrows(bool value);
 Q_SIGNALS:
  void drawAxisArrowsChanged(bool value);

 public:
  // Q_PROPERTY(bool DrawAxisArrows READ drawAxisArrows WRITE setDrawAxisArrows
  // NOTIFY drawAxisArrowsChanged)

  bool drawBoundingBox() override final;
  bool drawBoundingBoxRaw() override final;
  static QSharedPointer<NodeProperty> drawBoundingBoxProperty();
  static NodePropertyTyped<vx::types::Boolean> drawBoundingBoxPropertyTyped();
  NodeNodeProperty drawBoundingBoxInstance();
  void setDrawBoundingBox(bool value);
 Q_SIGNALS:
  void drawBoundingBoxChanged(bool value);

 public:
  // Q_PROPERTY(bool DrawBoundingBox READ drawBoundingBox WRITE
  // setDrawBoundingBox NOTIFY drawBoundingBoxChanged)

  bool drawOrigin() override final;
  bool drawOriginRaw() override final;
  static QSharedPointer<NodeProperty> drawOriginProperty();
  static NodePropertyTyped<vx::types::Boolean> drawOriginPropertyTyped();
  NodeNodeProperty drawOriginInstance();
  void setDrawOrigin(bool value);
 Q_SIGNALS:
  void drawOriginChanged(bool value);

 public:
  // Q_PROPERTY(bool DrawOrigin READ drawOrigin WRITE setDrawOrigin NOTIFY
  // drawOriginChanged)

  QString faceCulling() override final;
  QString faceCullingRaw() override final;
  static QSharedPointer<NodeProperty> faceCullingProperty();
  static NodePropertyTyped<vx::types::Enumeration> faceCullingPropertyTyped();
  NodeNodeProperty faceCullingInstance();
  void setFaceCulling(QString value);
 Q_SIGNALS:
  void faceCullingChanged(QString value);

 public:
  // Q_PROPERTY(QString FaceCulling READ faceCulling WRITE setFaceCulling NOTIFY
  // faceCullingChanged)

  vx::Color frontColor() override final;
  std::tuple<double, double, double, double> frontColorRaw() override final;
  static QSharedPointer<NodeProperty> frontColorProperty();
  static NodePropertyTyped<vx::types::Color> frontColorPropertyTyped();
  NodeNodeProperty frontColorInstance();
  void setFrontColor(vx::Color value);
 Q_SIGNALS:
  void frontColorChanged(vx::Color value);

 public:
  // Q_PROPERTY(vx::Color FrontColor READ frontColor WRITE setFrontColor NOTIFY
  // frontColorChanged)

  bool highlightCurrentTriangle() override final;
  bool highlightCurrentTriangleRaw() override final;
  static QSharedPointer<NodeProperty> highlightCurrentTriangleProperty();
  static NodePropertyTyped<vx::types::Boolean>
  highlightCurrentTrianglePropertyTyped();
  NodeNodeProperty highlightCurrentTriangleInstance();
  void setHighlightCurrentTriangle(bool value);
 Q_SIGNALS:
  void highlightCurrentTriangleChanged(bool value);

 public:
  // Q_PROPERTY(bool HighlightCurrentTriangle READ highlightCurrentTriangle
  // WRITE setHighlightCurrentTriangle NOTIFY highlightCurrentTriangleChanged)

  QString shadingTechnique() override final;
  QString shadingTechniqueRaw() override final;
  static QSharedPointer<NodeProperty> shadingTechniqueProperty();
  static NodePropertyTyped<vx::types::Enumeration>
  shadingTechniquePropertyTyped();
  NodeNodeProperty shadingTechniqueInstance();
  void setShadingTechnique(QString value);
 Q_SIGNALS:
  void shadingTechniqueChanged(QString value);

 public:
  // Q_PROPERTY(QString ShadingTechnique READ shadingTechnique WRITE
  // setShadingTechnique NOTIFY shadingTechniqueChanged)

  vx::Node* surface();
  QDBusObjectPath surfaceRaw() override final;
  static QSharedPointer<NodeProperty> surfaceProperty();
  static NodePropertyTyped<vx::types::NodeReference> surfacePropertyTyped();
  NodeNodeProperty surfaceInstance();
  void setSurface(vx::Node* value);
 Q_SIGNALS:
  void surfaceChanged(vx::Node* value);

 public:
  // Q_PROPERTY(vx::Node* Surface READ surface WRITE setSurface NOTIFY
  // surfaceChanged)
};

}  // namespace object3d_prop
inline namespace visualizer_prop {
class View3DPropertiesEntry : public vx::PropertiesEntryBase {
  View3DPropertiesEntry() = delete;

 public:
  ~View3DPropertiesEntry();
  View3DPropertiesEntry(vx::PropType::FieldOfView, double);
  View3DPropertiesEntry(vx::PropType::LookAt, QVector3D);
  View3DPropertiesEntry(vx::PropType::Orientation, QQuaternion);
  View3DPropertiesEntry(vx::PropType::ViewSizeUnzoomed, double);
  View3DPropertiesEntry(vx::PropType::ZoomLog, double);
  View3DPropertiesEntry(vx::PropType::Objects, QList<vx::Node*>);
  View3DPropertiesEntry(vx::PropType::ShowViewCenter, bool);
};
class View3DPropertiesBase {
 public:
  virtual ~View3DPropertiesBase();
  virtual double fieldOfView() = 0;
  virtual double fieldOfViewRaw() = 0;
  virtual QVector3D lookAt() = 0;
  virtual std::tuple<double, double, double> lookAtRaw() = 0;
  virtual QQuaternion orientation() = 0;
  virtual std::tuple<double, double, double, double> orientationRaw() = 0;
  virtual double viewSizeUnzoomed() = 0;
  virtual double viewSizeUnzoomedRaw() = 0;
  virtual double zoomLog() = 0;
  virtual double zoomLogRaw() = 0;
  virtual QList<QDBusObjectPath> objectsRaw() = 0;
  virtual bool showViewCenter() = 0;
  virtual bool showViewCenterRaw() = 0;
};
class View3DPropertiesCopy : public View3DPropertiesBase {
  QSharedPointer<const QMap<QString, QVariant>> _properties;

 public:
  View3DPropertiesCopy(
      const QSharedPointer<const QMap<QString, QVariant>>& properties);
  double fieldOfView() override final;
  double fieldOfViewRaw() override final;
  QVector3D lookAt() override final;
  std::tuple<double, double, double> lookAtRaw() override final;
  QQuaternion orientation() override final;
  std::tuple<double, double, double, double> orientationRaw() override final;
  double viewSizeUnzoomed() override final;
  double viewSizeUnzoomedRaw() override final;
  double zoomLog() override final;
  double zoomLogRaw() override final;
  QList<QDBusObjectPath> objectsRaw() override final;
  bool showViewCenter() override final;
  bool showViewCenterRaw() override final;
};
class View3DProperties : public QObject, public View3DPropertiesBase {
  Q_OBJECT
  vx::Node* _node;

 public:
  static const char* _getPrototypeJson();
  static QSharedPointer<vx::NodePrototype> getNodePrototype();
  View3DProperties(vx::Node* parent);
  ~View3DProperties();

  double fieldOfView() override final;
  double fieldOfViewRaw() override final;
  static QSharedPointer<NodeProperty> fieldOfViewProperty();
  static NodePropertyTyped<vx::types::Float> fieldOfViewPropertyTyped();
  NodeNodeProperty fieldOfViewInstance();
  void setFieldOfView(double value);
 Q_SIGNALS:
  void fieldOfViewChanged(double value);

 public:
  // Q_PROPERTY(double FieldOfView READ fieldOfView WRITE setFieldOfView NOTIFY
  // fieldOfViewChanged)

  QVector3D lookAt() override final;
  std::tuple<double, double, double> lookAtRaw() override final;
  static QSharedPointer<NodeProperty> lookAtProperty();
  static NodePropertyTyped<vx::types::Position3D> lookAtPropertyTyped();
  NodeNodeProperty lookAtInstance();
  void setLookAt(QVector3D value);
 Q_SIGNALS:
  void lookAtChanged(QVector3D value);

 public:
  // Q_PROPERTY(QVector3D LookAt READ lookAt WRITE setLookAt NOTIFY
  // lookAtChanged)

  QQuaternion orientation() override final;
  std::tuple<double, double, double, double> orientationRaw() override final;
  static QSharedPointer<NodeProperty> orientationProperty();
  static NodePropertyTyped<vx::types::Orientation3D> orientationPropertyTyped();
  NodeNodeProperty orientationInstance();
  void setOrientation(QQuaternion value);
 Q_SIGNALS:
  void orientationChanged(QQuaternion value);

 public:
  // Q_PROPERTY(QQuaternion Orientation READ orientation WRITE setOrientation
  // NOTIFY orientationChanged)

  double viewSizeUnzoomed() override final;
  double viewSizeUnzoomedRaw() override final;
  static QSharedPointer<NodeProperty> viewSizeUnzoomedProperty();
  static NodePropertyTyped<vx::types::Float> viewSizeUnzoomedPropertyTyped();
  NodeNodeProperty viewSizeUnzoomedInstance();
  void setViewSizeUnzoomed(double value);
 Q_SIGNALS:
  void viewSizeUnzoomedChanged(double value);

 public:
  // Q_PROPERTY(double ViewSizeUnzoomed READ viewSizeUnzoomed WRITE
  // setViewSizeUnzoomed NOTIFY viewSizeUnzoomedChanged)

  double zoomLog() override final;
  double zoomLogRaw() override final;
  static QSharedPointer<NodeProperty> zoomLogProperty();
  static NodePropertyTyped<vx::types::Float> zoomLogPropertyTyped();
  NodeNodeProperty zoomLogInstance();
  void setZoomLog(double value);
 Q_SIGNALS:
  void zoomLogChanged(double value);

 public:
  // Q_PROPERTY(double ZoomLog READ zoomLog WRITE setZoomLog NOTIFY
  // zoomLogChanged)

  QList<vx::Node*> objects();
  QList<QDBusObjectPath> objectsRaw() override final;
  static QSharedPointer<NodeProperty> objectsProperty();
  static NodePropertyTyped<vx::types::NodeReferenceList> objectsPropertyTyped();
  NodeNodeProperty objectsInstance();
  void setObjects(QList<vx::Node*> value);
 Q_SIGNALS:
  void objectsChanged(QList<vx::Node*> value);

 public:
  // Q_PROPERTY(QList<vx::Node*> Objects READ objects WRITE setObjects NOTIFY
  // objectsChanged)

  bool showViewCenter() override final;
  bool showViewCenterRaw() override final;
  static QSharedPointer<NodeProperty> showViewCenterProperty();
  static NodePropertyTyped<vx::types::Boolean> showViewCenterPropertyTyped();
  NodeNodeProperty showViewCenterInstance();
  void setShowViewCenter(bool value);
 Q_SIGNALS:
  void showViewCenterChanged(bool value);

 public:
  // Q_PROPERTY(bool ShowViewCenter READ showViewCenter WRITE setShowViewCenter
  // NOTIFY showViewCenterChanged)
};

}  // namespace visualizer_prop
inline namespace visualizer_prop {
class VolumeRenderingPropertiesEntry : public vx::PropertiesEntryBase {
  VolumeRenderingPropertiesEntry() = delete;

 public:
  ~VolumeRenderingPropertiesEntry();
  VolumeRenderingPropertiesEntry(vx::PropType::Input, QList<vx::Node*>);
};
class VolumeRenderingPropertiesBase {
 public:
  virtual ~VolumeRenderingPropertiesBase();
  virtual QList<QDBusObjectPath> inputRaw() = 0;
};
class VolumeRenderingPropertiesCopy : public VolumeRenderingPropertiesBase {
  QSharedPointer<const QMap<QString, QVariant>> _properties;

 public:
  VolumeRenderingPropertiesCopy(
      const QSharedPointer<const QMap<QString, QVariant>>& properties);
  QList<QDBusObjectPath> inputRaw() override final;
};
class VolumeRenderingProperties : public QObject,
                                  public VolumeRenderingPropertiesBase {
  Q_OBJECT
  vx::Node* _node;

 public:
  static const char* _getPrototypeJson();
  static QSharedPointer<vx::NodePrototype> getNodePrototype();
  VolumeRenderingProperties(vx::Node* parent);
  ~VolumeRenderingProperties();

  QList<vx::Node*> input();
  QList<QDBusObjectPath> inputRaw() override final;
  static QSharedPointer<NodeProperty> inputProperty();
  static NodePropertyTyped<vx::types::NodeReferenceList> inputPropertyTyped();
  NodeNodeProperty inputInstance();
  void setInput(QList<vx::Node*> value);
 Q_SIGNALS:
  void inputChanged(QList<vx::Node*> value);

 public:
  // Q_PROPERTY(QList<vx::Node*> Input READ input WRITE setInput NOTIFY
  // inputChanged)
};

}  // namespace visualizer_prop
}  // namespace vx
