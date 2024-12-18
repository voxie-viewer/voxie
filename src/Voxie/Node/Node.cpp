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

#include "Node.hpp"

#include <VoxieClient/DBusAdaptors.hpp>
#include <VoxieClient/DBusUtil.hpp>
#include <VoxieClient/JsonUtil.hpp>
#include <VoxieClient/VoxieDBus.hpp>

#include <Voxie/Data/Color.hpp>
#include <Voxie/Data/ColorInterpolator.hpp>
#include <Voxie/Data/ColorizerEntry.hpp>
#include <Voxie/Data/PropertySection.hpp>

#include <VoxieBackend/DBus/DBusTypes.hpp>

#include <Voxie/Node/NodeGroup.hpp>
#include <Voxie/Node/NodePrototype.hpp>
#include <Voxie/Node/PropertyUI.hpp>
#include <Voxie/Node/PropertyValueConvertRaw.hpp>
#include <Voxie/Node/Types.hpp>

#include <Voxie/DebugOptions.hpp>
#include <Voxie/IVoxie.hpp>

#include <QtCore/QDebug>

#include <QtGui/QIcon>

#include <QtWidgets/QLabel>
#include <QtWidgets/QWidget>

#include <QPushButton>

using namespace vx;

// TODO: Remove compatibility stuff where this is used to issue a warning
bool Node::showWarningOnOldObjectNames() { return true; }

QString Node::stateToString(State state) {
  switch (state) {
#define D(s)     \
  case State::s: \
    return #s
    D(Initial);
    D(Setup);
    D(Normal);
    D(Teardown);
    D(Destroyed);
#undef D
    default:
      return "Invalid " + QString::number((uint8_t)state);
  }
}

Node::~Node() {
  if (vx::debug_option::Log_TrackNodeLifecycle()->get())
    qDebug() << "Calling destructor for node" << this;

  if (this->state_ != State::Destroyed)
    qWarning() << "Destructor called for node not in Destroyed state" << this;

  // TODO: check whether this is also safe to do in the dtor.
  // removeChildNode() calls setNodeProperty() but should only call it for
  // non-custom properties, so that no virtual functions should be called.
  // Setting emitSignalsForThis / emitSignalsForObj to false should make sure no
  // handlers on this are being called.

  // Make a copy to make sure the list doesn't change while it's modified
  QList<Node*> childs = childNodes();
  for (const auto& node : childs) this->removeChildNode(node, false, true);

  // Make a copy to make sure the list doesn't change while it's modified
  QList<Node*> parents = parentNodes();
  for (const auto& node : parents) node->removeChildNode(this, true, false);

  if (referencingNodesAsInput.size())
    qCritical()
        << "Node being deconstructed has non-empty referencingNodesAsInput";
  if (referencingNodesAsOutput.size())
    qCritical() << "Node being deconstructed has non-empty "
                   "referencingNodesAsOutput";
}

QVariant Node::getNodeProperty(const QSharedPointer<NodeProperty>& property) {
  return getNodeProperty(property->name());
}
QVariant Node::getNodeProperty(const QString& key) {
  auto property = prototype()->getProperty(key, false);
  if (property->isCustomStorage()) {
    return getNodePropertyCustom(key);
  } else {
    if (!propertyValues_.contains(key))
      propertyValues_[key] = property->defaultValue();
    return propertyValues_[key];
  }
}

void Node::setNodeProperty(const QSharedPointer<NodeProperty>& property,
                           const QVariant& value) {
  setNodeProperty(property->name(), value);
}
void Node::setNodeProperty(const QString& key, const QVariant& value) {
  setNodeProperty(key, value, true);
}

