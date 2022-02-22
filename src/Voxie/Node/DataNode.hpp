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

#include <VoxieBackend/Data/DataChangedReason.hpp>

#include <Voxie/Node/Node.hpp>
#include <Voxie/Node/NodePrototype.hpp>
#include <Voxie/Node/NodeTag.hpp>

#include <QtCore/QFileInfo>

namespace vx {
class Data;
class DataVersion;
namespace io {
class Exporter;
class Importer;
}  // namespace io

class VOXIECORESHARED_EXPORT DataNode : public vx::Node {
  Q_OBJECT

  QFileInfo fileInfo;

  QList<QSharedPointer<NodeTag>> tags;
  QSharedPointer<vx::io::Importer> importer_;
  QMap<QString, QVariant> importProperties_;

 public:
  /**
   * @brief The DataNode Class should be the base class of every Data Node
   * like Volumes and Masks.
   * @param type Name of the DataNode at DBus Interface
   */
  DataNode(const QString& type, const QSharedPointer<NodePrototype>& prototype);

  QList<QString> supportedDBusInterfaces() override;

  virtual QSharedPointer<Data> data() = 0;
  void setData(const QSharedPointer<Data>& data);

  bool isAllowedChild(NodeKind kind) override;
  bool isAllowedParent(NodeKind kind) override;
  bool doTagsMatch(QSharedPointer<NodeProperty> property);
  bool hasMatchingTags(NodePrototype* childPrototype);

  const QFileInfo& getFileInfo() const { return this->fileInfo; }
  const QSharedPointer<vx::io::Importer>& importer() { return importer_; }
  const QMap<QString, QVariant>& importProperties() {
    return importProperties_;
  }
  void setFileInfo(const QFileInfo& fileInfo,
                   const QSharedPointer<vx::io::Importer>& importer,
                   const QMap<QString, QVariant>& importProperties);

  QList<QSharedPointer<NodeTag>>& getTags() { return tags; }

  void setTags(QList<QSharedPointer<NodeTag>>& pTags) {
    tags = pTags;
    Q_EMIT tagsChanged();
  }

  QSharedPointer<vx::io::Exporter> defaultExporter();

 protected:
  virtual void setDataImpl(const QSharedPointer<Data>& data) = 0;

 Q_SIGNALS:
  /**
   * This signal is emitted whenever setData() is called or the
   * data()->dataChanged is emitted.
   * @param newVersion The new data version
   * @param newUpdate True if dataChanged is emitted because a new update object
   * was created
   */
  void dataChanged(const QSharedPointer<Data>& data,
                   const QSharedPointer<DataVersion>& newVersion,
                   DataChangedReason reason);

  /**
   * Similar to dataChanged, but ignores DataChangedReason::NewUpdate changes.
   */
  void dataChangedFinished(const QSharedPointer<Data>& data,
                           const QSharedPointer<DataVersion>& newVersion,
                           DataChangedReason reason);

  void fileInfoChanged(const QFileInfo& fileInfo);

  void tagsChanged();

 private:
  QMetaObject::Connection dataChangedConnection;
};
}  // namespace vx
