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

#include "NodeProperty.hpp"

#include <VoxieClient/DBusAdaptors.hpp>
#include <VoxieClient/Exception.hpp>
#include <VoxieClient/JsonDBus.hpp>

#include <VoxieBackend/Component/ComponentContainer.hpp>

#include <Voxie/IVoxie.hpp>

#include <Voxie/Node/NodePrototype.hpp>
#include <Voxie/Node/PropertyValueConvertRaw.hpp>
#include <Voxie/Node/Types.hpp>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>

#include <QtCore/QPointer>

#include <QtGui/QQuaternion>

using namespace vx;

PropertyCondition::PropertyCondition() {}
PropertyCondition::~PropertyCondition() {}

PropertyConditionTrue::PropertyConditionTrue() {}
PropertyConditionTrue::~PropertyConditionTrue() {}

QSharedPointer<PropertyCondition> PropertyCondition::parse(
    const QSharedPointer<vx::NodePrototype>& prototype,
    const QJsonObject& json) {
  auto type = json["Type"].toString();
  if (type == "de.uni_stuttgart.Voxie.PropertyCondition.True")
    return PropertyConditionTrue::parseChild(prototype, json);
  else if (type == "de.uni_stuttgart.Voxie.PropertyCondition.Not")
    return PropertyConditionNot::parseChild(prototype, json);
  else if (type == "de.uni_stuttgart.Voxie.PropertyCondition.And")
    return PropertyConditionAnd::parseChild(prototype, json);
  else if (type == "de.uni_stuttgart.Voxie.PropertyCondition.Or")
    return PropertyConditionOr::parseChild(prototype, json);
  else if (type == "de.uni_stuttgart.Voxie.PropertyCondition.IsEmpty")
    return PropertyConditionIsEmpty::parseChild(prototype, json);
  else if (type == "de.uni_stuttgart.Voxie.PropertyCondition.HasValue")
    return PropertyConditionHasValue::parseChild(prototype, json);
  else
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Unknown property condition type: '" + type + "'");
}

bool PropertyConditionTrue::evaluate(vx::Node* obj) {
  Q_UNUSED(obj);
  return true;
}
void PropertyConditionTrue::collectDependencies(QSet<NodeProperty*>& list) {
  Q_UNUSED(list);
}
QJsonObject PropertyConditionTrue::toJson() {
  QJsonObject obj;
  obj["Type"] = "de.uni_stuttgart.Voxie.PropertyCondition.True";
  return obj;
}

QSharedPointer<PropertyConditionTrue> PropertyConditionTrue::parseChild(
    const QSharedPointer<vx::NodePrototype>& prototype,
    const QJsonObject& json) {
  Q_UNUSED(prototype);
  Q_UNUSED(json);
  return createQSharedPointer<PropertyConditionTrue>();
}

PropertyConditionNot::PropertyConditionNot(
    const QSharedPointer<PropertyCondition>& condition)
    : condition_(condition) {
  if (!condition_)
    throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                        "condition == nullptr");
}
PropertyConditionNot::~PropertyConditionNot() {}
bool PropertyConditionNot::evaluate(vx::Node* obj) {
  return !this->condition()->evaluate(obj);
}
void PropertyConditionNot::collectDependencies(QSet<NodeProperty*>& list) {
  this->condition()->collectDependencies(list);
}
QJsonObject PropertyConditionNot::toJson() {
  QJsonObject obj;
  obj["Type"] = "de.uni_stuttgart.Voxie.PropertyCondition.Not";
  obj["Condition"] = this->condition()->toJson();
  return obj;
}
QSharedPointer<PropertyConditionNot> PropertyConditionNot::parseChild(
    const QSharedPointer<vx::NodePrototype>& prototype,
    const QJsonObject& json) {
  auto condition = parse(prototype, json["Condition"].toObject());
  return createQSharedPointer<PropertyConditionNot>(condition);
}

