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

#include <VoxieBackend/Property/PropertyBase.hpp>

#include <VoxieClient/SharedFunPtr.hpp>

#include <VoxieClient/ObjectExport/ExportedObject.hpp>

#include <Voxie/Node/NodeKind.hpp>
#include <Voxie/Node/NodeTag.hpp>

#include <QList>
#include <QMap>
#include <QtDBus/QDBusAbstractAdaptor>
#include <QtDBus/QDBusObjectPath>
#include <QtDBus/QDBusVariant>

namespace vx {
class Node;
class NodePrototype;
class NodeProperty;
}  // namespace vx

// TODO: Move PropertyCondition* to separate file
class VOXIECORESHARED_EXPORT PropertyCondition {
 public:
  PropertyCondition();
  virtual ~PropertyCondition();

  virtual bool evaluate(vx::Node* obj) = 0;
  virtual void collectDependencies(QSet<vx::NodeProperty*>& list) = 0;
  virtual QJsonObject toJson() = 0;

  static QSharedPointer<PropertyCondition> parse(
      const QSharedPointer<vx::NodePrototype>& prototype,
      const QJsonObject& json);
};

class VOXIECORESHARED_EXPORT PropertyConditionTrue : public PropertyCondition {
 public:
  PropertyConditionTrue();
  ~PropertyConditionTrue();

  bool evaluate(vx::Node* obj) override;
  void collectDependencies(QSet<vx::NodeProperty*>& list) override;
  QJsonObject toJson() override;

  static QSharedPointer<PropertyConditionTrue> parseChild(
      const QSharedPointer<vx::NodePrototype>& prototype,
      const QJsonObject& json);
};

class VOXIECORESHARED_EXPORT PropertyConditionNot : public PropertyCondition {
  QSharedPointer<PropertyCondition> condition_;

 public:
  PropertyConditionNot(const QSharedPointer<PropertyCondition>& condition);
  ~PropertyConditionNot();

  bool evaluate(vx::Node* obj) override;
  void collectDependencies(QSet<vx::NodeProperty*>& list) override;
  QJsonObject toJson() override;

  const QSharedPointer<PropertyCondition>& condition() const {
    return condition_;
  }

  static QSharedPointer<PropertyConditionNot> parseChild(
      const QSharedPointer<vx::NodePrototype>& prototype,
      const QJsonObject& json);
};

class VOXIECORESHARED_EXPORT PropertyConditionAnd : public PropertyCondition {
  QList<QSharedPointer<PropertyCondition>> conditions_;

 public:
  PropertyConditionAnd(
      const QList<QSharedPointer<PropertyCondition>>& conditions);
  ~PropertyConditionAnd();

  bool evaluate(vx::Node* obj) override;
  void collectDependencies(QSet<vx::NodeProperty*>& list) override;
  QJsonObject toJson() override;

  const QList<QSharedPointer<PropertyCondition>>& conditions() const {
    return conditions_;
  }

  static QSharedPointer<PropertyConditionAnd> parseChild(
      const QSharedPointer<vx::NodePrototype>& prototype,
      const QJsonObject& json);
};

class VOXIECORESHARED_EXPORT PropertyConditionOr : public PropertyCondition {
  QList<QSharedPointer<PropertyCondition>> conditions_;

 public:
  PropertyConditionOr(
      const QList<QSharedPointer<PropertyCondition>>& conditions);
  ~PropertyConditionOr();

  bool evaluate(vx::Node* obj) override;
  void collectDependencies(QSet<vx::NodeProperty*>& list) override;
  QJsonObject toJson() override;

  const QList<QSharedPointer<PropertyCondition>>& conditions() const {
    return conditions_;
  }

  static QSharedPointer<PropertyConditionOr> parseChild(
      const QSharedPointer<vx::NodePrototype>& prototype,
      const QJsonObject& json);
};

class VOXIECORESHARED_EXPORT PropertyConditionIsEmpty
    : public PropertyCondition {
  QWeakPointer<vx::NodeProperty> property_;

 public:
  PropertyConditionIsEmpty(const QWeakPointer<vx::NodeProperty>& property);
  ~PropertyConditionIsEmpty();

  bool evaluate(vx::Node* obj) override;
  void collectDependencies(QSet<vx::NodeProperty*>& list) override;
  QJsonObject toJson() override;

  // TODO: return QSharedPointer<>?
  const QWeakPointer<vx::NodeProperty>& property() const { return property_; }

  static QSharedPointer<PropertyConditionIsEmpty> parseChild(
      const QSharedPointer<vx::NodePrototype>& prototype,
      const QJsonObject& json);
};

