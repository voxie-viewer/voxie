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

// QDBusConnection should be included as early as possible:
// https://bugreports.qt.io/browse/QTBUG-48351 /
// https://bugreports.qt.io/browse/QTBUG-48377
#include <QtDBus/QDBusConnection>

#include "NodePrototype.hpp"

#include <QMap>
#include <QSharedDataPointer>

#include <Voxie/IVoxie.hpp>
#include <Voxie/Node/NodeGroup.hpp>

#include <Voxie/Node/DataNode.hpp>
#include <Voxie/Node/NodeProperty.hpp>
#include <Voxie/Node/PropertyValueConvertRaw.hpp>

#include <Voxie/Vis/VisualizerNode.hpp>

#include <VoxieBackend/Data/Data.hpp>

#include <VoxieClient/DBusAdaptors.hpp>
#include <VoxieClient/DBusUtil.hpp>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>

#include <QtCore/QDebug>

using namespace vx;

// QDebug overloads for std::tuple<>
// TODO: move somewhere else
template <typename... T>
struct TupleQDebugImpl;
template <typename... T>
struct TupleQDebug {
  template <std::size_t... Is>
  static void output(QDebug& dbg, const std::tuple<T...>& values,
                     std::index_sequence<Is...>) {
    TupleQDebugImpl<T...>::output(dbg, std::get<Is>(values)...);
  }
};
template <>
struct TupleQDebugImpl<> {
  static void output(QDebug& dbg) { Q_UNUSED(dbg); }
};
template <typename T>
struct TupleQDebugImpl<T> {
  static void output(QDebug& dbg, const T& val) { dbg << val; }
};
template <typename T, typename... Tail>
struct TupleQDebugImpl<T, Tail...> {
  static void output(QDebug& dbg, const T& val, const Tail&... tail) {
    dbg << val;
    dbg << ", ";
    TupleQDebugImpl<Tail...>::output(dbg, tail...);
  }
};
template <typename... T>
inline QDebug operator<<(QDebug dbg, const std::tuple<T...>& values) {
  dbg.nospace() << "(";
  TupleQDebug<T...>::output(dbg.nospace(), values,
                            std::index_sequence_for<T...>{});
  dbg.nospace() << ")";
  return dbg.maybeSpace();
}

// QDebug overload for QDBusObjectPath
// TODO: move somewhere else
inline QDebug operator<<(QDebug dbg, const QDBusObjectPath& value) {
  dbg << value.path();
  return dbg.maybeSpace();
}

// TODO: Make sure Component::plugin_/type_/name_ are set for builtin nodes

void NodePrototype::resolve1(
    const QSharedPointer<ComponentContainer>& components) {
  QList<SharedFunPtr<void(const QSharedPointer<ComponentContainer>&)>> actions =
      resolveActions1;
  resolveActions1.clear();
  for (const auto& action : actions) action(components);
}
void NodePrototype::resolve2(
    const QSharedPointer<ComponentContainer>& components) {
  QList<SharedFunPtr<void(const QSharedPointer<ComponentContainer>&)>> actions =
      resolveActions2;
  resolveActions2.clear();
  for (const auto& action : actions) action(components);
}

void NodePrototype::createHelper(QSharedPointer<Node> node,
                                 QMap<QString, QVariant> properties,
                                 QList<Node*> inputs,
                                 const QMap<QString, QDBusVariant>& options,
                                 bool registerNode) {
  vx::checkOnMainThread("NodePrototype::createHelper");

  ExportedObject::checkOptions(options, "Data", "ManualDisplayName",
                               "ParentNodeGroup"
                               "AllowCompatibilityNames");

  if (registerNode) node->setStateToSetup();

  // For visualizers: Register visualizer with Root class. TODO: Visualizers
  // probably shouldn't be special-cased here.
  if (auto visualizer = dynamic_cast<vx::VisualizerNode*>(node.data()))
    vx::voxieRoot().registerVisualizer(visualizer);

  // TODO: Should the data already be set in the constructor?
  if (auto dataNode = dynamic_cast<vx::DataNode*>(node.data())) {
    if (ExportedObject::hasOption(options, "Data"))
      dataNode->setData(Data::lookupOptional(
          ExportedObject::getOptionValue<QDBusObjectPath>(options, "Data")));
  } else {
    if (ExportedObject::hasOption(options, "Data"))
      throw vx::Exception("de.uni_stuttgart.Voxie.InvalidOption",
                          "'Data' option given but node is not a DataNode");
  }

  if (ExportedObject::hasOption(options, "ManualDisplayName"))
    node->setManualDisplayName(
        ExportedObject::getOptionValue<std::tuple<bool, QString>>(
            options, "ManualDisplayName"));

  if (ExportedObject::hasOption(options, "ParentNodeGroup")) {
    node->setParentNodeGroup(
        NodeGroup::lookupOptional(
            ExportedObject::getOptionValue<QDBusObjectPath>(options,
                                                            "ParentNodeGroup"))
            .data());
  }

  node->setNodeProperties(properties);

  for (auto input : inputs) {
    input->addChildNode(node.data());
  }

  if (registerNode) node->setStateToNormal();
}