PropertyConditionAnd::PropertyConditionAnd(
    const QList<QSharedPointer<PropertyCondition>>& conditions)
    : conditions_(conditions) {
  for (const auto& condition : conditions_)
    if (!condition)
      throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                          "conditions contains nullptr");
}
PropertyConditionAnd::~PropertyConditionAnd() {}
bool PropertyConditionAnd::evaluate(vx::Node* obj) {
  for (const auto& condition : this->conditions())
    if (!condition->evaluate(obj)) return false;
  return true;
}
void PropertyConditionAnd::collectDependencies(QSet<NodeProperty*>& list) {
  for (const auto& condition : this->conditions())
    condition->collectDependencies(list);
}
QJsonObject PropertyConditionAnd::toJson() {
  QJsonObject obj;
  obj["Type"] = "de.uni_stuttgart.Voxie.PropertyCondition.And";
  QJsonArray array;
  for (const auto& condition : this->conditions()) array << condition->toJson();
  obj["Conditions"] = array;
  return obj;
}
QSharedPointer<PropertyConditionAnd> PropertyConditionAnd::parseChild(
    const QSharedPointer<vx::NodePrototype>& prototype,
    const QJsonObject& json) {
  QList<QSharedPointer<PropertyCondition>> conditions;
  for (const auto& child : json["Conditions"].toArray())
    conditions << parse(prototype, child.toObject());
  return createQSharedPointer<PropertyConditionAnd>(conditions);
}

PropertyConditionOr::PropertyConditionOr(
    const QList<QSharedPointer<PropertyCondition>>& conditions)
    : conditions_(conditions) {
  for (const auto& condition : conditions_)
    if (!condition)
      throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                          "conditions contains nullptr");
}
PropertyConditionOr::~PropertyConditionOr() {}
bool PropertyConditionOr::evaluate(vx::Node* obj) {
  for (const auto& condition : this->conditions())
    if (condition->evaluate(obj)) return true;
  return false;
}
void PropertyConditionOr::collectDependencies(QSet<NodeProperty*>& list) {
  for (const auto& condition : this->conditions())
    condition->collectDependencies(list);
}
QJsonObject PropertyConditionOr::toJson() {
  QJsonObject obj;
  obj["Type"] = "de.uni_stuttgart.Voxie.PropertyCondition.Or";
  QJsonArray array;
  for (const auto& condition : this->conditions()) array << condition->toJson();
  obj["Conditions"] = array;
  return obj;
}
QSharedPointer<PropertyConditionOr> PropertyConditionOr::parseChild(
    const QSharedPointer<vx::NodePrototype>& prototype,
    const QJsonObject& json) {
  QList<QSharedPointer<PropertyCondition>> conditions;
  for (const auto& child : json["Conditions"].toArray())
    conditions << parse(prototype, child.toObject());
  return createQSharedPointer<PropertyConditionOr>(conditions);
}

