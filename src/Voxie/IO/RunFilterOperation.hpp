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

#include <Voxie/Voxie.hpp>

#include <VoxieBackend/IO/Operation.hpp>

#include <VoxieClient/DBusTypeList.hpp>

#include <QtCore/QList>
#include <QtCore/QPointer>
#include <QtCore/QSharedPointer>

namespace vx {
class FilterNode;
class NodePrototype;
class ParameterCopy;

namespace io {

class VOXIECORESHARED_EXPORT RunFilterOperation : public vx::io::Operation {
  Q_OBJECT
  REFCOUNTEDOBJ_DECL(RunFilterOperation)

  QPointer<vx::FilterNode> filterNode_;

  QSharedPointer<vx::NodePrototype> prototype_;

  QSharedPointer<vx::ParameterCopy> parameters_;

 public:
  class VOXIECORESHARED_EXPORT Result : public ResultSuccess {
    // TODO: Change this to non-DBus types so that it also can be used by
    // builtin filters?

    // TODO: This should keep references to the results

    QMap<QDBusObjectPath, QMap<QString, QDBusVariant>> data_;

   public:
    Result(const QMap<QDBusObjectPath, QMap<QString, QDBusVariant>>& data);
    ~Result() override;

    const QMap<QDBusObjectPath, QMap<QString, QDBusVariant>>& data() const {
      return data_;
    }
  };

  // TODO: Remove parameterless constructor
  explicit RunFilterOperation();
  explicit RunFilterOperation(
      vx::FilterNode* filterNode,
      const QSharedPointer<vx::ParameterCopy>& parameters);
  ~RunFilterOperation();

  /**
   * Create a filter operation which will be finished once the code returns to
   * the main loop. This should only be used by filters which run in the main
   * thread and block the main loop.
   *
   * TODO: Get rid of users of this method.
   */
  static QSharedPointer<RunFilterOperation> createRunFilterOperation();

  const QPointer<vx::FilterNode>& filterNode() const { return filterNode_; }

  const QSharedPointer<vx::NodePrototype>& prototype() const {
    return prototype_;
  }

  const QSharedPointer<vx::ParameterCopy>& parameters() const {
    return parameters_;
  }

  // TODO: remove
  void emitFinished();
};
}  // namespace io
}  // namespace vx