void Node::setNodeProperty(const QSharedPointer<NodeProperty>& property,
                           const QVariant& value, bool emitSignalsForThis) {
  setNodeProperty(property->name(), value, emitSignalsForThis);
}
void Node::setNodeProperty(NodeProperty* property, const QVariant& value,
                           bool emitSignalsForThis) {
  setNodeProperty(property->name(), value, emitSignalsForThis);
}
void Node::setNodeProperty(const QString& key, const QVariant& valueOrig,
                           bool emitSignalsForThis) {
  auto property = prototype()->getProperty(key, false);
  // TODO: Add option to disable canonicalization, use verify() + value in this
  // case
  auto value = property->type()->canonicalize(*property, valueOrig);
  if (property->isReference()) {
    if (property->type() == types::NodeReferenceType()) {
      auto newValue = parseVariantNode(value);
      Node* oldValue = nullptr;
      // TODO: Check for new values whether the node state is Setup or Normal
      if (propertyValues_.contains(key)) {
        oldValue = parseVariantNode(propertyValues_[key]);
        if (oldValue) oldValue->referencingNodesAsInput.removeOne(this);
      }
      propertyValues_[key] = value;
      if (newValue) newValue->referencingNodesAsInput << this;

      if (oldValue != newValue) {
        if (oldValue) {
          Q_EMIT oldValue->childChanged(this);
          if (emitSignalsForThis) Q_EMIT this->parentChanged(oldValue);
        }
        if (newValue) {
          Q_EMIT newValue->childChanged(this);
          if (emitSignalsForThis) Q_EMIT this->parentChanged(newValue);
        }
      }
    } else if (property->type() == types::NodeReferenceListType()) {
      auto newValue = parseVariantNodeList(value);
      QList<Node*> oldValue;
      // TODO: Check for new values whether the node state is Setup or Normal
      if (propertyValues_.contains(key)) {
        oldValue = parseVariantNodeList(propertyValues_[key]);
        for (const auto& val : oldValue)
          if (val) val->referencingNodesAsInput.removeOne(this);
      }
      propertyValues_[key] = value;
      for (const auto& val : newValue) {
        if (val) val->referencingNodesAsInput << this;
      }

      QSet<Node*> inOldValue;
      for (const auto& val : oldValue)
        if (val) inOldValue << val;
      QSet<Node*> inNewValue;
      for (const auto& val : newValue)
        if (val) inNewValue << val;
      for (const auto& val : oldValue) {
        if (val && !inNewValue.contains(val)) {
          Q_EMIT val->childChanged(this);
          if (emitSignalsForThis) Q_EMIT this->parentChanged(val);
        }
      }
      for (const auto& val : newValue) {
        if (val && !inOldValue.contains(val)) {
          Q_EMIT val->childChanged(this);
          if (emitSignalsForThis) Q_EMIT this->parentChanged(val);
        }
      }
    } else if (property->type() == types::OutputNodeReferenceType()) {
      auto newValue = parseVariantNode(value);
      Node* oldValue = nullptr;
      // TODO: Check for new values whether the node state is Setup or Normal
      if (propertyValues_.contains(key)) {
        oldValue = parseVariantNode(propertyValues_[key]);
        if (oldValue) oldValue->referencingNodesAsOutput.removeOne(this);
      }
      propertyValues_[key] = value;
      if (newValue) newValue->referencingNodesAsOutput << this;

      if (oldValue != newValue) {
        if (oldValue) {
          if (emitSignalsForThis) Q_EMIT this->childChanged(oldValue);
          Q_EMIT oldValue->parentChanged(this);
        }
        if (newValue) {
          if (emitSignalsForThis) Q_EMIT this->childChanged(newValue);
          Q_EMIT newValue->parentChanged(this);
        }
      }
    } else {
      throw Exception("de.uni_stuttgart.Voxie.Error",
                      "Unknown reference type: " + property->typeName());
    }
  } else if (property->isCustomStorage()) {
    if (emitSignalsForThis)
      setNodePropertyCustom(key, value);
    else
      qCritical() << "Trying to call setNodePropertyCustom() with "
                     "emitSignalsForThis = false";
  } else {
    // TODO: check type
    propertyValues_[key] = value;
  }
  if (emitSignalsForThis) Q_EMIT this->propertyChanged(property, value);
}

QVariant Node::getNodePropertyCustom(QString key) {
  throw Exception("de.uni_stuttgart.Voxie.Error",
                  "Custom property getter for '" + key + "' not found");
}

void Node::setNodePropertyCustom(QString key, QVariant value) {
  Q_UNUSED(value);
  throw Exception("de.uni_stuttgart.Voxie.Error",
                  "Custom property setter for '" + key + "' not found");
}

void Node::emitCustomPropertyChanged(
    const QSharedPointer<NodeProperty>& property) {
  if (prototype()->getProperty(property->name(), false) != property)
    throw Exception("de.uni_stuttgart.Voxie.InternalError",
                    "Property " + property->name() +
                        " is for another prototype (not " +
                        prototype()->name() + ")");
  if (!property->isCustomStorage())
    throw Exception(
        "de.uni_stuttgart.Voxie.InternalError",
        "Property " + property->name() + " does not have custom storage");

  auto value = getNodePropertyCustom(property->name());
  Q_EMIT this->propertyChanged(property, value);
}

void Node::setNodeProperties(const QMap<QString, QVariant>& options) {
  QList<QSharedPointer<NodeProperty>> properties;
  for (const auto& key : options.keys())
    properties << prototype()->getProperty(key, false);

  std::sort(properties.begin(), properties.end(),
            [](const QSharedPointer<NodeProperty>& p1,
               const QSharedPointer<NodeProperty>& p2) {
              if (p1->callSetOrder() < p2->callSetOrder()) return true;
              if (p1->callSetOrder() > p2->callSetOrder()) return false;
              return p1->name() < p2->name();
            });

  for (auto property : properties) {
    // qDebug() << "SOP" << property->name();
    this->setNodeProperty(property, options[property->name()]);
  }
}

QSharedPointer<QObject> Node::getPropertyUIData(QString propertyName) {
  Q_UNUSED(propertyName);
  return QSharedPointer<QObject>();
}

QString Node::displayName() const {
  return displayNameIsSetManually ? manualDisplayName_ : automaticDisplayName_;
}

void Node::destroy() {
  vx::checkOnMainThread("Node::destroy");

  if (vx::debug_option::Log_TrackNodeLifecycle()->get())
    qDebug() << "Calling destroy() for node" << this;

  if (this->state_ == State::Destroyed) {
    qWarning() << "Node::destroy() called for destroyed node";
    return;
  }
  if (this->state_ == State::Teardown) {
    qWarning() << "Node::destroy() called recursively";
    return;
  }

  // If the state is still initial, the teardown step can be skipped because
  // there are no references to this node from other nodes.
  if (this->state_ != State::Initial) {
    if (vx::debug_option::Log_TrackNodeLifecycle()->get())
      qDebug() << "Doing teardown for node" << this;

    this->state_ = State::Teardown;
    Q_EMIT this->stateChanged(State::Teardown);

    // TODO: Removing all references to this node from other nodes should be
    // done here.

    if (this->state_ != State::Teardown) {
      qWarning() << "Node::destroy(): Expected state Teardown, got"
                 << stateToString(this->state_);
      return;
    }
  }

  if (vx::debug_option::Log_TrackNodeLifecycle()->get())
    qDebug() << "Node is now destroyed" << this;

  this->state_ = State::Destroyed;
  Q_EMIT this->stateChanged(State::Destroyed);

  // stateChanged(State::Destroyed) will cause the Root class to drop this
  // node from the global list of nodes. After that the node should be
  // freed (possibly once all other holders of references destroy those).

  // Note: This can happen in some cases if some thread still has a strong
  // reference to the node
  if (thisSharedWeakBase())
    qInfo() << "Node" << this->getPath().path()
            << "still has reference at end of destroy() method";
}

