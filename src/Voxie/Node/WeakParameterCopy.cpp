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

    // compare the inner maps' keys' values
    for (auto innerKey : this->properties()[key]->keys()) {
      if ((*this->properties()[key])[innerKey] !=
          (*other.properties()[key])[innerKey]) {
        return false;
      }
    }
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