QList<QSharedPointer<NodePrototype>> NodePrototype::allowedInputTypes() {
  QList<QSharedPointer<NodePrototype>> types;
  for (const auto& property : nodeProperties()) {
    if (property->isReference() && !property->isOutputReference()) {
      for (const auto& type : property->allowedTypes()) {
        auto typeStrong = type.toStrongRef();
        if (!typeStrong)
          throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                              "Prototype has already been destroyed");
        types << typeStrong;
      }
    }
  }
  return types;
}

const QList<QSharedPointer<NodeProperty>>& NodePrototype::nodeProperties() {
  return this->nodeProperties_;
}

QSharedPointer<NodeProperty> NodePrototype::getProperty(
    const QString& name, bool allowCompatibilityNames) {
  for (const auto& property : nodeProperties_) {
    if (property->name() == name) return property;
    if (allowCompatibilityNames)
      for (const auto& name2 : property->compatibilityNames())
        if (name2 == name) return property;
  }
  throw vx::Exception(
      "de.uni_stuttgart.Voxie.InvalidProperty",
      "Unknown property '" + name + "' for node '" + this->name() + "'");
}

// TODO: Stuff from rawJson?
QJsonValue NodePrototype::toJson() {
  QJsonObject obj;

  obj["Name"] = name();
  obj["DisplayName"] = _displayName;
  obj["Description"] = _description;

  if (icon() != "") obj["Icon"] = icon();

  if (!qSharedPointerDynamicCast<PropertyConditionTrue>(
          runFilterEnabledCondition()))
    obj["RunFilterEnabledCondition"] = runFilterEnabledCondition()->toJson();

  obj["NodeKind"] = nodeKindToString(nodeKind());

  if (nodeKind() == NodeKind::Data) {
    auto defaultExporterName = this->defaultExporterName();
    if (defaultExporterName != "") obj["DefaultExporter"] = defaultExporterName;

    auto supportedDataDBusInterfaces = this->supportedDataDBusInterfaces();
    QJsonArray interfaces;
    for (const auto& interfaceName : supportedDataDBusInterfaces)
      interfaces << interfaceName;
    obj["SupportedDataDBusInterfaces"] = interfaces;
  }

  QJsonObject properties;
  for (const auto& entry : nodeProperties())
    properties[entry->name()] = entry->toJson();
  obj["Properties"] = properties;

  return obj;
}

namespace vx {
class NodePrototypeImplementation : public NodePrototype {
  SharedFunPtr<QSharedPointer<Node>(
      const QMap<QString, QVariant>&, const QList<Node*>&,
      const QMap<QString, QDBusVariant>&, vx::NodePrototype*, bool)>
      constructor;

  static NodeKind getNodeKindFromJson(const QJsonObject& json) {
    if (json.contains("NodeKind"))
      return parseNodeKind(json["NodeKind"].toString());

    if (json.contains("ObjectKind")) {
      if (Node::showWarningOnOldObjectNames())
        qWarning() << "Got 'ObjectKind' value for prototype"
                   << json["Name"].toString();
      return parseObjectKind(json["ObjectKind"].toString());
    }

    throw vx::Exception("de.uni_stuttgart.Voxie.InvalidJson",
                        "Did not find 'NodeKind' key");
  }

