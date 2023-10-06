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

#include "SeriesData.hpp"

#include <VoxieClient/DBusAdaptors.hpp>

namespace vx {
namespace {
class SeriesDataAdaptorImpl : public SeriesDataAdaptor {
  SeriesData* object;

 public:
  SeriesDataAdaptorImpl(SeriesData* object)
      : SeriesDataAdaptor(object), object(object) {}
  ~SeriesDataAdaptorImpl() override {}

  qulonglong dimensionCount() const override {
    try {
      return object->dimensionCount();
    } catch (Exception& e) {
      return e.handle(object);
    }
  }

  QList<QDBusObjectPath> dimensions() const override {
    try {
      QList<QDBusObjectPath> result;
      for (const auto& dimension : object->dimensions())
        result << ExportedObject::getPath(dimension);
      return result;
    } catch (Exception& e) {
      return e.handle(object);
    }
  }

  void AddEntry(const QDBusObjectPath& update, const QList<quint64>& key,
                const QDBusObjectPath& value,
                const VX_IDENTITY_TYPE((QMap<QString, QDBusVariant>)) &
                    options) override {
    try {
      ExportedObject::checkOptions(options, "ReplaceMode");

      auto updateObj = DataUpdate::lookup(update);
      auto valueObj = Data::lookupOptional(value);

      auto replaceModeStr = ExportedObject::getOptionValueOrDefault<QString>(
          options, "ReplaceMode", "de.uni_stuttgart.Voxie.ReplaceMode.Insert");
      auto replaceMode = parseReplaceMode(replaceModeStr);

      object->verifyKey(key);

      if (updateObj->data().data() != object)
        throw vx::Exception("de.uni_stuttgart.Voxie.InvalidOperation",
                            "Given DataUpdate is for another object");
      if (!updateObj->running())
        throw vx::Exception("de.uni_stuttgart.Voxie.InvalidOperation",
                            "Given DataUpdate is already finished");

      object->addEntry(key, valueObj, replaceMode);
    } catch (Exception& e) {
      e.handle(object);
    }
  }

  QList<QList<quint64>> ListKeys(
      const VX_IDENTITY_TYPE((QMap<QString, QDBusVariant>)) &
      options) override {
    try {
      ExportedObject::checkOptions(options, "ReplaceMode");

      return object->entries().keys();
    } catch (Exception& e) {
      return e.handle(object);
    }
  }

  QDBusObjectPath LookupEntry(
      const QList<quint64>& key,
      const VX_IDENTITY_TYPE((QMap<QString, QDBusVariant>)) &
          options) override {
    try {
      ExportedObject::checkOptions(options, "ReplaceMode");

      object->verifyKey(key);

      return ExportedObject::getPath(object->lookupEntry(key));
    } catch (Exception& e) {
      return e.handle(object);
    }
  }
};
}  // namespace
}  // namespace vx

vx::SeriesData::SeriesData(
    const QList<QSharedPointer<SeriesDimension>>& dimensions)
    : Data(), DataContainer(this), dimensions_(dimensions) {
  new SeriesDataAdaptorImpl(this);

  for (const auto& dimension : dimensions_) {
    if (!dimension)
      throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                          "Got nullptr dimension");

    if (dimensionsByName_.contains(dimension->name()))
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Got duplicate dimension name");

    dimensionsByName_[dimension->name()] = dimension;
  }
}

vx::SeriesData::~SeriesData() {}

void vx::SeriesData::verifyKey(const EntryKeyList& key) {
  if (key.length() != dimensionCount())
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "SeriesData key has invalid number of values");

  for (int i = 0; i < dimensionCount(); i++)
    if (key[i] >= dimensions()[i]->length())
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "SeriesData key invalid value");
}

void vx::SeriesData::addEntry(const EntryKeyList& key,
                              const QSharedPointer<Data>& data,
                              ReplaceMode replaceMode) {
  vx::checkOnMainThread("SeriesData::addEntry");

  verifyKey(key);

  if (data) checkNewData(data);

  bool contains = entries_.contains(key);
  switch (replaceMode) {
    case ReplaceMode::Insert: {
      if (contains)
        throw vx::Exception(
            "de.uni_stuttgart.Voxie.DuplicateKey",
            "Attempting to insert duplicate key into SeriesData");
      break;
    }
    case ReplaceMode::Set: {
      break;
    }
    case ReplaceMode::ReplaceExisting: {
      if (!contains)
        throw vx::Exception("de.uni_stuttgart.Voxie.KeyNotFound",
                            "Failed to look up key to replace");
      break;
    }
  }

  if (contains) {
    auto child = this->entries_[key];
    entries_.remove(key);
    this->removeChildData(child);
  }

  if (data) {
    this->addChildData(data);
    entries_[key] = data;
  }
}

QSharedPointer<vx::Data> vx::SeriesData::lookupEntry(const EntryKeyList& key) {
  if (entries().contains(key))
    return entries()[key];
  else
    return QSharedPointer<vx::Data>();
}

QSharedPointer<vx::Data> vx::SeriesData::lookupEntryRequired(
    const EntryKeyList& key) {
  auto value = lookupEntry(key);
  if (!value)
    throw vx::Exception("de.uni_stuttgart.Voxie.KeyNotFound",
                        "Failed to look up key");
  return value;
}

QList<QSharedPointer<vx::SharedMemory>>
vx::SeriesData::getSharedMemorySections() {
  QList<QSharedPointer<SharedMemory>> sharedMemory = {};

  for (const auto& entry : this->entries()) {
    for (const auto& shmem : entry->getSharedMemorySections())
      sharedMemory.append(shmem);
  }

  return sharedMemory;
}
