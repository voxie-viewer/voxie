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

#include <VoxieClient/DBusUtil.hpp>

#include <VoxieClient/ObjectExport/ExportedObject.hpp>

#include <VoxieBackend/IO/Operation.hpp>

// TODO: Add a method which will call a callback either immediately or once
// there is a result

namespace vx {
class VOXIEBACKEND_EXPORT OperationResult : public vx::RefCountedObject {
  Q_OBJECT
  REFCOUNTEDOBJ_DECL(OperationResult)

  friend class vx::io::Operation;

  QSharedPointer<vx::io::Operation> operation_;

  QMutex resultMutex;
  QSharedPointer<vx::io::Operation::ResultBase> result_;

 protected:
  void initialize() override;

 public:
  OperationResult(const QSharedPointer<vx::io::Operation>& operation);
  ~OperationResult();

  const QSharedPointer<vx::io::Operation>& operation() const {
    return operation_;
  }

  /**
   * Return the result if the operation is finished, return null otherwise.
   */
  QSharedPointer<vx::io::Operation::ResultBase> resultOrNull();
  /**
   * Return the result if the operation is finished, throw an exception
   * otherwise.
   */
  QSharedPointer<vx::io::Operation::ResultBase> result();
  /**
   * Return the result if the operation is finished and was successful, throw an
   * exception otherwise.
   */
  QSharedPointer<vx::io::Operation::ResultSuccess> resultSuccess();

  // Call fun once the operation is finished. If the operation is already
  // finished, call fun immediately.
  void onBeforeFinished(
      QObject* obj,
      const vx::SharedFunPtr<void(
          const QSharedPointer<vx::io::Operation::ResultBase>& result)>& fun);
  void onBeforeFinishedSuccess(
      QObject* obj,
      const vx::SharedFunPtr<
          void(const QSharedPointer<vx::io::Operation::ResultSuccess>& result)>&
          fun);
  void onBeforeFinishedError(
      QObject* obj,
      const vx::SharedFunPtr<void(
          const QSharedPointer<vx::io::Operation::ResultError>& result)>& fun);

  // Call fun once the operation is finished. If the operation is already
  // finished, call fun immediately.
  void onFinished(
      QObject* obj,
      const vx::SharedFunPtr<void(
          const QSharedPointer<vx::io::Operation::ResultBase>& result)>& fun);
  void onFinishedSuccess(
      QObject* obj,
      const vx::SharedFunPtr<
          void(const QSharedPointer<vx::io::Operation::ResultSuccess>& result)>&
          fun);
  void onFinishedError(
      QObject* obj,
      const vx::SharedFunPtr<void(
          const QSharedPointer<vx::io::Operation::ResultError>& result)>& fun);

 private:
  void dbusDelayedReturnImpl(
      QDBusContext* context,
      const std::function<QDBusVariant(
          const QSharedPointer<vx::io::Operation::ResultSuccess>&)>& fun);

 public:
  /**
   * Set the context to 'delayed reply' and return the DBus method once the
   * operation has finished. If the operation fails, an error will be returned,
   * otherwise fun will be called with the result object to get the return
   * value. If fun throws, an error will be returned.
   */
  template <typename F,
            typename T = decltype((*(F*)nullptr)(
                (*(QSharedPointer<vx::io::Operation::ResultSuccess>*)nullptr)))>
  T dbusDelayedReturn(QDBusContext* context, const F& fun) {
    dbusDelayedReturnImpl(
        context,
        [fun](const QSharedPointer<vx::io::Operation::ResultSuccess>& result) {
          return vx::dbusMakeVariant<T>(fun(result));
        });
    return T();
  }
};

}  // namespace vx