 public:
  NodePrototypeImplementation(
      const QJsonObject& json,
      const SharedFunPtr<QSharedPointer<Node>(
          const QMap<QString, QVariant>&, const QList<Node*>&,
          const QMap<QString, QDBusVariant>&, vx::NodePrototype*, bool)>&
          customNodeClassConstructor)
      : NodePrototype(json["Name"].toString(), getNodeKindFromJson(json)) {
    constructor = customNodeClassConstructor;

    rawJson_ = createQSharedPointer<QJsonObject>(json);

    _displayName = json["DisplayName"].toString();
    _description = json["Description"].toString();
    if (json.contains("Icon"))
      icon_ = json["Icon"].toString();
    else
      icon_ = "";

    if (json.contains("DefaultExporter")) {
      if (nodeKind() != NodeKind::Data)
        throw vx::Exception("de.uni_stuttgart.Voxie.InvalidJson",
                            "Have DefaultExporter for non-data");
      defaultExporterName_ = json["DefaultExporter"].toString();
    }

    if (json.contains("SupportedDBusInterfaces")) {
      if (nodeKind() != NodeKind::Data)
        throw vx::Exception("de.uni_stuttgart.Voxie.InvalidJson",
                            "Have SupportedDBusInterfaces for non-data");
      qWarning() << "Got deprecated 'SupportedDBusInterfaces' element";
      for (const auto& interfaceName :
           json["SupportedDBusInterfaces"].toArray())
        this->supportedDataDBusInterfaces_ << interfaceName.toString();
    }
    if (json.contains("SupportedDataDBusInterfaces")) {
      if (nodeKind() != NodeKind::Data)
        throw vx::Exception("de.uni_stuttgart.Voxie.InvalidJson",
                            "Have SupportedDataDBusInterfaces for non-data");
      for (const auto& interfaceName :
           json["SupportedDataDBusInterfaces"].toArray())
        this->supportedDataDBusInterfaces_ << interfaceName.toString();
    }

    auto compatibilityNames = json["CompatibilityNames"].toArray();
    for (const auto& name : compatibilityNames)
      this->compatibilityNames_ << name.toString();

    //_nodeKind = getNodeKindFromJson(json["NodeKind"]);

    bool haveCustomNodeClassConstructor = customNodeClassConstructor != nullptr;
    resolveActions1 << [this, json, haveCustomNodeClassConstructor](
                           const QSharedPointer<ComponentContainer>&
                               components) {
      auto self = this->weakSelf.lock();
      // Should not happen, this should only be invoked after fromJson() has
      // finished
      if (!self) {
        throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                            "NodePrototype::weakSelf is nullptr");
      }
      auto properties = json["Properties"].toObject();
      QList<vx::SharedFunPtr<void(const QSharedPointer<vx::NodePrototype>&)>>
          finishActions;
      for (const auto& key : properties.keys())
        this->nodeProperties_.append(makeSharedQObject<NodeProperty>(
            key, properties[key].toObject(), haveCustomNodeClassConstructor,
            components, finishActions, resolveActions2));
      // TODO: Clean up parameters for NodeProperty constructor?
      for (const auto& action : finishActions) action(self);

      if (json.contains("RunFilterEnabledCondition")) {
        if (nodeKind() != NodeKind::Filter)
          throw vx::Exception("de.uni_stuttgart.Voxie.InvalidJson",
                              "Have RunFilterEnabledCondition for non-filter");
        runFilterEnabledCondition_ = PropertyCondition::parse(
            self, json["RunFilterEnabledCondition"].toObject());
      } else {
        runFilterEnabledCondition_ =
            createQSharedPointer<PropertyConditionTrue>();
      }
    };
  }

  QSharedPointer<Node> create(const QMap<QString, QVariant>& properties,
                              const QList<Node*>& inputs,
                              const QMap<QString, QDBusVariant>& options,
                              bool registerNode) override {
    return constructor(properties, inputs, options, this, registerNode);
  }
};
}  // namespace vx

QSharedPointer<vx::NodePrototype> NodePrototype::fromJson(
    const QJsonObject& json,
    const SharedFunPtr<QSharedPointer<Node>(
        const QMap<QString, QVariant>&, const QList<Node*>&,
        const QMap<QString, QDBusVariant>&, vx::NodePrototype*, bool)>&
        customNodeClassConstructor) {
  if (!customNodeClassConstructor)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "customNodeClassConstructor is nullptr");
  auto res = makeSharedQObject<NodePrototypeImplementation>(
      json, customNodeClassConstructor);
  res->weakSelf = res;
  return res;
}