PropertyConditionIsEmpty::PropertyConditionIsEmpty(
    const QWeakPointer<NodeProperty>& property)
    : property_(property) {
  auto prop = this->property().lock();
  if (!prop)
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.InternalError",
        "PropertyConditionIsEmpty constructor called with destroyed property");

  if (prop->type() != vx::types::NodeReferenceType() &&
      prop->type() != vx::types::OutputNodeReferenceType() &&
      prop->type() != vx::types::NodeReferenceListType() &&
      prop->type() != vx::types::IntListType() &&
      prop->type() != vx::types::LabelListType())

    throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                        "Unknown property type for PropertyConditionIsEmpty");
}
PropertyConditionIsEmpty::~PropertyConditionIsEmpty() {}
bool PropertyConditionIsEmpty::evaluate(vx::Node* obj) {
  auto prop = this->property().lock();
  if (!prop) {
    qWarning() << "PropertyConditionIsEmpty::evaluate() called with destroyed "
                  "property";
    return false;
  }
  auto value = obj->getNodeProperty(prop);
  if (prop->type() == vx::types::NodeReferenceType() ||
      prop->type() == vx::types::OutputNodeReferenceType()) {
    auto val = vx::Node::parseVariantNode(value);
    return val == nullptr;
  } else if (prop->type() == vx::types::NodeReferenceListType()) {
    auto val = vx::Node::parseVariantNodeList(value);
    return val.size() == 0;
  } else if (prop->type() == vx::types::IntListType()) {
    auto val = vx::Node::parseVariant<QList<qint64>>(value);
    return val.size() == 0;
  } else if (prop->type() == vx::types::LabelListType()) {
    auto val = vx::Node::parseVariant<QList<quint64>>(value);
    return val.size() == 0;
  } else {
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.InternalError",
        "Unknown property type in PropertyConditionIsEmpty::evaluate");
  }
}
void PropertyConditionIsEmpty::collectDependencies(QSet<NodeProperty*>& list) {
  auto prop = this->property().lock();
  if (!prop) {
    qWarning() << "PropertyConditionIsEmpty::collectDependencies() called "
                  "with destroyed property";
    return;
  }
  list.insert(prop.data());
}
QJsonObject PropertyConditionIsEmpty::toJson() {
  auto prop = this->property().lock();
  if (!prop) {
    qWarning()
        << "PropertyConditionIsEmpty::toJson() called with destroyed property";
    return QJsonObject();
  }
  QJsonObject obj;
  obj["Type"] = "de.uni_stuttgart.Voxie.PropertyCondition.IsEmpty";
  obj["Property"] = prop->name();
  return obj;
}
QSharedPointer<PropertyConditionIsEmpty> PropertyConditionIsEmpty::parseChild(
    const QSharedPointer<vx::NodePrototype>& prototype,
    const QJsonObject& json) {
  auto properties = prototype->nodeProperties();
  auto propName = json["Property"].toString();
  QSharedPointer<NodeProperty> prop = QSharedPointer<NodeProperty>();
  for (const auto& p : properties)
    if (p->name() == propName) prop = p;
  // if (!properties.contains(propName))
  if (!prop)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Unable to find property with name '" + propName +
                            "' in prototype '" + prototype->name() + "'");
  // auto prop = properties[propName];
  return createQSharedPointer<PropertyConditionIsEmpty>(prop);
}

PropertyConditionHasValue::PropertyConditionHasValue(
    const QWeakPointer<NodeProperty>& property, const QList<QVariant>& values)
    : property_(property), values_(values) {}
PropertyConditionHasValue::~PropertyConditionHasValue() {}
bool PropertyConditionHasValue::evaluate(vx::Node* obj) {
  auto prop = this->property().lock();
  if (!prop) {
    qWarning() << "PropertyConditionHasValue::evaluate() called with destroyed "
                  "property";
    return false;
  }
  auto value = obj->getNodeProperty(prop);
  // qDebug() << "Eval" << this->values().count();
  for (const auto& val : this->values()) {
    // qDebug() << "Compare" << val << value;
    if (val == value)  // TODO implement different equality?
      return true;
  }
  return false;
}
void PropertyConditionHasValue::collectDependencies(QSet<NodeProperty*>& list) {
  auto prop = this->property().lock();
  if (!prop) {
    qWarning() << "PropertyConditionHasValue::collectDependencies() called "
                  "with destroyed property";
    return;
  }
  list.insert(prop.data());
}
QJsonObject PropertyConditionHasValue::toJson() {
  auto prop = this->property().lock();
  if (!prop) {
    qWarning()
        << "PropertyConditionHasValue::toJson() called with destroyed property";
    return QJsonObject();
  }
  QJsonObject obj;
  obj["Type"] = "de.uni_stuttgart.Voxie.PropertyCondition.HasValue";
  obj["Property"] = prop->name();
  QJsonArray values;
  for (const auto& val : this->values()) {
    // TODO: Serialize values here
    // values << ...(val);
    Q_UNUSED(val);
  }
  obj["Values"] = values;
  return obj;
}
QSharedPointer<PropertyConditionHasValue> PropertyConditionHasValue::parseChild(
    const QSharedPointer<vx::NodePrototype>& prototype,
    const QJsonObject& json) {
  auto properties = prototype->nodeProperties();
  auto propName = json["Property"].toString();
  QSharedPointer<NodeProperty> prop = QSharedPointer<NodeProperty>();
  for (const auto& p : properties)
    if (p->name() == propName) prop = p;
  // if (!properties.contains(propName))
  if (!prop)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Unable to find property with name '" + propName +
                            "' in prototype '" + prototype->name() + "'");
  // auto prop = properties[propName];
  QList<QVariant> values;
  for (const auto& value : json["Values"].toArray()) {
    values << prop->type()->parseJson(value);
  }
  return createQSharedPointer<PropertyConditionHasValue>(prop, values);
}