void Node::setAutomaticDisplayName(const QString& name) {
  automaticDisplayName_ = name;
  if (!displayNameIsSetManually) {
    // qDebug() << "Node::displayNameChanged()" << displayName();
    Q_EMIT displayNameChanged(displayName());
  }
}

void Node::setManualDisplayName(const std::tuple<bool, QString>& name) {
  if (!std::get<0>(name) && std::get<1>(name) != "")
    throw Exception(
        "de.uni_stuttgart.Voxie.InvalidArgument",
        "Calling setManualDisplayName() with false and a non-empty string");
  displayNameIsSetManually = std::get<0>(name);
  manualDisplayName_ = std::get<1>(name);
  // qDebug() << "Node::displayNameChanged()" << displayName();
  Q_EMIT displayNameChanged(displayName());
}

QIcon Node::icon() {
  // TODO: interpret paths relative to JSON file?
  QString iconName = this->prototype()->icon();
  if (iconName != "")
    return QIcon(iconName);
  else if (nodeKind() == NodeKind::Filter)
    return QIcon(":/icons/universal.png");
  else if (nodeKind() == NodeKind::Property)
    return QIcon(":/icons/property.png");
  else
    return QIcon();
}

QList<Node*> Node::parentNodes() {
  QList<Node*> list;

  // Search through all input nodes
  for (const auto& property : prototype()->nodeProperties()) {
    if (!property->isReference() || property->isOutputReference()) continue;

    if (property->type() == types::NodeReferenceType()) {
      auto ptr = parseVariantNode(this->getNodeProperty(property));
      if (ptr) list.append(ptr);
    } else if (property->type() == types::NodeReferenceListType()) {
      for (const auto& ptr :
           parseVariantNodeList(this->getNodeProperty(property)))
        if (ptr) list.append(ptr);
    } else {
      throw Exception("de.uni_stuttgart.Voxie.Error",
                      "Unknown reference type: " + property->typeName());
    }
  }

  // Search for nodes which reference this nodes as output
  for (const auto& ptr : referencingNodesAsOutput)
    if (ptr) list.append(ptr);

  return list;
}
QList<Node*> Node::childNodes() {
  QList<Node*> list;

  // Search through all output nodes
  for (const auto& property : prototype()->nodeProperties()) {
    if (!property->isReference() || !property->isOutputReference()) continue;

    if (property->type() == types::OutputNodeReferenceType()) {
      auto ptr = parseVariantNode(this->getNodeProperty(property));
      if (ptr) list.append(ptr);
    } else {
      throw Exception("de.uni_stuttgart.Voxie.Error",
                      "Unknown reference type: " + property->typeName());
    }
  }

  // Search for nodes which reference this nodes as input
  for (const auto& ptr : referencingNodesAsInput)
    if (ptr) list.append(ptr);

  return list;
}

void Node::addChildNode(Node* obj, int slot) {
  voxieRoot().createNodeConnection(this, obj, slot);
}

void Node::removeChildNode(Node* obj, int index) {
  if (index != -1)
    removeChildNode(obj, index, true, true);
  else
    removeChildNode(obj, true, true);
}

void Node::removeChildNode(Node* obj, int index, bool emitSignalsForThis,
                           bool emitSignalsForObj) {
  // Search for the output property of matching index this which has obj as
  // value
  int counter = 0;
  for (const auto& property : this->prototype()->nodeProperties()) {
    if (!property->isReference() || !property->isOutputReference()) continue;

    auto value = parseVariantNode(this->getNodeProperty(property));

    if (index == counter && value == obj) {
      this->setNodeProperty(property, getVariant((Node*)nullptr),
                            emitSignalsForThis);
      return;
    }
    counter++;
  }

  // Search for the input property of matching index this which has obj as value
  counter = 0;
  for (const auto& property : obj->prototype()->nodeProperties()) {
    if (!property->isReference() || property->isOutputReference()) continue;

    if (property->type() == types::NodeReferenceType()) {
      auto value = parseVariantNode(obj->getNodeProperty(property));
      if (index == counter && value == this) {
        obj->setNodeProperty(property, getVariant((Node*)nullptr),
                             emitSignalsForObj);
        return;
      }

    } else if (property->type() == types::NodeReferenceListType()) {
      QList<Node*> list = parseVariantNodeList(obj->getNodeProperty(property));
      if (index == counter && list.removeOne(this)) {
        obj->setNodeProperty(property, getVariant(list), emitSignalsForObj);
        return;
      }

    } else {
      throw Exception("de.uni_stuttgart.Voxie.Error",
                      "Unknown reference type: " + property->typeName());
      return;
    }
    counter++;
  }

  qWarning() << "Unable to remove" << prototype()->name() << obj
             << "as child of" << obj->prototype() << this;
}