class VOXIECORESHARED_EXPORT PropertyConditionHasValue
    : public PropertyCondition {
  QWeakPointer<vx::NodeProperty> property_;
  QList<QVariant> values_;

 public:
  PropertyConditionHasValue(const QWeakPointer<vx::NodeProperty>& property,
                            const QList<QVariant>& values);
  ~PropertyConditionHasValue();

  bool evaluate(vx::Node* obj) override;
  void collectDependencies(QSet<vx::NodeProperty*>& list) override;
  QJsonObject toJson() override;

  // TODO: return QSharedPointer<>?
  const QWeakPointer<vx::NodeProperty>& property() const { return property_; }
  const QList<QVariant>& values() const { return values_; }

  static QSharedPointer<PropertyConditionHasValue> parseChild(
      const QSharedPointer<vx::NodePrototype>& prototype,
      const QJsonObject& json);
};

namespace vx {
class VOXIECORESHARED_EXPORT NodeProperty : public vx::PropertyBase {
  Q_OBJECT

  bool isCustomStorage_;
  bool isCustomUI_;
  bool isReference_;
  bool isOutput_;  // only valid if isReference_ is true

  QList<QSharedPointer<vx::NodeTag>> inTags_;
  QList<QSharedPointer<vx::NodeTag>> outTags_;
  QList<vx::NodeKind> allowedKinds_;

  int uiPosition_;
  int callSetOrder_;

  QSharedPointer<PropertyCondition> enabledCondition_;

  bool isDouble_;
  bool isInt_;
  bool hasMinimum_;
  double doubleMinimum_;
  qint64 intMinimum_;
  bool hasMaximum_;
  double doubleMaximum_;
  qint64 intMaximum_;
  QStringList patterns_;
  bool hasPatterns_;

 public:
  QList<QWeakPointer<vx::NodePrototype>> _allowedTypes;

  QList<QSharedPointer<vx::NodeTag>>& inTags() { return inTags_; }
  QList<QSharedPointer<vx::NodeTag>>& outTags() { return outTags_; }

  NodeProperty(
      const QString& name, const QJsonObject& data, bool allowCustom,
      const QSharedPointer<ComponentContainer>& components,
      QList<vx::SharedFunPtr<void(const QSharedPointer<vx::NodePrototype>&)>>&
          finishActions,
      QList<vx::SharedFunPtr<void(const QSharedPointer<ComponentContainer>&)>>&
          resolveActions);

  /**
   * @brief Return a JSON value describing the node property
   */
  QJsonValue toJson() override;

  /**
   * @brief Return whether the property points to one or more other nodes.
   */
  bool isReference() { return isReference_; }

  /**
   * @brief Return whether this is an output reference. Throws an exception when
   * isReference() is false.
   */
  bool isOutputReference() {
    if (!isReference())
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "isReference() is false");
    return isOutput_;
  }

  /**
   * @brief Return whether the property is stored by a
   * getNodePropertyCustom()/setNodePropertyCustom() override
   */
  bool isCustomStorage() { return isCustomStorage_; }

  /**
   * @brief Return whether the property has custom code for displaying it in the
   * UI
   */
  bool isCustomUI() { return isCustomUI_; }

  /**
   * @brief For property types which refer to other nodes: A list of allowed
   * node types. An node is allowed if its type is listed in allowedTypes()
   * or its kind is listed in allowedKinds().
   */
  const QList<QWeakPointer<vx::NodePrototype>>& allowedTypes() {
    return _allowedTypes;
  }

  /**
   * @brief For property types which refer to other nodes: A list of allowed
   * node kinds. An node is allowed if its type is listed in allowedTypes()
   * or its kind is listed in allowedKinds().
   */
  const QList<vx::NodeKind>& allowedKinds() { return allowedKinds_; }

  int uiPosition() { return uiPosition_; }
  int callSetOrder() { return callSetOrder_; }

  const QSharedPointer<PropertyCondition>& enabledCondition() {
    return enabledCondition_;
  }

  bool hasMinimum() { return hasMinimum_; }
  bool hasMaximum() { return hasMaximum_; }
  bool hasPatterns() { return hasPatterns_; }
  double doubleMinimum();
  qint64 intMinimum();
  double doubleMaximum();
  qint64 intMaximum();
  QStringList patterns();

  /**
   * @brief Return whether obj is allowed as a value for the property. Throws an
   * exception when the property is not a reference property.
   *
   * Currently this is equivalent to allowsAsValue(obj->prototype()).
   */
  bool allowsAsValue(vx::Node* obj);

  /**
   * @brief Return whether nodes with type prototype are allowed as a value
   * for the property. Throws an exception when the property is not a reference
   * property.
   */
  bool allowsAsValue(vx::NodePrototype* childPrototype);
  bool allowsAsValue(const QSharedPointer<vx::NodePrototype>& childPrototype);
};
}  // namespace vx
