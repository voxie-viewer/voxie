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

#include "OperationRegistry.hpp"

#include <VoxieClient/QtUtil.hpp>

#include <VoxieBackend/DebugOptions.hpp>

#include <VoxieBackend/IO/Operation.hpp>

using namespace vx;
using namespace vx::io;

OperationRegistry::OperationRegistry() {
  vx::checkOnMainThread("OperationRegistry::OperationRegistry");
}
OperationRegistry::~OperationRegistry() {}

OperationRegistry* OperationRegistry::instance() {
  static QSharedPointer<OperationRegistry> instanceValue =
      makeSharedQObject<OperationRegistry>();
  return instanceValue.data();
}

QList<QSharedPointer<Operation>> OperationRegistry::getRunningOperations() {
  vx::checkOnMainThread("OperationRegistry::getRunningOperations()");

  QList<QSharedPointer<Operation>> result;
  for (const auto& operation : this->operations) {
    auto op = operation.lock();
    if (op) result << op;
  }
  return result;
}

void OperationRegistry::registerNewRunningOperationHandler(
    QObject* obj,
    const std::function<void(const QSharedPointer<Operation>&)>& handler) {
  vx::checkOnMainThread(
      "OperationRegistry::registerNewRunningOperationHandler()");

  QObject::connect(this, &OperationRegistry::newRunningOperation, obj, handler);
  auto operations = getRunningOperations();

  enqueueOnThread(obj, [operations, handler]() {
    for (const auto& operation : operations) {
      // qDebug() << "Calling handler for existing operation" << operation;
      handler(operation);
    }
  });
}

void OperationRegistry::addOperation(
    const QSharedPointer<Operation>& operation) {
  vx::checkOnMainThread("OperationRegistry::addOperation()");

  if (!operation)
    throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                        "operation is nullptr");

  if (vx::debug_option::Log_OperationRegistry()->get())
    qDebug() << "New operation" << operation->description();

  // Clean up destroyed operations
  // TODO: This probably should not be needed because all operations should emit
  // finished() before being destroyed
  QObject::connect(operation.data(), &QObject::destroyed, this, [this]() {
    // TODO: Do this more efficiently?
    for (int i = 0; i < operations.size();) {
      if (operations[i].isNull()) {
        operations.removeAt(i);
      } else {
        i++;
      }
    }
  });
  this->operations << operation;
  QWeakPointer<Operation> operationWeak = operation;
  // TODO: It seems like some operations (e.g. run "Extract Isosurface") are not
  // destroyed on windows. Check this.
  operation->onFinished(
      this, [this, operationWeak](
                const QSharedPointer<Operation::ResultError>& result) {
        Q_UNUSED(result);

        auto operationStrong = operationWeak.lock();
        if (!operationStrong) return;

        // TODO: Do this more efficiently?
        for (int i = 0; i < operations.size();) {
          if (operations[i] == operationStrong) {
            operations.removeAt(i);
          } else {
            i++;
          }
        }
      });

  Q_EMIT this->newRunningOperation(operation);
}
