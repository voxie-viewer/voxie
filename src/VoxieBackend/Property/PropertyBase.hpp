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

#include <VoxieBackend/VoxieBackend.hpp>

#include <VoxieClient/ObjectExport/ExportedObject.hpp>

#include <QtCore/QList>

namespace vx {
class PropertyType;
class ComponentContainer;

struct VOXIEBACKEND_EXPORT EnumEntry {
  QString name_;
  QString displayName_;
  int uiPosition_;
  QList<QString> compatibilityNames_;

  const QString& name() const { return name_; }
  const QString& displayName() const { return displayName_; }
  int uiPosition() const { return uiPosition_; }
  const QList<QString>& compatibilityNames() const {
    return compatibilityNames_;
  }
};

class VOXIEBACKEND_EXPORT PropertyBase : public vx::ExportedObject {
  Q_OBJECT

  QSharedPointer<vx::PropertyType> type_;
  QString _name;
  QString _displayName;
  QList<QString> compatibilityNames_;
  QString shortDescription_;

  QVariant defaultValue_;
  bool hasExplicitDefaultValue_;

  QList<EnumEntry> enumEntries_;

  // TODO: simply store the JSON instead of all properties?
  QSharedPointer<QJsonObject> rawJson_;

 public:
  /**
   * @brief Internal name used in DBus interface
   */
  const QString& name() const { return _name; }
  const QList<QString>& compatibilityNames() const {
    return compatibilityNames_;
  }
  /* human-readable name used fo displaying in the UI */
  const QString& displayName() const { return _displayName; }
  /**
   * @brief Short description, shown in tooltips.
   */
  const QString& shortDescription() const { return shortDescription_; }
  /* string representing the type of the field */
  const QSharedPointer<vx::PropertyType>& type() { return type_; }
  QString typeName();

  PropertyBase(const QString& name, const QJsonObject& data,
               const QSharedPointer<ComponentContainer>& propertyTypeContainer);

  const QList<EnumEntry>& enumEntries() { return enumEntries_; }

  /**
   * @brief Return a JSON value describing the object property
   */
  virtual QJsonValue toJson() = 0;

  const QJsonObject& rawJson() const { return *rawJson_; }

  /**
   * @brief Return the default value for the property.
   */
  const QVariant& defaultValue() const { return defaultValue_; }

  /**
   * @brief True if the default value for this property was set explicitly.
   */
  bool hasExplicitDefaultValue() const { return hasExplicitDefaultValue_; }
};
}  // namespace vx
