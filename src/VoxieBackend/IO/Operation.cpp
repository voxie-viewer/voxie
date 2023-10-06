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

#include "Operation.hpp"

#include <VoxieClient/QtUtil.hpp>

#include <VoxieBackend/IO/OperationRegistry.hpp>
#include <VoxieBackend/IO/OperationResult.hpp>
#include <VoxieBackend/IO/SharpThread.hpp>

#include <VoxieClient/DBusAdaptors.hpp>

#include <QtDBus/QDBusConnection>

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QThread>

using namespace vx;
using namespace vx::io;

namespace vx {
namespace io {
class OperationAdaptorImpl : public OperationAdaptor {
  Operation* object;

 public:
  OperationAdaptorImpl(Operation* object)
      : OperationAdaptor(object), object(object) {}
  ~OperationAdaptorImpl() override {}

  bool isCancelled() const override {
    try {
      return object->isCancelled();
    } catch (Exception& e) {
      e.handle(object);
      return false;
    }
  }
  bool isFinished() const override {
    try {
      return object->isFinished();
    } catch (Exception& e) {
      e.handle(object);
      return false;
    }
  }

  void WaitFor(const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options, "Timeout");

      if (ExportedObject::hasOption(options, "Timeout")) {
        auto timeout =
            ExportedObject::getOptionValue<double>(options, "Timeout");
        if (timeout != 0) {
          // TODO: Support this
          throw vx::Exception("de.uni_stuttgart.Voxie.NotSupported",
                              "Timeout != 0 are not supported");
        }

        // Timeout is 0, just throw a timeout exception if the operation is not
        // yet finished
        if (!object->isFinished())
          throw vx::Exception("de.uni_stuttgart.Voxie.Timeout",
                              "Timeout while waiting for operation to finish");
      }

      object->dbusDelayedReturn(object);
    } catch (Exception& e) {
      e.handle(object);
    }
  }

  bool GetSucceeded() override {
    try {
      return !object->resultError();
    } catch (Exception& e) {
      return e.handle(object);
    }
  }
  QString GetError(QString& message) override {
    try {
      auto result = object->resultError();
      if (!result)
        throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                            "Operation did not fail");
      message = result->error()->message();
      return result->error()->name();
    } catch (Exception& e) {
      return e.handle(object);
    }
  }
};
}  // namespace io
}  // namespace vx

Operation::Operation()
    : RefCountedObject("Operation"),
      scriptOutput_(createQSharedPointer<QString>()) {
  auto adaptor = new OperationAdaptorImpl(this);

  cancelledVal.store(0);

  QObject::connect(this, &Operation::finished, adaptor, [adaptor]() {
    Q_EMIT adaptor->Finished(QMap<QString, QDBusVariant>());
  });
}
Operation::~Operation() {}

Operation::ResultBase::~ResultBase() {}

Operation::ResultSuccess::~ResultSuccess() {}
bool Operation::ResultSuccess::isError() const { return false; }

Operation::ResultError::ResultError(const QSharedPointer<vx::Exception>& error)
    : error_(error) {
  if (!error)
    throw Exception("de.uni_stuttgart.Voxie.InternalError",
                    "ResultError::ResultError(): Got nullptr error");
}
Operation::ResultError::~ResultError() {}
bool Operation::ResultError::isError() const { return true; }

void Operation::cancel() {
  if (QThread::currentThread() != thread())
    qCritical() << "Operation::cancel() called from wrong thread"
                << QThread::currentThread();

  if (isCancelled()) return;

  cancelledVal.store(1);

  Q_EMIT cancelled();
}

void Operation::updateProgress(float progress) {
  if (progress < 0 || progress > 1) {
    qWarning() << "Got progress value of" << progress;
    if (progress < 0)
      progress = 0;
    else
      progress = 1;
  }

  // This will emit this->progressChanged(progress) but will compress multiple
  // invocations of updateProgress() while the slots are still running into a
  // single invocation

  QMutexLocker lock(&progressMutex);

  progressPending = true;
  progressPendingValue = progress;

  if (!progressUpdating) {
    enqueueOnThread(this, [this] { emitProgressChanged(); });
    progressUpdating = true;
  }
}

void Operation::emitProgressChanged() {
  QMutexLocker lock(&progressMutex);

  if (!progressUpdating) {
    qCritical() << "Operation::updateProgress: progressUpdating is false;";
    return;
  }
  if (!progressPending) {
    qCritical() << "Operation::updateProgress: progressPending is false;";
    progressUpdating = false;
    return;
  }
  float value = progressPendingValue;
  progressPending = false;

  lock.unlock();
  Q_EMIT this->progressChanged(value);
  lock.relock();

  if (progressPending)
    enqueueOnThread(this, [this] { emitProgressChanged(); });
  else
    progressUpdating = false;
}

void Operation::setDescription(const QString& desc) {
  if (thread() != QThread::currentThread()) {
    qCritical() << "Operation::setDescription() call from incorrect thread";
    return;
  }

  description_ = desc;
  Q_EMIT descriptionChanged(description_);
}
const QString& Operation::description() {
  if (thread() != QThread::currentThread()) {
    qCritical() << "Operation::description() call from incorrect thread";
  }

  return description_;
}

