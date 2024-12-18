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

#include "ParameterCopy.hpp"

#include <Voxie/Node/FilterNode.hpp>
#include <Voxie/Node/SegmentationStep.hpp>
#include <Voxie/Node/Types.hpp>

using namespace vx;

QSharedPointer<WeakParameterCopy> ParameterCopy::createWeakCopy() {
  QMap<QDBusObjectPath, WeakParameterCopy::WeakDataInfo> weakDataMap;

  for (auto it = this->dataMap_.cbegin(); it != this->dataMap_.cend(); ++it) {
    weakDataMap.insert(it.key(), WeakParameterCopy::WeakDataInfo(
                                     it.value().data(), it.value().version()));
  }

  return createQSharedPointer<WeakParameterCopy>(
      this->mainNodePath(), this->properties(), this->prototypes(),
      this->extensionInfo(), weakDataMap);
}

ParameterCopy::DataInfo ParameterCopy::getData(
    const QDBusObjectPath& key) const {
  if (!dataMap().contains(key))
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.InternalError",
        "ParameterCopy::getData(): Cannot find key: " + key.path());
  return dataMap().value(
      key, DataInfo(QSharedPointer<Data>(), QSharedPointer<DataVersion>()));
}

QSharedPointer<ParameterCopy> ParameterCopy::getParameters(Node* mainNode) {
  QList<Node*> todo;
  todo.append(mainNode);

  return getParameters(todo, mainNode);
}

QSharedPointer<ParameterCopy> ParameterCopy::getParameters(QList<Node*> nodes,
                                                           Node* mainNode) {
  QMap<QDBusObjectPath, QSharedPointer<const QMap<QString, QVariant>>>
      properties;
  QMap<QDBusObjectPath, QSharedPointer<NodePrototype>> prototypes;
  QMap<QDBusObjectPath, DataInfo> dataMap;
  QMap<QDBusObjectPath, QMap<QString, QString>> extensionInfo;

  // Find all referenced nodes
  QSet<Node*> done;
  while (nodes.size()) {
    auto obj = nodes[nodes.size() - 1];
    nodes.removeAt(nodes.size() - 1);
    if (done.contains(obj)) continue;
    done.insert(obj);

    auto propertiesObj = createQSharedPointer<QMap<QString, QVariant>>();

    properties.insert(obj->getPath(), propertiesObj);
    prototypes.insert(obj->getPath(), obj->prototype());

    for (const auto& property : obj->prototype()->nodeProperties()) {
      (*propertiesObj)[property->name()] = obj->getNodeProperty(property);

      if (!property->isReference()) continue;

      if (property->type() == types::NodeReferenceType() ||
          property->type() == types::OutputNodeReferenceType()) {
        auto target = Node::parseVariantNode(obj->getNodeProperty(property));
        if (target) nodes.append(target);
      } else if (property->type() == types::NodeReferenceListType()) {
        for (const auto& target :
             Node::parseVariantNodeList(obj->getNodeProperty(property))) {
          if (target) nodes.append(target);
        }
      } else {
        qWarning() << "Unknown reference type: " + property->typeName();
      }
    }

    if (auto dataObj = dynamic_cast<DataNode*>(obj)) {
      auto data = dataObj->data();
      dataMap.insert(obj->getPath(),
                     DataInfo(data, data ? data->currentVersion()
                                         : QSharedPointer<DataVersion>()));
    }

    if (auto filterNode = dynamic_cast<FilterNode*>(obj)) {
      extensionInfo.insert(obj->getPath(), filterNode->getExtensionInfo());
    } else if (auto segmentationStepNode =
                   dynamic_cast<SegmentationStep*>(obj)) {
      extensionInfo.insert(obj->getPath(),
                           segmentationStepNode->getExtensionInfo());
    }
  }

  return createQSharedPointer<ParameterCopy>(
      mainNode->getPath(), properties, prototypes, extensionInfo, dataMap);
}

ParameterCopy::DataInfo::DataInfo(const QSharedPointer<Data>& data,
                                  const QSharedPointer<DataVersion>& version)
    : data_(data), version_(version) {
  if (data && !version)
    throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                        "Attempting to create DataInfo with non-null data "
                        "and null version");
}

bool ParameterCopy::DataInfo::operator==(const DataInfo& other) const {
  return this->data_ == other.data_ && this->version_ == other.version_;
}

bool ParameterCopy::DataInfo::operator!=(const DataInfo& other) const {
  return !(*this == other);
}
