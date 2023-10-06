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

#include <VoxieBackend/Component/ComponentTypeInfo.hpp>
#include <VoxieBackend/Component/ExternalOperation.hpp>

#include <Voxie/Node/FilterNode.hpp>

#include <Voxie/IO/RunFilterOperation.hpp>

namespace vx {
template <typename T>
class SharedFunPtr;
class PropertyBase;

class ExtensionFilterNode : public FilterNode {
  Q_OBJECT
  REFCOUNTEDOBJ_DECL(ExtensionFilterNode)

  QString scriptFilename_;

 public:
  ExtensionFilterNode(const QSharedPointer<vx::NodePrototype>& prototype,
                      const QString& scriptFilename);
  ~ExtensionFilterNode();

  const QString& scriptFilename() const { return scriptFilename_; }

 private:
  QSharedPointer<vx::io::RunFilterOperation> calculate(
      bool isAutomaticFilterRun) override;

 Q_SIGNALS:
  void error(const Exception& e, const QSharedPointer<QString>& scriptOutput);
};

class ExternalOperationRunFilterAdaptorImpl;
class ExternalOperationRunFilter : public ExternalOperation {
  Q_OBJECT
  REFCOUNTEDOBJ_DECL(ExternalOperationRunFilter)

  friend class ExternalOperationRunFilterAdaptorImpl;

  QSharedPointer<vx::io::RunFilterOperation> operation_;

  QPointer<vx::FilterNode> filterNode_;
  QDBusObjectPath filterNodePath_;

  QSharedPointer<vx::NodePrototype> prototype_;

  QMap<QDBusObjectPath, QMap<QString, QDBusVariant>> parameters_;

  // A list of objects which are kept alive while the operation is running
  QSharedPointer<QList<QSharedPointer<vx::ExportedObject>>> references;

  bool isAutomaticFilterRun_;

 protected:
  void cleanup() override;

 public:
  explicit ExternalOperationRunFilter(
      const QSharedPointer<vx::io::RunFilterOperation>& operation,
      const QMap<QDBusObjectPath, QMap<QString, QDBusVariant>>& parameters,
      const QSharedPointer<QList<QSharedPointer<vx::ExportedObject>>>&
      references, bool isAutomaticFilterRun);
  ~ExternalOperationRunFilter() override;

  const QSharedPointer<vx::io::RunFilterOperation>& operation() const {
    return operation_;
  }

  QString action() override;

  QString name() override;

  const QPointer<vx::FilterNode>& filterNode() { return filterNode_; }

  const QMap<QDBusObjectPath, QMap<QString, QDBusVariant>>& parameters() {
    return parameters_;
  }

  bool isAutomaticFilterRun() { return isAutomaticFilterRun_; }
};
}  // namespace vx
