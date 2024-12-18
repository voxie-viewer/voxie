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

#include <VoxieBackend/Component/Component.hpp>
#include <VoxieBackend/Component/ComponentTypeInfo.hpp>

#include <QList>
#include <QMap>
#include <QtDBus/QDBusAbstractAdaptor>
#include <QtDBus/QDBusObjectPath>
#include <QtDBus/QDBusVariant>

#include <QtCore/QJsonValue>

class PropertyInstance;

namespace vx {
class Node;
class NodeProperty;

class PropertyUI;

class VOXIEBACKEND_EXPORT PropertyType : public vx::plugin::Component {
  Q_OBJECT

  QString name_;
  QString displayName_;
  QVariant defaultValue_;

 protected:
  PropertyType(const QString& name, const QString& displayName,
               const QVariant& defaultValue);
  ~PropertyType();

 public:
  /**
   * @brief Internal name used in DBus interface
   */
  const QString& name() const { return name_; }

  /**
   * @brief Name shown in GUI
   */
  const QString& displayName() const { return displayName_; }

  /**
   * @brief Return the value which is used as default value if it is not changed
   * by the property itself.
   */
  const QVariant& defaultValue() const { return defaultValue_; }

  /**
   * @brief Check whether value is a valid value for property and throw an
   * exception otherwise (property has to have this as type).
   * Non-canonical values are considered invalid.
   */
  virtual void verifyValue(NodeProperty& property, const QVariant& value) = 0;

  /**
   * @brief Check whether value is a valid value in a context where there is no
   * property.
   * Non-canonical values are considered invalid.
   */
  virtual void verifyValueWithoutProperty(const QVariant& value) = 0;

  /**
   * @brief Check whether value is a valid value for property and throw an
   * exception otherwise (property has to have this as type). Return the
   * canonical version of the property.
   */
  virtual QVariant canonicalize(NodeProperty& property,
                                const QVariant& value) = 0;

  /**
   * @brief Return the Qt meta type id for the type used in the (raw) variants
   * for storing the type.
   */
  virtual int getRawQMetaType() = 0;

  virtual PropertyUI* createUI(const QSharedPointer<NodeProperty>& property,
                               Node* node) = 0;

  virtual PropertyUI* createUISimple(
      const QSharedPointer<PropertyInstance>& propertyInstance);

  /**
   * @brief Parse the given JSON data value as a property of this type.
   *
   * Throws a Exception if the value cannot be parsed.
   */
  virtual QVariant parseJson(const QJsonValue& value) = 0;

  /**
   * @brief Convert the raw representation of the specified data type to a
   * human-readable string representation.
   *
   * Throws an exception if the type of the value contained within the passed
   * variant is not compatible with this PropertyType.
   *
   * Returns the type DisplayName if no string conversion function is defined
   * for this PropertyType.
   */
  virtual QString valueToString(const QVariant& value) = 0;

  /**
   * @brief Get a detailed description of the value as a human-readable string.
   *
   * Throws an exception if the type of the value contained within the passed
   * variant is not compatible with this PropertyType.
   *
   * Returns an empty string if no description is defined for this PropertyType.
   */
  virtual QString valueGetDescription(const QVariant& value) = 0;

  /**
   * @brief Convert the raw representation to the DBus representation.
   *
   * Throws a Exception if the value cannot be converted;
   */
  virtual QDBusVariant rawToDBus(const QVariant& value) = 0;
  /**
   * @brief Convert the DBus representation to the raw representation.
   *
   * Throws a Exception if the value cannot be converted;
   */
  virtual QVariant dbusToRaw(const QDBusVariant& value) = 0;

  // Same as rawToDBus and dbusToRaw, but with a list of values
  virtual QDBusVariant rawToDBusList(const QList<QVariant>& value) = 0;
  virtual QList<QVariant> dbusToRawList(const QDBusVariant& value) = 0;

  virtual QDBusSignature dbusSignature() = 0;

  /**
   * Returns true iff two values of this type can be compared with a total
   * ordering.
   */
  virtual bool isComparable() = 0;
  /**
   * Returns -1 if if v1 < v2, 0 if v1 == v2 and 1 if v1 > v2.
   *
   * v1 and v2 must be raw values of this type.
   *
   * Throws an exception is isComparable() returns false.
   */
  virtual int compare(const QVariant& v1, const QVariant& v2) = 0;

  QList<QString> supportedComponentDBusInterfaces() override;
};

template <>
struct ComponentTypeInfo<PropertyType> {
  static const char* name() {
    return "de.uni_stuttgart.Voxie.ComponentType.PropertyType";
  }
  static const QList<std::tuple<QString, bool>> compatibilityNames() {
    return {
        std::make_tuple("de.uni_stuttgart.Voxie.PropertyType", true),
    };
  }
};
}  // namespace vx
