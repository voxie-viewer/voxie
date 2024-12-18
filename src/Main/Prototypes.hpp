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
#ifndef VOXIE_PROP_DEFINED_File
#define VOXIE_PROP_DEFINED_File
namespace PropType {
class File : public vx::PropTypeBase {};
}  // namespace PropType
namespace Prop {
constexpr vx::PropType::File File = {};
}
#endif
inline namespace visualizer_prop {
class MarkdownPropertiesEntry : public vx::PropertiesEntryBase {
  MarkdownPropertiesEntry() = delete;

 public:
  ~MarkdownPropertiesEntry();
  MarkdownPropertiesEntry(vx::PropType::File, vx::Node*);
};
class MarkdownPropertiesBase {
 public:
  virtual ~MarkdownPropertiesBase();
  virtual QDBusObjectPath fileRaw() = 0;
};
class MarkdownPropertiesCopy : public MarkdownPropertiesBase {
  QSharedPointer<const QMap<QString, QVariant>> _properties;

 public:
  MarkdownPropertiesCopy(
      const QSharedPointer<const QMap<QString, QVariant>>& properties);
  QDBusObjectPath fileRaw() override final;
};
class MarkdownProperties : public QObject, public MarkdownPropertiesBase {
  Q_OBJECT
  vx::Node* _node;

 public:
  static const char* _getPrototypeJson();
  static QSharedPointer<vx::NodePrototype> getNodePrototype();
  MarkdownProperties(vx::Node* parent);
  ~MarkdownProperties();

  vx::Node* file();
  QDBusObjectPath fileRaw() override final;
  static QSharedPointer<NodeProperty> fileProperty();
  static NodePropertyTyped<vx::types::NodeReference> filePropertyTyped();
  NodeNodeProperty fileInstance();
  void setFile(vx::Node* value);
 Q_SIGNALS:
  void fileChanged(vx::Node* value);

 public:
  // Q_PROPERTY(vx::Node* File READ file WRITE setFile NOTIFY fileChanged)
};

}  // namespace visualizer_prop
}  // namespace vx
