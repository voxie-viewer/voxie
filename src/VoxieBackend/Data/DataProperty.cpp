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

#include "DataProperty.hpp"

#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>

static QHash<QString,
             QList<std::tuple<QJsonObject, QWeakPointer<vx::DataProperty>>>>
    existingDataProperties;

vx::DataProperty::DataProperty(
    const QString& name, const QJsonObject& data,
    const QSharedPointer<ComponentContainer>& propertyTypeContainer)
    : PropertyBase(name, data, propertyTypeContainer) {
  vx::checkOnMainThread("DataProperty::getOrCreate()");

  // TODO: Throw here if there are unknown keys in the JSON data?
  // TODO: Which keys should be allowed here?
  QSet<QString> known{
      "DisplayName",
      "Type",
  };
}

QSharedPointer<vx::DataProperty> vx::DataProperty::getOrCreate(
    const QString& name, const QJsonObject& data,
    const QSharedPointer<ComponentContainer>& propertyTypeContainer) {
  vx::checkOnMainThread("DataProperty::getOrCreate()");

  if (existingDataProperties.contains(name)) {
    const auto& list = existingDataProperties[name];
    for (const auto& entry : list) {
      if (std::get<0>(entry) == data) {
        auto prop = std::get<1>(entry).lock();
        if (prop) return prop;
      }
    }
  }

  auto prop = create(name, data, propertyTypeContainer);
  QObject::connect(prop.data(), &QObject::destroyed, [name]() {
    // qDebug() << "Start cleanup for" << name;
    if (!existingDataProperties.contains(name)) return;
    auto& list = existingDataProperties[name];

    int pos = 0;
    while (pos < list.size()) {
      if (std::get<1>(list[pos]).isNull()) {
        // qDebug() << "Cleanup at" << pos;
        list.removeAt(pos);
      } else {
        pos++;
      }
    }
    // qDebug() << "Cleanup for" << name << "done";
  });
  existingDataProperties[name].push_back(std::make_tuple(data, prop));
  return prop;
}

vx::DataProperty::~DataProperty() {}

QJsonValue vx::DataProperty::toJson() { return rawJson(); }