void Node::removeChildNode(Node* obj, bool emitSignalsForThis,
                           bool emitSignalsForObj) {
  // Search for first output property in this which has obj as value
  for (const auto& property : this->prototype()->nodeProperties()) {
    if (!property->isReference() || !property->isOutputReference()) continue;

    auto value = parseVariantNode(this->getNodeProperty(property));
    if (value != obj) continue;

    this->setNodeProperty(property, getVariant((Node*)nullptr),
                          emitSignalsForThis);
    return;
  }

  // Search for first input property in obj which has this as value
  for (const auto& property : obj->prototype()->nodeProperties()) {
    if (!property->isReference() || property->isOutputReference()) continue;

    if (property->type() == types::NodeReferenceType()) {
      auto value = parseVariantNode(obj->getNodeProperty(property));
      if (value != this) continue;

      obj->setNodeProperty(property, getVariant((Node*)nullptr),
                           emitSignalsForObj);
    } else if (property->type() == types::NodeReferenceListType()) {
      QList<Node*> list = parseVariantNodeList(obj->getNodeProperty(property));
      if (!list.removeOne(this)) continue;

      obj->setNodeProperty(property, getVariant(list), emitSignalsForObj);
    } else {
      throw Exception("de.uni_stuttgart.Voxie.Error",
                      "Unknown reference type: " + property->typeName());
    }
    return;
  }

  qWarning() << "Unable to remove" << prototype()->name() << obj
             << "as child of" << obj->prototype() << this;

  /*
  childNodes_.removeOne(obj);
  obj->parentNodes_.removeOne(this);
  Q_EMIT childChanged(obj);
  Q_EMIT obj->parentChanged(this);
  */
}

QWidget* Node::getCustomPropertySectionContent(const QString& name) {
  throw Exception("de.uni_stuttgart.Voxie.Error",
                  "Could not find custom property section content with name '" +
                      name + "' in node with type " +
                      this->prototype()->name());
}

void Node::addPropertySection(QWidget* section, int position,
                              bool isInitiallyExpanded) {
  vx::checkOnMainThread("Node::addPropertySection");

  // TODO: Avoid using properties here?
  section->setProperty("isInitiallyExpanded",
                       QVariant::fromValue<bool>(isInitiallyExpanded));

  // qDebug() << "addPropertySection" << section << position <<
  // defaultSectionPosition_;
  if (position == -1) {
    if (defaultSectionPosition_ == -1) {
      position = propertySections_.size();
    } else {
      position = defaultSectionPosition_;
      defaultSectionPosition_++;
    }
  } else if (position < defaultSectionPosition_) {
    defaultSectionPosition_++;
  }
  // qDebug() << "addPropertySection2" << section << position <<
  // defaultSectionPosition_;
  if (position < 0 || position > propertySections_.size()) {
    qWarning() << "Trying to add property section at incorrect position";
    position = propertySections_.size();
  }

  connect(section, &QObject::destroyed, this,
          [this, section]() { propertySections_.removeOne(section); });
  propertySections_.insert(position, section);

  connect(this, &QObject::destroyed, section, &QObject::deleteLater);
  Q_EMIT propertySectionAdded(section);
}

void Node::addContextMenuAction(QAction* action) {
  contextMenuActions_ << action;
}

bool Node::isCreatableChild(NodeKind node) { return isAllowedChild(node); }

// TODO: Deal with cycles, be faster on DACs etc.
bool Node::hasAncestor(Node* node) {
  for (const auto& obj : parentNodes()) {
    if (obj == node) return true;
    if (obj->hasAncestor(node)) return true;
  }
  return false;
}

bool Node::hasParent(Node* node) {
  for (const auto& obj : parentNodes())
    if (obj == node) return true;
  return false;
}

bool Node::hasChild(Node* node) {
  for (const auto& obj : childNodes())
    if (obj == node) return true;
  return false;
}

NodeKind Node::nodeKind() { return prototype()->nodeKind(); }

namespace vx {
namespace {
class NodeAdaptorImpl : public NodeAdaptor, public ObjectAdaptor {
  Node* object;

 public:
  NodeAdaptorImpl(Node* object)
      : NodeAdaptor(object), ObjectAdaptor(object), object(object) {}
  virtual ~NodeAdaptorImpl() {}

  // Note: Returning errors from the getters / setters currently does not work

  QDBusObjectPath parentNodeGroup() const override {
    if (object->parentNodeGroup() == nullptr) {
      return QDBusObjectPath("/");
    } else {
      return object->parentNodeGroup().data()->getPath();
    }
  }

  void setParentNodeGroup(const QDBusObjectPath& value) override {
    object->setParentNodeGroup(NodeGroup::lookupOptional(value).data());
  }

  QStringList exportedProperties() const override {
    QStringList result;

    for (auto property : object->exportedProperties()) {
      result.append(property->name());
    }

    return result;
  }

  void setExportedProperties(const QStringList& value) override {
    QList<QSharedPointer<NodeProperty>> result;

    for (QString propertyString : value) {
      for (auto property : object->prototype()->nodeProperties()) {
        if (property->name() == propertyString) result.append(property);
      }
    }

    object->setExportedProperties(result);
  }

  QString displayName() const override {
    try {
      return object->displayName();
    } catch (vx::Exception& e) {
      e.handle(object);
      qWarning() << "Got error during DBus property:" << e.what();
      return "";
    }
  }

  QString automaticDisplayName() const override {
    try {
      return object->automaticDisplayName();
    } catch (vx::Exception& e) {
      e.handle(object);
      qWarning() << "Got error during DBus property:" << e.what();
      return "";
    }
  }

  std::tuple<bool, QString> manualDisplayName() const override {
    try {
      return object->manualDisplayName();
    } catch (vx::Exception& e) {
      e.handle(object);
      qWarning() << "Got error during DBus property:" << e.what();
      return std::make_tuple(false, "");
    }
  }
  void setManualDisplayName(std::tuple<bool, QString> value) override {
    try {
      object->setManualDisplayName(value);
    } catch (vx::Exception& e) {
      e.handle(object);
      qWarning() << "Got error during DBus property:" << e.what();
      return;
    }
  }

