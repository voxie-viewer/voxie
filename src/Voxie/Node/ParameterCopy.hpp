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

#include <Voxie/Node/DataNode.hpp>
#include <Voxie/Node/Node.hpp>
#include <Voxie/Node/NodePrototype.hpp>
#include <Voxie/Node/ParameterCopyBase.hpp>
#include <Voxie/Node/WeakParameterCopy.hpp>
#include <VoxieClient/Exception.hpp>

#include <VoxieBackend/Data/Data.hpp>

namespace vx {
class Data;
class DataVersion;
class Node;
class NodePrototype;

/**
 * A copy of the properties of some Node and all its dependencies.
 *
 * This copy can survive if the some of the Nodes are destroyed.
 *
 * This class will also keep alive any Data objects referenced by these Nodes.
 */
class VOXIECORESHARED_EXPORT ParameterCopy : public ParameterCopyBase {
 public:
  class DataInfo {
    QSharedPointer<Data> data_;
    QSharedPointer<DataVersion> version_;

   public:
    DataInfo() : data_(nullptr), version_(nullptr) {}
    DataInfo(const QSharedPointer<Data>& data,
             const QSharedPointer<DataVersion>& version);

    const QSharedPointer<Data>& data() const { return data_; }
    const QSharedPointer<DataVersion>& version() const { return version_; }

    bool operator==(const DataInfo& other) const;

    bool operator!=(const DataInfo& other) const;
  };

  ParameterCopy(
      const QDBusObjectPath& mainNodePath,
      const QMap<QDBusObjectPath,
                 QSharedPointer<const QMap<QString, QVariant>>>& properties,
      const QMap<QDBusObjectPath, QSharedPointer<NodePrototype>>& prototypes,
      const QMap<QDBusObjectPath, DataInfo>& dataMap)
      : ParameterCopyBase(mainNodePath, properties, prototypes),
        dataMap_(dataMap) {}
  ~ParameterCopy() {}

  static QSharedPointer<ParameterCopy> getParameters(Node* mainNode);

  /**
   * @brief Get single parameterCopy that contains all parameters of the
   * passed nodes. The mainNode has to be specified independently and should be
   * contained in the list
   */
  static QSharedPointer<ParameterCopy> getParameters(QList<Node*> nodes,
                                                     Node* mainNode);

  ParameterCopy::DataInfo getData(const QDBusObjectPath& key) const;
  const QMap<QDBusObjectPath, DataInfo>& dataMap() const { return dataMap_; }
  QSharedPointer<WeakParameterCopy> createWeakCopy();

 private:
  QMap<QDBusObjectPath, DataInfo> dataMap_;
};

}  // namespace vx