QSharedPointer<vx::NodePrototype> NodePrototype::fromJson(
    const char* json,
    const SharedFunPtr<QSharedPointer<Node>(
        const QMap<QString, QVariant>&, const QList<Node*>&,
        const QMap<QString, QDBusVariant>&, vx::NodePrototype*, bool)>&
        customNodeClassConstructor) {
  QJsonParseError error;
  auto doc = QJsonDocument::fromJson(json, &error);
  if (doc.isNull())
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Error parsing JSON: " + error.errorString() +
                            "\nJSON string:\n" + json);
  if (!customNodeClassConstructor)
    throw vx::Exception("de.uni_stuttgart.Voxie.NotImplemented",
                        "Not Implemented");
  return fromJson(doc.object(), customNodeClassConstructor);
}

QString NodePrototype::defaultExporterName() {
  if (this->nodeKind() != NodeKind::Data)
    throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                        "NodePrototype::defaultExporterName() called for "
                        "non-data NodePrototype");
  return defaultExporterName_;
}

QList<QString> NodePrototype::supportedDataDBusInterfaces() {
  if (this->nodeKind() != NodeKind::Data)
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.InternalError",
        "NodePrototype::supportedDataDBusInterfaces() called for "
        "non-data NodePrototype");
  return supportedDataDBusInterfaces_;
}