void Operation::finish(const QSharedPointer<ResultBase>& result) {
  vx::checkOnMainThread("Operation::finish");

  if (!result)
    throw Exception("de.uni_stuttgart.Voxie.InternalError",
                    "Operation::finish(): Got nullptr result");

  if (isFinished_) {
    qWarning() << "Got multiple results for operation" << this
               << "ignoring later result";
    return;
  }

  if (finishCalled_) {
    qWarning() << "Got additional results for operation" << this
               << "while processing result";
    return;
  }

  finishCalled_ = true;
  resultError_ = qSharedPointerDynamicCast<ResultError>(result);

  // TODO: Where should the result be set? Here or after the beforeFinished
  // signal is emitted?
  // Note that currently OperationResult::onBeforeFinished() and
  // OperationResult::onBeforeFinishedSuccess() rely on this being set before
  // beforeFinishedCalled_ is set to true.
  {
    QSharedPointer<OperationResult> operationResult;
    {
      QMutexLocker lock(&operationResultMutex);
      operationResult = operationResult_.lock();
    }
    if (operationResult) {
      QMutexLocker lock(&operationResult->resultMutex);
      operationResult->result_ = result;
    } else {
      // TODO: Should this print a warning if there is no OperationResult
      // object? Or at least if there never was one?
    }
  }

  beforeFinishedCalled_ = true;
  Q_EMIT this->beforeFinished(result);

  // Free all listeners connected to this signal (the signal will only be
  // emitted once)
  QObject::disconnect(this, &Operation::beforeFinished, nullptr, nullptr);

  isFinished_ = true;

  Q_EMIT this->finished(result);

  // Free all listeners connected to this signal (the signal will only be
  // emitted once)
  QObject::disconnect(this, &Operation::finished, nullptr, nullptr);
}

void Operation::onBeforeFinished(
    QObject* obj,
    const vx::SharedFunPtr<void(const QSharedPointer<ResultError>& result)>&
        fun) {
  vx::checkOnMainThread("Operation::onBeforeFinished");
  if (!this->beforeFinishedCalled_) {
    // There is no race condition between beforeFinishedCalled_ and registering
    // the handlers because both finish() and this function are on the main
    // thread
    QObject::connect(this, &Operation::beforeFinished, obj,
                     [fun](const QSharedPointer<ResultBase>& result) {
                       fun(qSharedPointerDynamicCast<ResultError>(result));
                     });
  } else {
    fun(resultError());
  }
}
void Operation::onBeforeFinishedError(
    QObject* obj,
    const vx::SharedFunPtr<void(const QSharedPointer<ResultError>& result)>&
        fun) {
  onBeforeFinished(obj, [fun](const QSharedPointer<ResultError>& result) {
    if (result) fun(result);
  });
}
void Operation::onBeforeFinishedSuccess(QObject* obj,
                                        const vx::SharedFunPtr<void()>& fun) {
  onBeforeFinished(obj, [fun](const QSharedPointer<ResultError>& result) {
    if (!result) fun();
  });
}

void Operation::onFinished(
    QObject* obj,
    const vx::SharedFunPtr<void(const QSharedPointer<ResultError>& result)>&
        fun) {
  vx::checkOnMainThread("Operation::onFinished");
  if (!this->isFinished()) {
    // There is no race condition between isFinished() and registering the
    // handlers because both finish() and this function are on the main thread
    QObject::connect(this, &Operation::finished, obj,
                     [fun](const QSharedPointer<ResultBase>& result) {
                       fun(qSharedPointerDynamicCast<ResultError>(result));
                     });
  } else {
    fun(resultError());
  }
}
void Operation::onFinishedError(
    QObject* obj,
    const vx::SharedFunPtr<void(const QSharedPointer<ResultError>& result)>&
        fun) {
  onFinished(obj, [fun](const QSharedPointer<ResultError>& result) {
    if (result) fun(result);
  });
}
void Operation::onFinishedSuccess(QObject* obj,
                                  const vx::SharedFunPtr<void()>& fun) {
  onFinished(obj, [fun](const QSharedPointer<ResultError>& result) {
    if (!result) fun();
  });
}

void Operation::runThreaded(
    const QSharedPointer<Operation>& op, const QString& description,
    const vx::SharedFunPtr<QSharedPointer<ResultSuccess>()>& fun) {
  op->setDescription(description);
  OperationRegistry::instance()->addOperation(op);

  SharpThread* thread = new SharpThread([fun, op]() -> void {
    QSharedPointer<Operation::ResultBase> result;
    try {
      result = fun();
      if (!result)
        throw Exception("de.uni_stuttgart.Voxie.InternalError",
                        "Operation::runThreaded(): Got a nullptr result");
    } catch (Exception& e) {
      result =
          createQSharedPointer<ResultError>(createQSharedPointer<Exception>(e));
    }
    executeOnMainThread([op, result]() { op->finish(result); });
  });

  connect(thread, &QThread::finished, thread, &QObject::deleteLater);

  thread->start();
}

QSharedPointer<Operation::ResultError> Operation::resultError() {
  if (!this->isFinished())
    throw vx::Exception("de.uni_stuttgart.Voxie.OperationNotFinished",
                        "The operation is still running");
  // Should be thread-safe when isFinished is true
  return this->resultError_;
}

// Note: This is similar to OperationResult::dbusDelayedReturnImpl
void Operation::dbusDelayedReturn(QDBusContext* context) {
  vx::checkOnMainThread("Operation::dbusDelayedReturn");

  auto main = QCoreApplication::instance();
  auto conn = context->connection();
  auto msg = context->message();
  context->setDelayedReply(true);

  this->onFinished(
      main,
      [conn, msg](const QSharedPointer<vx::io::Operation::ResultError>& error) {
        if (error)
          conn.send(msg.createErrorReply(error->error()->name(),
                                         error->error()->message()));
        else
          conn.send(msg.createReply());
      });
}

OperationSimple::OperationSimple() {}
OperationSimple::~OperationSimple() {}
