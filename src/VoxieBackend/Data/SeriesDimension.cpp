/*
 * Copyright (c) 2014-2023 The Voxie Authors
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

#include "SeriesDimension.hpp"

#include <VoxieBackend/Data/DataProperty.hpp>

#include <VoxieBackend/Property/PropertyType.hpp>

#include <VoxieClient/DBusAdaptors.hpp>

namespace vx {
namespace {
class SeriesDimensionAdaptorImpl : public SeriesDimensionAdaptor {
  SeriesDimension* object;

 public:
  SeriesDimensionAdaptorImpl(SeriesDimension* object)
      : SeriesDimensionAdaptor(object), object(object) {}
  ~SeriesDimensionAdaptorImpl() override {}

  /*
  QString displayName() const override {
    try {
      return object->property()->displayName();
    } catch (Exception& e) {
      return e.handle(object);
    }
  }

  QString name() const override {
    try {
      return object->property()->name();
    } catch (Exception& e) {
      return e.handle(object);
    }
  }

  QDBusObjectPath type() const override {
    try {
      return ExportedObject::getPath(object->property()->type());
    } catch (Exception& e) {
      return e.handle(object);
    }
  }
  */

  QDBusObjectPath property() const override {
    try {
      return ExportedObject::getPath(object->property());
    } catch (Exception& e) {
      return e.handle(object);
    }
  }

  quint64 length() const override {
    try {
      return object->length();
    } catch (Exception& e) {
      return e.handle(object);
    }
  }

  QDBusVariant ListEntries(
      const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options);

      return object->property()->type()->rawToDBusList(object->entries());
    } catch (Exception& e) {
      return e.handle(object);
    }
  }

  QDBusVariant GetEntryValue(
      quint64 entryKey, const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options);

      return object->property()->type()->rawToDBus(
          object->getEntryValue(entryKey));
    } catch (Exception& e) {
      return e.handle(object);
    }
  };
  quint64 LookupEntryByValue(
      const QDBusVariant& entryValue,
      const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options);

      return object->lookupEntryByValueOrInvalid(
          object->property()->type()->dbusToRaw(entryValue));
    } catch (Exception& e) {
      return e.handle(object);
    }
  };
};
}  // namespace
}  // namespace vx

vx::SeriesDimension::SeriesDimension(
    const QSharedPointer<DataProperty>& property,
    const QList<QVariant>& entries)
    : RefCountedObject("SeriesDimension"),
      property_(property),
      entries_(entries) {
  new SeriesDimensionAdaptorImpl(this);

  if (!property)
    throw vx::Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                        "property is nullptr");

  for (const auto& entry : entries) {
    property->type()->verifyValueWithoutProperty(entry);
  }
}

vx::SeriesDimension::~SeriesDimension() {}

QVariant vx::SeriesDimension::getEntryValue(EntryKey key) {
  if (key >= length())
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "SeriesData key is out of range");
  return entries()[key];
}

vx::SeriesDimension::EntryKey vx::SeriesDimension::lookupEntryByValueOrInvalid(
    const QVariant& value) {
  // TODO: Faster lookup?
  for (EntryKey i = 0; i < length(); i++) {
    if (entries()[i] == value) return i;
  }
  return invalidEntryKey;
}
