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

#include "OperationResult.hpp"

#include <VoxieClient/DBusAdaptors.hpp>

using namespace vx;
using namespace vx::io;

namespace vx {
class OperationResultAdaptorImpl : public OperationResultAdaptor {
  OperationResult* object;

 public:
  OperationResultAdaptorImpl(OperationResult* object)
      : OperationResultAdaptor(object), object(object) {}
  ~OperationResultAdaptorImpl() override {}

  QDBusObjectPath operation() const override {
    try {
      return vx::ExportedObject::getPath(object->operation());
    } catch (Exception& e) {
      e.handle(object);
      return vx::ExportedObject::getPath(nullptr);
    }
  }
};
}  // namespace vx

OperationResult::OperationResult(const QSharedPointer<Operation>& operation)
    : vx::RefCountedObject("OperationResult"), operation_(operation) {
  new OperationResultAdaptorImpl(this);

  if (!operation)
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.InternalError",
        "OperationResult::OperationResult: operation is nullptr");
}

void OperationResult::initialize() {
  vx::RefCountedObject::initialize();

  QMutexLocker lock(&operation()->operationResultMutex);

  if (operation()->operationResult_)
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.InternalError",
        "OperationResult::OperationResult: operationResult is already set");
  operation()->operationResult_ = thisShared();
}

OperationResult::~OperationResult() {}

QSharedPointer<Operation::ResultBase> OperationResult::resultOrNull() {
  QMutexLocker lock(&resultMutex);
  return result_;
}
QSharedPointer<Operation::ResultBase> OperationResult::result() {
  auto result = this->resultOrNull();
  if (!result)
    throw vx::Exception("de.uni_stuttgart.Voxie.OperationNotFinished",
                        "The operation is still running");
  return result;
}
QSharedPointer<Operation::ResultSuccess> OperationResult::resultSuccess() {
  auto result =
      qSharedPointerDynamicCast<Operation::ResultSuccess>(this->result());
  if (!result)
    throw vx::Exception("de.uni_stuttgart.Voxie.OperationFailed",
                        "The operation did not succeed");
  return result;
}

void OperationResult::onBeforeFinished(
    QObject* obj,
    const vx::SharedFunPtr<
        void(const QSharedPointer<Operation::ResultBase>& result)>& fun) {
  vx::checkOnMainThread("OperationResult::onBeforeFinished");
  if (!this->operation()->beforeFinishedCalled_) {
    // There is no race condition between beforeFinishedCalled_ and registering
    // the handlers because both finish() and this function are on the main
    // thread
    QObject::connect(
        operation().data(), &Operation::beforeFinished, obj,
        [fun](const QSharedPointer<Operation::ResultBase>& result) {
          fun(result);
        });
  } else {
    auto result = this->result_;
    fun(result);
  }
}
void OperationResult::onBeforeFinishedSuccess(
    QObject* obj,
    const vx::SharedFunPtr<
        void(const QSharedPointer<Operation::ResultSuccess>& result)>& fun) {
  onBeforeFinished(
      obj, [fun](const QSharedPointer<Operation::ResultBase>& result) {
        auto resultSuccess =
            qSharedPointerDynamicCast<Operation::ResultSuccess>(result);
        if (resultSuccess) fun(resultSuccess);
      });
}
void OperationResult::onBeforeFinishedError(
    QObject* obj,
    const vx::SharedFunPtr<
        void(const QSharedPointer<Operation::ResultError>& result)>& fun) {
  operation()->onBeforeFinishedError(obj, fun);
}

void OperationResult::onFinished(
    QObject* obj,
    const vx::SharedFunPtr<
        void(const QSharedPointer<Operation::ResultBase>& result)>& fun) {
  vx::checkOnMainThread("OperationResult::onFinished");
  if (!this->operation()->isFinished()) {
    // There is no race condition between isFinished() and registering the
    // handlers because both finish() and this function are on the main thread
    QObject::connect(
        operation().data(), &Operation::finished, obj,
        [fun](const QSharedPointer<Operation::ResultBase>& result) {
          fun(result);
        });
  } else {
    auto result = this->result_;
    fun(result);
  }
}
void OperationResult::onFinishedSuccess(
    QObject* obj,
    const vx::SharedFunPtr<
        void(const QSharedPointer<Operation::ResultSuccess>& result)>& fun) {
  onFinished(obj, [fun](const QSharedPointer<Operation::ResultBase>& result) {
    auto resultSuccess =
        qSharedPointerDynamicCast<Operation::ResultSuccess>(result);
    if (resultSuccess) fun(resultSuccess);
  });
}
void OperationResult::onFinishedError(
    QObject* obj,
    const vx::SharedFunPtr<
        void(const QSharedPointer<Operation::ResultError>& result)>& fun) {
  operation()->onFinishedError(obj, fun);
}

// Note: This is similar to Operation::dbusDelayedReturn
void OperationResult::dbusDelayedReturnImpl(
    QDBusContext* context,
    const std::function<QDBusVariant(
        const QSharedPointer<vx::io::Operation::ResultSuccess>&)>& fun) {
  vx::checkOnMainThread("OperationResult::dbusDelayedReturnImpl");

  auto main = QCoreApplication::instance();
  auto conn = context->connection();
  auto msg = context->message();
  context->setDelayedReply(true);

  this->onFinished(
      main, [conn, msg,
             fun](const QSharedPointer<vx::io::Operation::ResultBase>& result) {
        QDBusMessage replyMessage;
        try {
          auto error =
              qSharedPointerDynamicCast<Operation::ResultError>(result);

          if (error) {
            replyMessage = msg.createErrorReply(error->error()->name(),
                                                error->error()->message());
          } else {
            auto success =
                qSharedPointerDynamicCast<Operation::ResultSuccess>(result);
            if (!success)
              throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                                  "Cannot cast to Operation::ResultSuccess");
            replyMessage = msg.createReply(fun(success).variant());
          }
        } catch (vx::Exception& e) {
          replyMessage = msg.createErrorReply(e.name(), e.message());
        }
        conn.send(replyMessage);
      });
}