// TODO: Stuff from rawJson?
QJsonValue NodeProperty::toJson() {
  QJsonObject obj;
  obj["DisplayName"] = this->displayName();
  if (shortDescription() != "") obj["ShortDescription"] = shortDescription();
  obj["Type"] = type()->name();
  if (isCustomStorage()) obj["IsCustomStorage"] = true;
  if (isCustomUI()) obj["IsCustomUI"] = true;
  if (isReference()) {
    QJsonArray allowed;
    for (const auto& proto : allowedTypes()) {
      auto protoStrong = proto.toStrongRef();
      if (!protoStrong)
        throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                            "Prototype has already been destroyed");
      allowed << protoStrong->name();
    }

    if (allowed.count()) obj["AllowedNodePrototypes"] = allowed;

    QJsonArray allowedKinds;
    for (const auto& kind : this->allowedKinds())
      allowedKinds << nodeKindToString(kind);
    if (allowedKinds.count()) obj["AllowedNodeKinds"] = allowedKinds;
  }
  if (enumEntries().count()) {
    QJsonObject values;
    for (const auto& value : enumEntries()) {
      QJsonObject enumObj;
      enumObj["DisplayName"] = value.displayName();
      if (value.uiPosition() != -1) enumObj["UIPosition"] = value.uiPosition();
      if (value.compatibilityNames().size() != 0) {
        QJsonArray array;
        for (const auto& name : value.compatibilityNames()) array << name;
        enumObj["CompatibilityNames"] = array;
      }
      values[value.name()] = enumObj;
    }
    obj["EnumEntries"] = values;
  }

  // TODO Serialize DefaultValue

  if (hasMinimum()) {
    if (isDouble_)
      obj["MinimumValue"] = doubleMinimum();
    else if (isInt_)
      obj["MinimumValue"] = intMinimum();
    else
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Unknown type with minimum");
  }
  if (hasMaximum()) {
    if (isDouble_)
      obj["MaximumValue"] = doubleMaximum();
    else if (isInt_)
      obj["MaximumValue"] = intMaximum();
    else
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Unknown type with maximum");
  }

  if (uiPosition() != -1) obj["UIPosition"] = uiPosition();
  if (callSetOrder() != 0) obj["CallSetOrder"] = callSetOrder();
  if (hasPatterns()) obj["Patterns"] = QJsonArray::fromStringList(patterns());
  return obj;
}

double NodeProperty::doubleMinimum() {
  if (!isDouble_)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Not a double property");
  if (!hasMinimum())
    throw vx::Exception("de.uni_stuttgart.Voxie.Error", "No minimum value set");
  return doubleMinimum_;
}
qint64 NodeProperty::intMinimum() {
  if (!isInt_)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error", "Not a int property");
  if (!hasMinimum())
    throw vx::Exception("de.uni_stuttgart.Voxie.Error", "No minimum value set");
  return intMinimum_;
}
double NodeProperty::doubleMaximum() {
  if (!isDouble_)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Not a double property");
  if (!hasMaximum())
    throw vx::Exception("de.uni_stuttgart.Voxie.Error", "No maximum value set");
  return doubleMaximum_;
}
qint64 NodeProperty::intMaximum() {
  if (!isInt_)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error", "Not a int property");
  if (!hasMaximum())
    throw vx::Exception("de.uni_stuttgart.Voxie.Error", "No maximum value set");
  return intMaximum_;
}

QStringList NodeProperty::patterns() {
  if (!hasPatterns())
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Not a file name property");
  return patterns_;
}

bool NodeProperty::allowsAsValue(vx::Node* obj) {
  if (!obj)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error", "Got nullptr obj");
  return allowsAsValue(obj->prototype());
}

bool NodeProperty::allowsAsValue(
    const QSharedPointer<vx::NodePrototype>& childPrototype) {
  return allowsAsValue(childPrototype.data());
}
bool NodeProperty::allowsAsValue(vx::NodePrototype* childPrototype) {
  if (!childPrototype)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Got nullptr childPrototype");
  if (!isReference())
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Not a reference property");

  for (const auto& kind : allowedKinds()) {
    if (childPrototype->nodeKind() == kind) return true;
  }
  for (const auto& prototype : allowedTypes()) {
    auto prototypeStrong = prototype.lock();
    if (!prototypeStrong) continue;
    if (prototypeStrong != childPrototype) continue;
    return true;
  }

  return false;
}

