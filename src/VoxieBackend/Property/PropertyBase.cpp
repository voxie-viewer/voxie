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

#include "PropertyBase.hpp"

#include <VoxieBackend/Component/ComponentContainer.hpp>

#include <VoxieBackend/Property/PropertyType.hpp>

#include <VoxieClient/DBusAdaptors.hpp>
#include <VoxieClient/JsonDBus.hpp>

using namespace vx;

class PropertyAdaptorImpl : public PropertyAdaptor {
  Q_OBJECT

  PropertyBase* object;

 public:
  PropertyAdaptorImpl(PropertyBase* object)
      : PropertyAdaptor(object), object(object) {}
  virtual ~PropertyAdaptorImpl() {}

  QString name() const override { return object->name(); }

  QString displayName() const override { return object->displayName(); }

  QDBusObjectPath type() const override {
    return vx::ExportedObject::getPath(object->type());
  }

  QDBusVariant propertyDefinition() const override {
    return vx::jsonToDBus(object->toJson());
  }
};

PropertyBase::PropertyBase(
    const QString& propertyName, const QJsonObject& data,
    const QSharedPointer<ComponentContainer>& propertyTypeContainer)
    : RefCountedObject("PropertyBase") {
  _name = propertyName;
  auto compatibilityNames = data["CompatibilityNames"].toArray();
  for (const auto& compatName : compatibilityNames)
    this->compatibilityNames_ << compatName.toString();

  _displayName = data["DisplayName"].toString();
  if (data.contains("ShortDescription"))
    shortDescription_ = data["ShortDescription"].toString();
  else
    shortDescription_ = "";
  auto typeStr = data["Type"].toString();
  type_ = propertyTypeContainer->getComponentTyped<PropertyType>(typeStr, true);
  hasExplicitDefaultValue_ = data.contains("DefaultValue");
  defaultValue_ = hasExplicitDefaultValue_
                      ? type_->parseJson(data["DefaultValue"])
                      : type_->defaultValue();

  if (data.contains("EnumEntries")) {
    auto entries = data["EnumEntries"].toObject();
    for (const auto& entryName : entries.keys()) {
      auto valueObj = entries[entryName].toObject();
      EnumEntry entry;
      entry.name_ = entryName;
      entry.displayName_ = valueObj["DisplayName"].toString();
      if (valueObj.contains("UIPosition"))
        entry.uiPosition_ = valueObj["UIPosition"].toInt();
      else
        entry.uiPosition_ = -1;
      for (const auto& compatName : valueObj["CompatibilityNames"].toArray())
        entry.compatibilityNames_ << compatName.toString();
      enumEntries_ << entry;
    }
    std::sort(enumEntries_.begin(), enumEntries_.end(),
              [](const EnumEntry& e1, const EnumEntry& e2) {
                return e1.name() < e2.name();
              });
  }

  rawJson_ = createQSharedPointer<QJsonObject>(data);

  new PropertyAdaptorImpl(this);
}

QString PropertyBase::typeName() { return type()->name(); }

#include "PropertyBase.moc"