  vx::TupleVector<double, 2> graphPosition() const override {
    try {
      return toTupleVector(voxieRoot().getGraphPosition(object));
    } catch (vx::Exception& e) {
      qWarning() << "Got error during DBus property:" << e.what();
      return e.handle(object);
    }
  }

  void setGraphPosition(vx::TupleVector<double, 2> pos) override {
    try {
      voxieRoot().setGraphPosition(object, toVector(pos));
    } catch (vx::Exception& e) {
      qWarning() << "Got error during DBus property:" << e.what();
      e.handle(object);
    }
  }

  QDBusObjectPath prototype() const override {
    try {
      return ExportedObject::getPath(object->prototype());
    } catch (vx::Exception& e) {
      qWarning() << "Got error during DBus property:" << e.what();
      return e.handle(object);
    }
  }

  QDBusVariant GetProperty(
      const QString& key, const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options, "AllowCompatibilityNames");
      bool allowCompatibilityNames =
          ExportedObject::getOptionValueOrDefault<bool>(
              options, "AllowCompatibilityNames", true);

      auto property =
          this->object->prototype()->getProperty(key, allowCompatibilityNames);
      auto valueRaw = this->object->getNodeProperty(property);
      return property->type()->rawToDBus(valueRaw);
    } catch (vx::Exception& e) {
      e.handle(object);
      return QDBusVariant();
    }
  }

  void SetProperty(const QString& key, const QDBusVariant& value,
                   const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options, "AllowCompatibilityNames");
      bool allowCompatibilityNames =
          ExportedObject::getOptionValueOrDefault<bool>(
              options, "AllowCompatibilityNames", true);

      auto prototype = this->object->prototype();
      auto property = prototype->getProperty(key, allowCompatibilityNames);
      auto valueRaw =
          Node::propertyDBusToRaw(prototype.data(), property, value);
      this->object->setNodeProperty(property, valueRaw);
    } catch (vx::Exception& e) {
      e.handle(object);
    }
  }

  void SetProperties(const QMap<QString, QDBusVariant>& values,
                     const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options, "AllowCompatibilityNames");
      bool allowCompatibilityNames =
          ExportedObject::getOptionValueOrDefault<bool>(
              options, "AllowCompatibilityNames", true);

      QVariantMap properties;
      for (const auto& key : values.keys()) {
        auto prototype = this->object->prototype();
        auto property = prototype->getProperty(key, allowCompatibilityNames);
        auto valueRaw = Node::propertyDBusToRaw(prototype.data(), property,
                                                QDBusVariant(values[key]));
        if (properties.contains(property->name()))
          // Can happen with compatibility names
          throw vx::Exception(
              "de.uni_stuttgart.Voxie.DuplicateProperty",
              "Property '" + property->name() + "' was given multiple time");
        properties[property->name()] = valueRaw;
      }
      this->object->setNodeProperties(properties);
    } catch (vx::Exception& e) {
      e.handle(object);
    }
  }

  void Destroy(const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options);

      this->object->destroy();
    } catch (vx::Exception& e) {
      e.handle(object);
    }
  }
};
}  // namespace
}  // namespace vx

