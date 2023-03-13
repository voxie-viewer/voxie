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

#include "RunFilterOperation.hpp"

#include <VoxieBackend/IO/Operation.hpp>

#include <VoxieBackend/Component/Extension.hpp>

#include <VoxieBackend/Component/ExternalOperation.hpp>

#include <Voxie/Node/FilterNode.hpp>

using namespace vx;
using namespace vx::io;

RunFilterOperation::Result::Result(
    const QMap<QDBusObjectPath, QMap<QString, QDBusVariant>>& data)
    : data_(data) {}
RunFilterOperation::Result::~Result() {}

RunFilterOperation::RunFilterOperation() {
  // qDebug() << "RunFilterOperation::RunFilterOperation (1)" << this;
}
RunFilterOperation::RunFilterOperation(
    vx::FilterNode* filterNode,
    const QSharedPointer<vx::ParameterCopy>& parameters)
    : filterNode_(filterNode), parameters_(parameters) {
  if (!filterNode)
    throw Exception(
        "de.uni_stuttgart.Voxie.InternalError",
        "RunFilterOperation::RunFilterOperation: Got nullptr filterNode");
  if (!parameters)
    throw Exception(
        "de.uni_stuttgart.Voxie.InternalError",
        "RunFilterOperation::RunFilterOperation: Got nullptr parameters");

  // qDebug() << "RunFilterOperation::RunFilterOperation" << this;
}

RunFilterOperation::~RunFilterOperation() {
  // qDebug() << "RunFilterOperation::~RunFilterOperation" << this;
}

QSharedPointer<RunFilterOperation>
RunFilterOperation::createRunFilterOperation() {
  QSharedPointer<RunFilterOperation> op(new RunFilterOperation());
  enqueueOnThread(op.data(), [op] {
    // This will be executed once the code returns to the main loop
    op->emitFinished();
  });
  return op;
}

// TODO: remove
void RunFilterOperation::emitFinished() {
  this->finish(createQSharedPointer<ResultSuccess>());
}
