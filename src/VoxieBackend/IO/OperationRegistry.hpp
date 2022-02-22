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

#include <VoxieBackend/VoxieBackend.hpp>

#include <QtCore/QList>
#include <QtCore/QSharedPointer>

#include <functional>

namespace vx {
namespace io {
class Operation;
}

class VOXIEBACKEND_EXPORT OperationRegistry : public QObject {
  Q_OBJECT

  QList<QWeakPointer<vx::io::Operation>> operations;

 public:
  OperationRegistry();
  ~OperationRegistry();

  static OperationRegistry* instance();

  QList<QSharedPointer<vx::io::Operation>> getRunningOperations();

  /**
   * Register a handler which will be called for all newly running operations.
   *
   * The handler will also be called immediately for all already running
   * operations.
   *
   * The handler will be deregistered once obj is destroyed.
   */
  void registerNewRunningOperationHandler(
      QObject* obj,
      const std::function<void(const QSharedPointer<vx::io::Operation>&)>&
          handler);

  // TODO: Should this be called automatically at some point?
  void addOperation(const QSharedPointer<vx::io::Operation>& operation);

 Q_SIGNALS:
  void newRunningOperation(const QSharedPointer<vx::io::Operation>& operation);
};

}  // namespace vx