void Node::addNodePropertyUIToSection(
    Node* self, PropertySection* section,
    const QSharedPointer<NodeProperty>& property, const QJsonObject& json) {
  // TODO: Is this ok if the QPointer's dtor is called from another thread?
  QSharedPointer<QPointer<PropertyUI>> ptrPtrUi =
      createQSharedPointer<QPointer<PropertyUI>>(nullptr);

  auto generatePropertyLayout = [=]() {
    auto type = property->type();
    *ptrPtrUi = type->createUI(property, self);
    (*ptrPtrUi)->setParent(self);
    (*ptrPtrUi)->init();

    QString labelStyle = (*ptrPtrUi)->isMultiline() ? "ExtraLine" : "SameLine";
    if (json.contains("LabelStyle")) labelStyle = json["LabelStyle"].toString();

    if (labelStyle == "SameLine") {
      QHBoxLayout* layout = new QHBoxLayout();
      QLabel* label = new QLabel(property->displayName() + ":");
      auto widget = (*ptrPtrUi)->widget();
      if (property->shortDescription() != "") {
        label->setToolTip(property->shortDescription());
        // TODO: overwrite widget tooltip here?
        widget->setToolTip(property->shortDescription());
      }
      layout->addWidget(label);
      layout->addWidget(widget);
      return layout;
    } else if (labelStyle == "ExtraLine") {
      QHBoxLayout* layout = new QHBoxLayout();
      QVBoxLayout* layout2 = new QVBoxLayout();
      QLabel* label = new QLabel(property->displayName() + ":");
      auto widget = (*ptrPtrUi)->widget();
      if (property->shortDescription() != "") {
        label->setToolTip(property->shortDescription());
        // TODO: overwrite widget tooltip here?
        widget->setToolTip(property->shortDescription());
      }
      // TODO: How should this look like? Where should the export button be?
      layout2->addWidget(label);
      layout2->addWidget(widget);
      layout->addLayout(layout2);
      return layout;
    } else if (labelStyle == "None") {
      auto widget = (*ptrPtrUi)->widget();
      if (property->shortDescription() != "") {
        // TODO: overwrite widget tooltip here?
        widget->setToolTip(property->shortDescription());
      }
      auto layout = new QHBoxLayout();
      layout->addWidget(widget);
      return layout;
    } else {
      qWarning() << "Unknown label style:" << labelStyle;
      return (QHBoxLayout*)nullptr;
    }
  };

  // the button that appears next to each property and can be used to export
  // that property if it is inside a node group
  QPushButton* btnExport = new QPushButton();
  btnExport->setCheckable(true);
  btnExport->setToolTip("Export");
  btnExport->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  // only enable export button if node is in a node group
  btnExport->setEnabled(parentNodeGroup_ != nullptr);

  // enable/disable button when parent node group of this node is changed
  connect(this, &Node::parentNodeGroupChanged, this,
          [btnExport](NodeGroup* newNodeGroup) {
            btnExport->setEnabled(newNodeGroup != nullptr);
          });

  // on loading make the button checked if the loaded property is already in the
  // exportedProperties list
  if (self->exportedProperties().contains(property)) {
    btnExport->setIcon(QIcon(":/icons/bookmark--minus.png"));
    btnExport->setChecked(true);
  } else {
    btnExport->setIcon(QIcon(":/icons/bookmark--plus.png"));
    btnExport->setChecked(false);
  }

  // if the list of exported properties changes we also need to check if the
  // property of our button is in there so that we can update the button's
  // checked status
  QObject::connect(
      self, &Node::exportedPropertiesChanged,
      [btnExport, property](
          const QList<QSharedPointer<NodeProperty>>& exportedProperties) {
        btnExport->setChecked(exportedProperties.contains(property));
      });

  QSharedPointer<QPointer<QHBoxLayout>> propLayout =
      createQSharedPointer<QPointer<QHBoxLayout>>(nullptr);

  // change icon to plus or minus bookmark depending on if
  // button is checked and add or remove the property from
  // the exportedProperties list
  QObject::connect(btnExport, &QPushButton::toggled, btnExport, [=]() {
    if (btnExport->isChecked()) {
      // each child node gets its own section in the parent node groups
      // properties ui. this->exportedPropertiesSection is a pointer to that
      // section. Create the section if it doesn't exist already. We'll need it
      // later.
      if (self->exportedPropertiesSection == nullptr) {
        self->exportedPropertiesSection =
            new PropertySection(self->displayName());

        connect(self, &Node::displayNameChanged,
                self->exportedPropertiesSection, [=]() {
                  self->exportedPropertiesSection->setWindowTitle(
                      self->displayName());
                });

        if (self->parentNodeGroup()) {
          self->parentNodeGroup().data()->addPropertySection(
              self->exportedPropertiesSection);
        }
      }

      // add the property's widget to the parent node group's properties ui
      *propLayout = generatePropertyLayout();
      self->exportedPropertiesSection->addHBoxLayout(*propLayout);

      // export button should now get a minus icon to signify that the property
      // was exported
      btnExport->setIcon(QIcon(":/icons/bookmark--minus.png"));

      // add the property to the list of exported properties if it's not already
      // in there
      QList<QSharedPointer<NodeProperty>> list = self->exportedProperties();
      if (!list.contains(property)) {
        list.append(property);
        self->setExportedProperties(list);
      }
    } else {
      // user clicked the button to un-export the property

      // change the button icon back
      btnExport->setIcon(QIcon(":/icons/bookmark--plus.png"));

      // remove the property from the exported properties list
      QList<QSharedPointer<NodeProperty>> list = self->exportedProperties();
      if (list.contains(property)) {
        list.removeAll(property);
        self->setExportedProperties(list);

        if (*propLayout != nullptr && self->parentNodeGroup()) {
          for (int i = 0; i < (*propLayout)->count(); ++i) {
            QWidget* widget = (*propLayout)->itemAt(i)->widget();
            if (widget != nullptr) widget->deleteLater();
          }
          (*propLayout)->deleteLater();
          (*propLayout) = nullptr;
          (*ptrPtrUi)->deleteLater();
          (*ptrPtrUi) = nullptr;

          // delete this node's properties section from the parent node groups
          // properties ui if that section is now empty
          if (list.isEmpty()) {
            self->exportedPropertiesSection->deleteLater();
            self->exportedPropertiesSection = nullptr;
          }
        }
      }
    }
  });

  auto propertyLayout = generatePropertyLayout();
  QWidget* spacer = new QWidget();
  spacer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  propertyLayout->addWidget(spacer);
  propertyLayout->addWidget(btnExport);
  section->addHBoxLayout(propertyLayout);
}

Node::Node(const QString& type, const QSharedPointer<NodePrototype>& prototype)
    : DynamicObject(type), prototype_(prototype) {
  auto adaptor = new NodeAdaptorImpl(this);
  QObject::connect(this, &Node::propertyChanged, this,
                   [adaptor](const QSharedPointer<NodeProperty>& property,
                             const QVariant& value) {
                     try {
                       NodeAdaptor* adaptor1 = adaptor;
                       Q_EMIT adaptor1->PropertyChanged(
                           property->name(), property->type()->rawToDBus(value),
                           vx::emptyOptions());
                       ObjectAdaptor* adaptor2 = adaptor;
                       Q_EMIT adaptor2->PropertyChanged(
                           property->name(), property->type()->rawToDBus(value),
                           vx::emptyOptions());
                     } catch (Exception& e) {
                       qWarning()
                           << "Exception while emitting PropertyChanged event:"
                           << e.message();
                     }
                   });

  if (vx::debug_option::Log_TrackNodeLifecycle()->get())
    qDebug() << "Calling constructor for node" << this;

  if (!voxieRoot().isHeadless()) {
    for (const auto& spSection_ : this->prototype()
                                      ->rawJson()["UI"]
                                      .toObject()["SidePanelSections"]
                                      .toArray()) {
      auto spSection = expectObject(spSection_);

      QString sectionType = "";
      if (spSection.contains("Type"))
        sectionType = expectString(spSection["Type"]);

      if (sectionType == "AutomaticSectionPlaceholder") {
        if (defaultSectionPosition_ != -1) {
          qWarning() << "Got multiple AutomaticSectionPlaceholder entries";
        }
        defaultSectionPosition_ = propertySections().size();
      } else if (sectionType == "") {
        QString name = spSection["Name"].toString();
        QString displayName = spSection["DisplayName"].toString();
        // qDebug() << "Section:" << name << displayName;

        bool showOnStartup = true;
        if (spSection.contains("ShowOnStartup"))
          showOnStartup = expectBool(spSection["ShowOnStartup"]);

        auto section = new PropertySection(displayName);
        this->addPropertySection(section, propertySections().size(),
                                 showOnStartup);

        sections << section;
      } else {
        qWarning() << "Unknown side panel section type:" << sectionType;
      }
    }
  }
}

