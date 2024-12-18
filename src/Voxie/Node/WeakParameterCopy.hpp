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

#include <Voxie/Node/ParameterCopyBase.hpp>
#include <VoxieClient/Exception.hpp>

namespace vx {

class WeakParameterCopy : public ParameterCopyBase {
 public:
  class WeakDataInfo {
    QWeakPointer<Data> data_;
    QSharedPointer<DataVersion> version_;

   public:
    WeakDataInfo() : data_(), version_() {}
    WeakDataInfo(const QWeakPointer<Data>& data,
                 const QSharedPointer<DataVersion>& version);

    const QWeakPointer<Data>& data() const { return data_; }
    const QSharedPointer<DataVersion>& version() const { return version_; }

    bool operator==(const WeakDataInfo& other) const;

    bool operator!=(const WeakDataInfo& other) const;
  };

  WeakParameterCopy(
      const QDBusObjectPath& mainNodePath,
      const QMap<QDBusObjectPath,
                 QSharedPointer<const QMap<QString, QVariant>>>& properties,
      const QMap<QDBusObjectPath, QSharedPointer<NodePrototype>>& prototypes,
      const QMap<QDBusObjectPath, QMap<QString, QString>>& extensionInfo,
      const QMap<QDBusObjectPath, WeakDataInfo>& dataMap)
      : ParameterCopyBase(mainNodePath, properties, prototypes, extensionInfo),
        dataMap_(dataMap) {}
  ~WeakParameterCopy() {}

  WeakParameterCopy::WeakDataInfo getData(const QDBusObjectPath& key) const;

  const QMap<QDBusObjectPath, WeakDataInfo>& dataMap() const {
    return dataMap_;
  }

  bool operator==(const WeakParameterCopy& other) const;

  bool operator!=(const WeakParameterCopy& other) const;

 private:
  QMap<QDBusObjectPath, WeakDataInfo> dataMap_;
};

}  // namespace vx