class NodePropertyAdaptorImpl : public NodePropertyAdaptor,
                                public ObjectPropertyAdaptor {
  NodeProperty* object;

 public:
  NodePropertyAdaptorImpl(NodeProperty* object)
      : NodePropertyAdaptor(object),
        ObjectPropertyAdaptor(object),
        object(object) {}
  virtual ~NodePropertyAdaptorImpl() {}
};

NodeProperty::NodeProperty(
    const QString& name, const QJsonObject& data, bool allowCustom,
    const QSharedPointer<ComponentContainer>& components,
    QList<vx::SharedFunPtr<void(const QSharedPointer<vx::NodePrototype>&)>>&
        finishActions,
    QList<vx::SharedFunPtr<void(const QSharedPointer<ComponentContainer>&)>>&
        resolveActions)
    : PropertyBase(name, data, components) {
  isCustomStorage_ = data.contains("IsCustomStorage")
                         ? data["IsCustomStorage"].toBool()
                         : false;
  isCustomUI_ =
      data.contains("IsCustomUI") ? data["IsCustomUI"].toBool() : false;
  if (!allowCustom && (isCustomStorage_ || isCustomUI_))
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Custom properties not allowed here");
  if (type() == vx::types::NodeReferenceType() ||
      type() == vx::types::NodeReferenceListType() ||
      type() == vx::types::OutputNodeReferenceType()) {
    this->isReference_ = true;
    this->isOutput_ = type() == vx::types::OutputNodeReferenceType();
    if (isCustomStorage_)
      throw vx::Exception(
          "de.uni_stuttgart.Voxie.Error",
          "Custom property storage not supported for object references");

    if (data.contains("AllowedObjectTypes")) {
      if (Node::showWarningOnOldObjectNames())
        finishActions <<
            [name](const QSharedPointer<vx::NodePrototype>& prototype) {
              qWarning() << "Got 'AllowedObjectTypes' value on property" << name
                         << "of prototype" << prototype->name();
            };
      auto types = data["AllowedObjectTypes"].toArray();
      resolveActions <<
          [types, this](const QSharedPointer<ComponentContainer>& components2) {
            for (const auto& typeVal : types) {
              auto type = typeVal.toString();
              _allowedTypes
                  << components2->getComponentTyped<NodePrototype>(type, true);
            }
          };
    }
    auto types = data["AllowedNodePrototypes"].toArray();
    resolveActions <<
        [types, this](const QSharedPointer<ComponentContainer>& components2) {
          for (const auto& typeVal : types) {
            auto type = typeVal.toString();
            _allowedTypes << components2->getComponentTyped<NodePrototype>(
                type, true);
          }
        };

    if (data.contains("AllowedObjectKinds")) {
      if (Node::showWarningOnOldObjectNames())
        finishActions <<
            [name](const QSharedPointer<vx::NodePrototype>& prototype) {
              qWarning() << "Got 'AllowedObjectKinds' value on property" << name
                         << "of prototype" << prototype->name();
            };
      for (const auto& kindVal : data["AllowedObjectKinds"].toArray())
        allowedKinds_.append(vx::parseObjectKind(kindVal.toString()));
    }
    for (const auto& kindVal : data["AllowedNodeKinds"].toArray())
      allowedKinds_.append(vx::parseNodeKind(kindVal.toString()));

    if (data.contains("EnumEntries"))
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "EnumEntries not allowed here");
  } else {
    this->isReference_ = false;
    this->isOutput_ = false;
    if (data.contains("AllowedObjectTypes"))
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "AllowedObjectTypes not allowed here");
    if (data.contains("AllowedObjectKinds"))
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "AllowedObjectKinds not allowed here");
  }

  if (data.contains("RequiredTags") || data.contains("OutputTags")) {
    if (!isReference_) throw;
    QJsonArray tags;
    if (data.contains("RequiredTags")) {
      if (isOutput_)
        throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                            "RequiredTags not allowed for Output");
      tags = data["RequiredTags"].toArray();
    } else {
      if (!isOutput_)
        throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                            "OutputTags not allowed for Input");
      tags = data["OutputTags"].toArray();
    }
    for (int i = 0; i < tags.count(); i++) {
      if (!vx::NodeTag::exist(tags[i].toString())) {
        throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                            "Tag " + tags[i].toString() + " does not exist");
      }
      if (isOutput_) {
        outTags_ << vx::NodeTag::getTag(tags[i].toString());
      } else {
        inTags_ << vx::NodeTag::getTag(tags[i].toString());
      }
    }
  }
  if (type() == vx::types::FileNameType()) {
    hasPatterns_ = true;
    if (data.contains("Patterns")) {
      for (const auto& val : data["Patterns"].toArray()) {
        patterns_ << val.toString();
      }
    } else {
      patterns_ = QStringList({"All Files (*.*)"});
    }
  } else {
    hasPatterns_ = false;
  }

  if (data.contains("UIPosition"))
    uiPosition_ = data["UIPosition"].toInt();
  else
    uiPosition_ = -1;
  if (data.contains("CallSetOrder"))
    callSetOrder_ = data["CallSetOrder"].toInt();
  else
    callSetOrder_ = 0;

  if (data.contains("EnabledCondition")) {
    auto conditionJson = data["EnabledCondition"].toObject();
    finishActions << [this, conditionJson](
                         const QSharedPointer<vx::NodePrototype>& prototype) {
      enabledCondition_ = PropertyCondition::parse(prototype, conditionJson);
    };
  } else {
    enabledCondition_ = createQSharedPointer<PropertyConditionTrue>();
  }

  if (type() == vx::types::FloatType()) {
    isDouble_ = true;
    isInt_ = false;
    if (data.contains("MinimumValue")) {
      if (data["MinimumValue"].type() != QJsonValue::Double)
        throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                            "Expected MinimumValue to be a number");
      hasMinimum_ = true;
      doubleMinimum_ = data["MinimumValue"].toDouble();
    } else {
      hasMinimum_ = false;
      doubleMinimum_ = 0.0;
    }
    if (data.contains("MaximumValue")) {
      if (data["MaximumValue"].type() != QJsonValue::Double)
        throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                            "Expected MaximumValue to be a number");
      hasMaximum_ = true;
      doubleMaximum_ = data["MaximumValue"].toDouble();
    } else {
      hasMaximum_ = false;
      doubleMaximum_ = 0.0;
    }
    intMinimum_ = 0;
    intMaximum_ = 0;
  } else if (type() == vx::types::IntType()) {
    isDouble_ = false;
    isInt_ = true;
    if (data.contains("MinimumValue")) {
      if (data["MinimumValue"].type() != QJsonValue::Double)
        throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                            "Expected MinimumValue to be a number");
      hasMinimum_ = true;
      intMinimum_ = (qint64)data["MinimumValue"].toDouble();
    } else {
      hasMinimum_ = false;
      intMinimum_ = 0.0;
    }
    if (data.contains("MaximumValue")) {
      if (data["MaximumValue"].type() != QJsonValue::Double)
        throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                            "Expected MaximumValue to be a number");
      hasMaximum_ = true;
      intMaximum_ = (qint64)data["MaximumValue"].toDouble();
    } else {
      hasMaximum_ = false;
      intMaximum_ = 0.0;
    }
    doubleMinimum_ = 0;
    doubleMaximum_ = 0;
  } else {
    if (data.contains("MinimumValue"))
      throw vx::Exception(
          "de.uni_stuttgart.Voxie.Error",
          "MinimumValue not allowed for non-numeric properties");
    if (data.contains("MaximumValue"))
      throw vx::Exception(
          "de.uni_stuttgart.Voxie.Error",
          "MaximumValue not allowed for non-numeric properties");
    isDouble_ = false;
    isInt_ = false;
    hasMinimum_ = false;
    doubleMinimum_ = 0.0;
    intMinimum_ = 0;
    hasMaximum_ = false;
    doubleMaximum_ = 0.0;
    intMaximum_ = 0;
  }

  new NodePropertyAdaptorImpl(this);
}