// This will be executed after the node has been fully
// constructed and all virtual methods are available
void Node::initialize() {
  DynamicObject::initialize();

  // TODO: With the SliceVisualizer there there is some flicker effect when
  // calling initializeReal() directly
  if (prototype()->name() == "de.uni_stuttgart.Voxie.FakeFilter") {
    initializeReal();
  } else {
    enqueueOnThread(this, [this] { this->initializeReal(); });
  }
}

void Node::initializeReal() {
  // qDebug() << "CREATE" << prototype()->_name;

  if (voxieRoot().isHeadless()) return;

  PropertySection* propSection = nullptr;

  QSet<QSharedPointer<NodeProperty>> handledProperties;
  int pos = 0;
  for (const auto& spSection_ : this->prototype()
                                    ->rawJson()["UI"]
                                    .toObject()["SidePanelSections"]
                                    .toArray()) {
    auto spSection = spSection_.toObject();

    QString sectionType = "";
    if (spSection.contains("Type"))
      sectionType = expectString(spSection["Type"]);

    if (sectionType == "AutomaticSectionPlaceholder") {
      // Do nothing
    } else if (sectionType == "") {
      if (pos >= sections.size()) {
        qCritical() << "pos >= sections.size()";
        return;
      }
      PropertySection* section = sections[pos];
      if (!section) {
        qWarning() << "PropertySection has been destroyed";
        continue;
      }

      for (const auto& entry_ : spSection["Entries"].toArray()) {
        auto entry = entry_.toObject();
        auto type = entry["Type"].toString();

        if (type == "Property") {
          auto propertyName = entry["Property"].toString();
          auto property = this->prototype()->getProperty(propertyName, false);

          addNodePropertyUIToSection(this, section, property, entry);
          handledProperties.insert(property);
        } else if (type == "Custom") {
          auto name = entry["Name"].toString();

          QWidget* widget;
          try {
            widget = this->getCustomPropertySectionContent(name);
          } catch (vx::Exception& e) {
            qWarning() << "Error while creating custom section entry with name"
                       << name << "for node of type"
                       << this->prototype()->name();
            continue;
          }

          auto layout = new QHBoxLayout();
          layout->addWidget(widget);
          section->addHBoxLayout(layout);
        } else {
          qWarning() << "Unknown section entry type:" << type;
        }
      }

      pos++;
    } else {
      qWarning() << "Unknown side panel section type:" << sectionType;
    }
  }

  QList<QSharedPointer<NodeProperty>> properties =
      this->prototype()->nodeProperties();
  std::sort(properties.begin(), properties.end(),
            [](const QSharedPointer<NodeProperty>& p1,
               const QSharedPointer<NodeProperty>& p2) {
              if (p1->uiPosition() < p2->uiPosition()) return true;
              if (p1->uiPosition() > p2->uiPosition()) return false;
              return p1->name() < p2->name();
            });
  for (const auto& property : properties) {
    if (property->isCustomUI()) continue;
    if (handledProperties.contains(property)) continue;
    /*
    if (property->name() == "de.uni_stuttgart.Voxie.Input" ||
        property->name() == "de.uni_stuttgart.Voxie.Output")
      continue;  // TODO
    */
    // qDebug() << property->name();
    if (!propSection) {
      propSection = new PropertySection("Node Properties");
      this->addPropertySection(propSection);
    }
    addNodePropertyUIToSection(this, propSection, property, QJsonObject());
  }
}

Node* Node::parseVariantNode(const QVariant& variant) {
  auto value = parseVariant<QDBusObjectPath>(variant);
  if (value.path() == "/") return nullptr;
  auto ptr = ExportedObject::lookupWeakObject(value);
  if (!ptr) {
    /*
    throw Exception("de.uni_stuttgart.Voxie.Error",
                             "Failed to look up node " + value.path());
    */
    qWarning() << "Failed to look up node" << value.path();
    return nullptr;
  }
  auto obj = dynamic_cast<Node*>(ptr);
  if (!obj) {
    /*
    throw Exception("de.uni_stuttgart.Voxie.Error",
                             "Path " + value.path() + " is not an vx::Node");
    */
    qWarning() << "Path" << value.path() << "is not an vx::Node";
    return nullptr;
  }
  return obj;
}
QList<Node*> Node::parseVariantNodeList(const QVariant& variant) {
  auto list = parseVariant<QList<QDBusObjectPath>>(variant);
  QList<Node*> res;
  for (const auto& value : list) {
    if (value.path() == "/") {
      qWarning() << "Node list contains nullptr";
      continue;
    }
    auto ptr = ExportedObject::lookupWeakObject(value);
    if (!ptr) {
      /*
      throw Exception("de.uni_stuttgart.Voxie.Error",
                               "Failed to look up node " + value.path());
      */
      qWarning() << "Failed to look up node" << value.path() << "for list";
      continue;
    }
    auto obj = dynamic_cast<Node*>(ptr);
    if (!obj) {
      /*
      throw Exception(
          "de.uni_stuttgart.Voxie.Error",
          "Path " + value.path() + " is not an vx::Node");
      */
      qWarning() << "Path" << value.path()
                 << "is not an vx::Node for node list";
      continue;
    }
    res.append(obj);
  }
  return res;
}

