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

#include "WeakParameterCopy.hpp"

#include <QtCore/QDebug>

#include <VoxieClient/Format.hpp>

#include <Voxie/Node/NodePrototype.hpp>

#include <VoxieBackend/Data/Data.hpp>

using namespace vx;

WeakParameterCopy::WeakDataInfo::WeakDataInfo(
    const QWeakPointer<Data>& data, const QSharedPointer<DataVersion>& version)
    : data_(data), version_(version) {
  if (data && !version)
    throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                        "Attempting to create DataInfo with non-null data "
                        "and null version");
}

bool WeakParameterCopy::WeakDataInfo::operator==(
    const WeakDataInfo& other) const {
  return this->data_ == other.data_ && this->version_ == other.version_;
}

bool WeakParameterCopy::WeakDataInfo::operator!=(
    const WeakDataInfo& other) const {
  return !(*this == other);
}

bool WeakParameterCopy::operator==(const WeakParameterCopy& other) const {
  // if the two maps don't contain the same amount of items they can't be
  // equal
  if (this->properties().size() != other.properties().size()) {
    return false;
  }

  // if the two maps don't contain the same amount of items they can't be
  // equal
  if (this->prototypes().size() != other.prototypes().size()) {
    return false;
  }

  // iterate over the keys of one map and check if all of them are in the
  // other map too. If they are, check that the values of the keys are equal
  for (auto key : this->prototypes().keys()) {
    if (!other.prototypes().contains(key)) {
      return false;
    }

    if (this->prototypes()[key] != other.prototypes()[key]) {
      return false;
    }
  }

  // iterate over the keys of one map and check if all of them are in the
  // other map too. If they are, check that the values of the keys are equal
  // too
  for (auto key : this->properties().keys()) {
    if (!other.properties().contains(key)) {
      return false;
    }

    // compare sizes of the inner maps
    if (this->properties()[key]->size() != other.properties()[key]->size()) {
      return false;
    }

    auto thisPrototype = this->prototypes()[key];
    auto otherPrototype = other.prototypes()[key];
    if (!thisPrototype || !otherPrototype) {
      qWarning()
          << "Failed to look up prototype in WeakParameterCopy::operator==()";
      return false;
    }

    // compare the inner maps' keys' values
    for (auto innerKey : this->properties()[key]->keys()) {
      auto thisType = thisPrototype->getProperty(innerKey, false)->type();
      auto otherType = otherPrototype->getProperty(innerKey, false)->type();

      // Type of property changed (Can this happend?
      if (thisType != otherType) return false;

      if (!thisType->equals((*this->properties()[key])[innerKey],
                            (*other.properties()[key])[innerKey]))
        return false;
    }
  }

  // if the two maps don't contain the same amount of items they can't be
  // equal
  if (this->extensionInfo().size() != other.extensionInfo().size()) {
    return false;
  }

  // iterate over the keys of one map and check if all of them are in the
  // other map too. If they are, check that the values of the keys are equal
  for (auto key : this->extensionInfo().keys()) {
    if (!other.extensionInfo().contains(key)) {
      return false;
    }

    // Note: This uses the QMap<QString, QString> operator==
    if (this->extensionInfo()[key] != other.extensionInfo()[key]) {
      return false;
    }
  }

  // if the two maps don't contain the same amount of items they can't be
  // equal
  if (this->dataMap_.size() != other.dataMap_.size()) {
    return false;
  }

  // iterate over the keys of one map and check if all of them are in the
  // other map too. If they are, check that the values of the keys are equal
  for (auto key : this->dataMap_.keys()) {
    if (!other.dataMap_.contains(key)) {
      return false;
    }

    if (this->dataMap_[key] != other.dataMap_[key]) {
      return false;
    }
  }

  return true;
}

bool WeakParameterCopy::operator!=(const WeakParameterCopy& other) const {
  return !(*this == other);
}

WeakParameterCopy::WeakDataInfo WeakParameterCopy::getData(
    const QDBusObjectPath& key) const {
  if (!dataMap().contains(key))
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.InternalError",
        "WeakParameterCopy::getData(): Cannot find key: " + key.path());
  return dataMap().value(
      key, WeakDataInfo(QSharedPointer<Data>(), QSharedPointer<DataVersion>()));
}

static QString myToString(const WeakParameterCopy::WeakDataInfo& info) {
  auto data = info.data().lock();
  return vx::format("({}, {})", data ? data->getPath().path() : "nullptr",
                    info.version()->versionString());
}

static QString myToString(const vx::Any& var) {
  // TODO: Call type()->valueToString()?
  // TODO: Provide generic toString() for vx::Any?
  // return "vx::Any(" + vx::toString(var.type()) + ")";
  return vx::toString(var);
}

// TODO: Move this somewhere else?
template <typename K, typename V>
static QString myToString(const QMap<K, V>& map) {
  QString res = "{";
  bool empty = true;
  for (const auto& key : map.keys()) {
    if (!empty) res += ", ";
    res += myToString(key);
    res += ": ";
    res += myToString(map[key]);
    empty = false;
  }
  res += "}";
  return res;
}

static QString myToString(const QDBusObjectPath& path) { return path.path(); }

static QString myToString(const QSharedPointer<NodePrototype>& prototype) {
  return prototype->name();
}

template <typename T>
static QString myToString(const QSharedPointer<T>& ptr) {
  return myToString(*ptr);
}

static QString myToString(const QString& s) { return s; }

QString WeakParameterCopy::toString() const {
  QString propertiesStr = myToString(this->properties());

  QString prototypesStr = myToString(this->prototypes());

  QString extensionInfoStr = myToString(this->extensionInfo());

  QString dataMapStr = myToString(this->dataMap());

  return vx::format("WeakParameterCopy({}, {}, {}, {}, {}",
                    this->mainNodePath().path(), propertiesStr, prototypesStr,
                    extensionInfoStr, dataMapStr);
}