class NodePrototypeAdaptorImpl : public NodePrototypeAdaptor,
                                 public ObjectPrototypeAdaptor {
  NodePrototype* object;

 public:
  NodePrototypeAdaptorImpl(NodePrototype* object)
      : NodePrototypeAdaptor(object),
        ObjectPrototypeAdaptor(object),
        object(object) {}
  ~NodePrototypeAdaptorImpl() {}

  QString nodeKind() const override {
    return nodeKindToString(object->nodeKind());
  }
  QString objectKind() const override {
    return objectKindToString(object->nodeKind());
  }

  QString name() const override { return object->name(); }

  QString displayName() const override { return object->displayName(); }

  QString description() const override { return object->description(); }

 public Q_SLOTS:
  QDBusObjectPath CreateNode(
      const QMap<QString, QDBusVariant>& properties,
      const QMap<QString, QDBusVariant>& options) override {
    try {
      if (0)
        for (const auto& key : properties.keys()) {
          auto var = QDBusVariant(properties[key]);
          auto sig = dbusGetVariantSignature(var);
          qDebug() << "Got key" << key << "with sig" << sig.signature();
          // if (sig.signature() == "(dddd)")
          //   qDebug() << "Value"
          //            << dbusGetVariantValue<
          //                   std::tuple<double, double, double, double>>(var);
#define X(...)                                    \
  if (sig == vx::dbusGetSignature<__VA_ARGS__>()) \
    qDebug() << vx::dbusGetVariantValue<__VA_ARGS__>(var);
          X(std::tuple<double, double, double, double>)
          X(QString)
          X(QDBusObjectPath)
          X(QList<QDBusObjectPath>)
          X(bool)
#undef X
          // qDebug() << "Got key" << key << "with sig" <<
          // QMetaType::typeName(properties[key].userType());
        }
      QVariantMap properties2;
      bool allowCompatibilityNames =
          ExportedObject::getOptionValueOrDefault<bool>(
              options, "AllowCompatibilityNames", true);
      for (const auto& key : properties.keys()) {
        auto property = object->getProperty(
            key, allowCompatibilityNames);  // TODO: error handling
        if (properties2.contains(property->name()))
          // Can happen with compatibility names
          throw vx::Exception(
              "de.uni_stuttgart.Voxie.DuplicateProperty",
              "Property '" + property->name() + "' was given multiple time");
        properties2[property->name()] = Node::propertyDBusToRaw(
            object, property, QDBusVariant(properties[key]));
      }
      QList<Node*> inputNodes;  // TODO: remove
      return vx::ExportedObject::getPath(
          object->create(properties2, inputNodes, options).data());
    } catch (vx::Exception& e) {
      e.handle(object);
      return vx::ExportedObject::getPath(nullptr);
    }
  }
  QDBusObjectPath CreateObject(
      const QMap<QString, QDBusVariant>& properties,
      const QMap<QString, QDBusVariant>& options) override {
    try {
      if (0)
        for (const auto& key : properties.keys()) {
          auto var = QDBusVariant(properties[key]);
          auto sig = dbusGetVariantSignature(var);
          qDebug() << "Got key" << key << "with sig" << sig.signature();
          // if (sig.signature() == "(dddd)")
          //   qDebug() << "Value"
          //            << dbusGetVariantValue<
          //                   std::tuple<double, double, double, double>>(var);
#define X(...)                                    \
  if (sig == vx::dbusGetSignature<__VA_ARGS__>()) \
    qDebug() << vx::dbusGetVariantValue<__VA_ARGS__>(var);
          X(std::tuple<double, double, double, double>)
          X(QString)
          X(QDBusObjectPath)
          X(QList<QDBusObjectPath>)
          X(bool)
#undef X
          // qDebug() << "Got key" << key << "with sig" <<
          // QMetaType::typeName(properties[key].userType());
        }
      QVariantMap properties2;
      bool allowCompatibilityNames =
          ExportedObject::getOptionValueOrDefault<bool>(
              options, "AllowCompatibilityNames", true);
      for (const auto& key : properties.keys()) {
        auto property = object->getProperty(
            key, allowCompatibilityNames);  // TODO: error handling
        if (properties2.contains(property->name()))
          // Can happen with compatibility names
          throw vx::Exception(
              "de.uni_stuttgart.Voxie.DuplicateProperty",
              "Property '" + property->name() + "' was given multiple time");
        properties2[property->name()] = Node::propertyDBusToRaw(
            object, property, QDBusVariant(properties[key]));
      }
      QList<Node*> inputNodes;  // TODO: remove
      return vx::ExportedObject::getPath(
          object->create(properties2, inputNodes, options).data());
    } catch (vx::Exception& e) {
      e.handle(object);
      return vx::ExportedObject::getPath(nullptr);
    }
  }

  QList<QDBusObjectPath> ListProperties(
      const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options);

      QList<QDBusObjectPath> propPaths = QList<QDBusObjectPath>();
      for (auto prop : object->nodeProperties()) {
        propPaths.append(vx::ExportedObject::getPath(prop));
      }
      return propPaths;
    } catch (vx::Exception& e) {
      e.handle(object);
      return QList<QDBusObjectPath>();
    }
  }
  QList<QDBusObjectPath> ListObjectProperties(
      const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options);

      QList<QDBusObjectPath> propPaths = QList<QDBusObjectPath>();
      for (auto prop : object->nodeProperties()) {
        propPaths.append(vx::ExportedObject::getPath(prop));
      }
      return propPaths;
    } catch (vx::Exception& e) {
      e.handle(object);
      return QList<QDBusObjectPath>();
    }
  }

  /**
   * @brief return the QDBuspath of the property with
   * the given name
   * @param name of the property to return
   * @return QDBusPath of the NodeProperty with the given path or
   * Exception if not existing
   */
  QDBusObjectPath GetPropertyByName(
      const QString& name,
      const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options, "AllowCompatibilityNames");
      bool allowCompatibilityNames =
          ExportedObject::getOptionValueOrDefault<bool>(
              options, "AllowCompatibilityNames", true);

      return vx::ExportedObject::getPath(
          object->getProperty(name, allowCompatibilityNames));
    } catch (vx::Exception& e) {
      e.handle(object);
      return vx::ExportedObject::getPath(nullptr);
    }
  }
  /**
   * @brief return the QDBuspath of the property with
   * the given name
   * @param name of the property to return
   * @return QDBusPath of the NodeProperty with the given path or
   * Exception if not existing
   */
  QDBusObjectPath GetObjectPropertyByName(
      const QString& name,
      const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options, "AllowCompatibilityNames");
      bool allowCompatibilityNames =
          ExportedObject::getOptionValueOrDefault<bool>(
              options, "AllowCompatibilityNames", true);

      return vx::ExportedObject::getPath(
          object->getProperty(name, allowCompatibilityNames));
    } catch (vx::Exception& e) {
      e.handle(object);
      return vx::ExportedObject::getPath(nullptr);
    }
  }

  QStringList supportedDataDBusInterfaces() const override {
    try {
      if (object->nodeKind() == NodeKind::Data)
        return object->supportedDataDBusInterfaces();
      else
        return {};
    } catch (vx::Exception& e) {
      return e.handle(object);
    }
  }
};

NodePrototype::NodePrototype(const QString& name, NodeKind kind)
    : Component(ComponentTypeInfo<NodePrototype>::name(), name), _kind(kind) {
  new NodePrototypeAdaptorImpl(this);

  rawJson_ = createQSharedPointer<QJsonObject>();  // TODO
  runFilterEnabledCondition_ =
      createQSharedPointer<PropertyConditionTrue>();  // TODO
}

QList<QString> NodePrototype::supportedComponentDBusInterfaces() {
  return {
      "de.uni_stuttgart.Voxie.NodePrototype",
      "de.uni_stuttgart.Voxie.ObjectPrototype",
  };
}