void Node::setStateToSetup() {
  vx::checkOnMainThread("Node::setStateToSetup");

  if (this->state_ == State::Initial) {
    vx::voxieRoot().registerNode(thisShared());
    this->state_ = State::Setup;
    Q_EMIT this->stateChanged(State::Setup);
  } else {
    qWarning() << "Invalid state for node in Node::setStateToSetup:"
               << stateToString(this->state_);
  }
}

void Node::setStateToNormal() {
  vx::checkOnMainThread("Node::setStateToNormal");

  if (this->state_ == State::Setup) {
    this->state_ = State::Normal;
    Q_EMIT this->stateChanged(State::Normal);
    this->setupFinished();
  } else {
    qWarning() << "Invalid state for node in Node::setStateToNormal:"
               << stateToString(this->state_);
  }
}

void Node::setupFinished() {}

QVariant Node::getVariant(Node* obj) {
  return QVariant::fromValue(obj ? obj->getPath() : QDBusObjectPath("/"));
}
QVariant Node::getVariant(const QList<Node*>& objs) {
  QList<QDBusObjectPath> ret;
  for (const auto& obj : objs) {
    if (obj) {
      ret.append(obj->getPath());
    } else {
      qWarning() << "Node list contains nullptr";
    }
  }
  return QVariant::fromValue(ret);
}

QMap<QString, QVariant> Node::propertyValues() {
  QMap<QString, QVariant> map;
  for (const auto& prop : prototype()->nodeProperties())
    map[prop->name()] = getNodeProperty(prop->name());
  return map;
}

QVariant Node::propertyDBusToRaw(NodePrototype* prototype,
                                 const QSharedPointer<NodeProperty>& property,
                                 const QDBusVariant& value) {
  // Support for loading old ValueColorMapping values (before
  // 19ad5739b10ba87d970663ad5c1e5902b01440ed)
  if (property->type() == vx::types::ValueColorMappingType() &&
      vx::dbusGetVariantSignature(value) == QDBusSignature("a(d(dddd))")) {
    auto val = dbusGetVariantValue<
        QList<std::tuple<double, vx::TupleVector<double, 4>>>>(value);
    QList<std::tuple<double, vx::TupleVector<double, 4>, qint32>> val2;
    for (const auto& entry : val)
      val2.push_back(
          std::make_tuple(std::get<0>(entry), std::get<1>(entry), 0));
    // return property->type()->dbusToRaw(dbusMakeVariant(val2));
    QList<vx::ColorizerEntry> entries;
    for (const auto& entry : val)
      entries << ColorizerEntry(std::get<0>(entry), std::get<1>(entry),
                                ColorInterpolator::RGB);
    return QVariant::fromValue(
        vx::PropertyValueConvertRaw<
            vx::types::ValueColorMapping::RawType,
            vx::types::ValueColorMapping::QtType>::toRaw(entries));
  }

  // Support for loading old CCL nodes (up until
  // f2ecd9434aaa0e65b4a0d2c3a228422e266c3bf6)
  if (prototype->name() == "de.uni_stuttgart.Voxie.CCL" &&
      property->name() == "de.uni_stuttgart.Voxie.Input" &&
      vx::dbusGetVariantSignature(value) == QDBusSignature("ao")) {
    auto val = dbusGetVariantValue<QList<QDBusObjectPath>>(value);

    QDBusObjectPath path("/");
    if (val.size() == 1) {
      path = val[0];
    } else if (val.size() != 0) {
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Got more than one input for CCL");
    }

    return QVariant::fromValue(path);
  }

  return property->type()->dbusToRaw(value);
}

QPointer<NodeGroup> Node::parentNodeGroup() const { return parentNodeGroup_; }

void Node::setParentNodeGroup(const QPointer<NodeGroup> group) {
  if (parentNodeGroup() != nullptr)
    disconnect(parentNodeGroup_.data(), &Node::stateChanged, this,
               &Node::parentNodeGroupStateChanged);

  parentNodeGroup_ = group;

  if (group != nullptr)
    connect(group.data(), &Node::stateChanged, this,
            &Node::parentNodeGroupStateChanged);
  Q_EMIT parentNodeGroupChanged(group.data());
}

QList<QSharedPointer<NodeProperty>> Node::exportedProperties() const {
  return exportedProperties_;
}

void Node::setExportedProperties(
    const QList<QSharedPointer<NodeProperty>>& properties) {
  exportedProperties_ = properties;
  exportedPropertiesChanged(properties);
  if (parentNodeGroup_) {
    parentNodeGroup_.data()->childrenExportedPropertiesChanged();
  }
}

void Node::parentNodeGroupStateChanged(State state) {
  if (state == Node::State::Teardown) {
    this->deleteLater();
  }
}

PropertiesEntryBase::PropertiesEntryBase(const QString& name,
                                         const QVariant& value)
    : name_(name), value_(value) {}
PropertiesEntryBase::~PropertiesEntryBase() {}
