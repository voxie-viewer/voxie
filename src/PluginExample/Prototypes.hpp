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
#ifndef VOXIE_PROP_DEFINED_Output
#define VOXIE_PROP_DEFINED_Output
namespace PropType {
class Output : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::Output Output = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_Seed
#define VOXIE_PROP_DEFINED_Seed
namespace PropType {
class Seed : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::Seed Seed = {};
}
#endif
#ifndef VOXIE_PROP_DEFINED_Size
#define VOXIE_PROP_DEFINED_Size
namespace PropType {
class Size : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::Size Size = {};
}
#endif
inline namespace filter_prop {
class TheSphereGeneratorPropertiesEntry : public vx::PropertiesEntryBase {
  TheSphereGeneratorPropertiesEntry() = delete;

 public:
  ~TheSphereGeneratorPropertiesEntry();
  TheSphereGeneratorPropertiesEntry(vx::PropType::Seed, qint64);
  TheSphereGeneratorPropertiesEntry(vx::PropType::Size, qint64);
  TheSphereGeneratorPropertiesEntry(vx::PropType::Output, vx::Node*);
};
class TheSphereGeneratorPropertiesBase {
 public:
  virtual ~TheSphereGeneratorPropertiesBase();
  virtual qint64 seed() = 0;
  virtual qint64 seedRaw() = 0;
  virtual qint64 size() = 0;
  virtual qint64 sizeRaw() = 0;
  virtual QDBusObjectPath outputRaw() = 0;
};
class TheSphereGeneratorPropertiesCopy
    : public TheSphereGeneratorPropertiesBase {
  QSharedPointer<const QMap<QString, QVariant>> _properties;

 public:
  TheSphereGeneratorPropertiesCopy(
      const QSharedPointer<const QMap<QString, QVariant>>& properties);
  qint64 seed() override final;
  qint64 seedRaw() override final;
  qint64 size() override final;
  qint64 sizeRaw() override final;
  QDBusObjectPath outputRaw() override final;
};
class TheSphereGeneratorProperties : public QObject,
                                     public TheSphereGeneratorPropertiesBase {
  Q_OBJECT
  vx::Node* _node;

 public:
  static const char* _getPrototypeJson();
  static QSharedPointer<vx::NodePrototype> getNodePrototype();
  TheSphereGeneratorProperties(vx::Node* parent);
  ~TheSphereGeneratorProperties();

  qint64 seed() override final;
  qint64 seedRaw() override final;
  static QSharedPointer<NodeProperty> seedProperty();
  static NodePropertyTyped<vx::types::Int> seedPropertyTyped();
  NodeNodeProperty seedInstance();
  void setSeed(qint64 value);
 Q_SIGNALS:
  void seedChanged(qint64 value);

 public:
  // Q_PROPERTY(qint64 Seed READ seed WRITE setSeed NOTIFY seedChanged)

  qint64 size() override final;
  qint64 sizeRaw() override final;
  static QSharedPointer<NodeProperty> sizeProperty();
  static NodePropertyTyped<vx::types::Int> sizePropertyTyped();
  NodeNodeProperty sizeInstance();
  void setSize(qint64 value);
 Q_SIGNALS:
  void sizeChanged(qint64 value);

 public:
  // Q_PROPERTY(qint64 Size READ size WRITE setSize NOTIFY sizeChanged)

  vx::Node* output();
  QDBusObjectPath outputRaw() override final;
  static QSharedPointer<NodeProperty> outputProperty();
  static NodePropertyTyped<vx::types::OutputNodeReference>
  outputPropertyTyped();
  NodeNodeProperty outputInstance();
  void setOutput(vx::Node* value);
 Q_SIGNALS:
  void outputChanged(vx::Node* value);

 public:
  // Q_PROPERTY(vx::Node* Output READ output WRITE setOutput NOTIFY
  // outputChanged)
};

}  // namespace filter_prop
inline namespace visualizer_prop {
class RandomChartPropertiesEntry : public vx::PropertiesEntryBase {
  RandomChartPropertiesEntry() = delete;

 public:
  ~RandomChartPropertiesEntry();
};
class RandomChartPropertiesBase {
 public:
  virtual ~RandomChartPropertiesBase();
};
class RandomChartPropertiesCopy : public RandomChartPropertiesBase {
  QSharedPointer<const QMap<QString, QVariant>> _properties;

 public:
  RandomChartPropertiesCopy(
      const QSharedPointer<const QMap<QString, QVariant>>& properties);
};
class RandomChartProperties : public QObject, public RandomChartPropertiesBase {
  Q_OBJECT
  vx::Node* _node;

 public:
  static const char* _getPrototypeJson();
  static QSharedPointer<vx::NodePrototype> getNodePrototype();
  RandomChartProperties(vx::Node* parent);
  ~RandomChartProperties();
};

}  // namespace visualizer_prop
}  // namespace vx
