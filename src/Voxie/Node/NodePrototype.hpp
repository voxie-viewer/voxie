/*
 * Copyright (c) 2014-2022 The Voxie Authors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#pragma once

#include <Voxie/Voxie.hpp>

#include <Voxie/Node/Node.hpp>
#include <Voxie/Node/NodeProperty.hpp>

#include <VoxieClient/Exception.hpp>
#include <VoxieClient/SharedFunPtr.hpp>

#include <VoxieClient/ObjectExport/ExportedObject.hpp>

#include <VoxieBackend/Component/Component.hpp>
#include <VoxieBackend/Component/ComponentTypeInfo.hpp>

#include <QList>
#include <QMap>
#include <QtDBus/QDBusAbstractAdaptor>
#include <QtDBus/QDBusObjectPath>
#include <QtDBus/QDBusVariant>

// NODE_PROTOTYPE_DECL* is in Node.hpp

#define NODE_PROTOTYPE_IMPL_SEP(prototypeName, classname)                 \
  QSharedPointer<vx::NodePrototype> classname::getPrototypeSingleton() {  \
    static QSharedPointer<vx::NodePrototype> prototype(                   \
        vx::NodePrototype::fromJson(                                      \
            prototypeName##Properties::_getPrototypeJson(),               \
            [](const QMap<QString, QVariant>& properties,                 \
               const QList<Node*>& inputs,                                \
               const QMap<QString, QDBusVariant>& options,                \
               vx::NodePrototype* prototype2, bool registerNode) {        \
              QSharedPointer<vx::Node> node =                             \
                  classname::createBase<classname>();                     \
              prototype2->createHelper(node, properties, inputs, options, \
                                       registerNode);                     \
              return node;                                                \
            }));                                                          \
    return prototype;                                                     \
  }

#define NODE_PROTOTYPE_IMPL(classname) \
  NODE_PROTOTYPE_IMPL_SEP(classname, classname)

#define NODE_PROTOTYPE_IMPL_2(classnamePrefix, suffix) \
  NODE_PROTOTYPE_IMPL_SEP(classnamePrefix, classnamePrefix##suffix)

#define NODE_PROTOTYPE_IMPL_DATA(classnamePrefix) \
  NODE_PROTOTYPE_IMPL_SEP(data_prop::classnamePrefix, classnamePrefix##Node)

namespace vx {
class NodePrototypeImplementation;

class VOXIECORESHARED_EXPORT NodePrototype : public vx::plugin::Component {
  Q_OBJECT

  // TODO: Merge NodePrototypeImplementation into NodePrototype
  friend class NodePrototypeImplementation;

  NodeKind _kind;

  NodePrototype(const QString& name, NodeKind kind);

  /* human-readable name used fo displaying in the UI */
  QString _displayName;
  /* description what the corresponding node does - used for display in the
   * UI*/
  QString _description;
  /* general kind of the nodes use static fields of this NodePrototype to
   * chose a value*/

  QString icon_;

  QSharedPointer<PropertyCondition> runFilterEnabledCondition_;

  QSharedPointer<QJsonObject> rawJson_;

  QList<QSharedPointer<NodeProperty>> nodeProperties_;

  QList<QString> compatibilityNames_;

  // Only valid for DataNodes
  QString defaultExporterName_;

  // Only valid for DataNodes
  QList<QString> supportedDataDBusInterfaces_;

 public:
  NodeKind nodeKind() { return _kind; }

  QSharedPointer<PropertyCondition> runFilterEnabledCondition() const {
    return runFilterEnabledCondition_;
  }

  QString defaultExporterName();

  QList<QString> supportedDataDBusInterfaces();

  const QJsonObject& rawJson() const { return *rawJson_; }

  QList<QString> supportedComponentDBusInterfaces() override;

 private:
  QList<SharedFunPtr<void(const QSharedPointer<ComponentContainer>&)>>
      resolveActions1;
  QList<SharedFunPtr<void(const QSharedPointer<ComponentContainer>&)>>
      resolveActions2;
  QWeakPointer<NodePrototype> weakSelf;

 public:
  // TODO: Clean up
  // Will create properties
  void resolve1(const QSharedPointer<ComponentContainer>& components);
  // Will create property conditions etc. (which might need properties on other
  // prototypes)
  void resolve2(const QSharedPointer<ComponentContainer>& components);

 public:
  virtual QSharedPointer<Node> create(
      const QMap<QString, QVariant>& properties, const QList<Node*>& inputs,
      const QMap<QString, QDBusVariant>& options, bool registerNode = true) = 0;

  /**
   * @brief createHelper is used in the overwritten create methods of factories
   * for sharing common code
   * it automatically sets the given properties and the parent inputs of a
   * node as well as registering the node at the root instance
   * @param node node instance to initialize
   * @param properties passed from create
   * @param inputs passed from create
   * @param options passed from create
   */
  void createHelper(QSharedPointer<vx::Node> node,
                    QMap<QString, QVariant> properties, QList<Node*> inputs,
                    const QMap<QString, QDBusVariant>& options,
                    bool registerNode);

  const QString& displayName() { return _displayName; }
  const QString& description() { return _description; }

  const QString& icon() { return icon_; }

  /**
   * @brief allowedInputTypes returns a list of QDBusObjectPaths of the
   * factories whose nodes are allowed as inputs for the node of this
   * prototype
   * @return a list of QDBusObjectPaths of the factories whose nodes are
   * allowed as inputs
   */
  virtual QList<QSharedPointer<NodePrototype>> allowedInputTypes();

  /**
   * @brief nodeProperties returns a list of the properties which the nodes
   * of this prototype have
   * @return QDBusPaths to the corresponding node properties
   */
  const QList<QSharedPointer<NodeProperty>>& nodeProperties();

  /**
   * @brief A list of alternative names which can be used for looking up this
   * prototype.
   */
  QList<QString> compatibilityNames() override final {
    return compatibilityNames_;
  }

  /**
   * @brief getProperty returns the property with the given name
   * @param name of the property to return
   * @return the property or a Exception if there is no property with
   * this name
   */
  QSharedPointer<NodeProperty> getProperty(const QString& name,
                                           bool allowCompatibilityNames);

  /**
   * @brief Return a JSON value describing the node prototype
   */
  QJsonValue toJson();

  QSharedPointer<vx::NodePrototype> static fromJson(
      const char* json,
      const SharedFunPtr<QSharedPointer<Node>(
          const QMap<QString, QVariant>&, const QList<Node*>&,
          const QMap<QString, QDBusVariant>&, vx::NodePrototype*, bool)>&
          customNodeClassConstructor);

  QSharedPointer<vx::NodePrototype> static fromJson(
      const QJsonObject& json,
      const SharedFunPtr<QSharedPointer<Node>(
          const QMap<QString, QVariant>&, const QList<Node*>&,
          const QMap<QString, QDBusVariant>&, vx::NodePrototype*, bool)>&
          customNodeClassConstructor);
};

template <>
struct ComponentTypeInfo<NodePrototype> {
  static const char* name() {
    return "de.uni_stuttgart.Voxie.ComponentType.NodePrototype";
  }
  static const QList<std::tuple<QString, bool>> compatibilityNames() {
    return {
        std::make_tuple("de.uni_stuttgart.Voxie.ObjectPrototype", true),
    };
  }
};
}  // namespace vx

Q_DECLARE_METATYPE(vx::NodePrototype*)
