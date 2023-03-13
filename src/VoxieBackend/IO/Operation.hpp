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

#include <VoxieClient/Exception.hpp>
#include <VoxieClient/Exceptions.hpp>
#include <VoxieClient/SharedFunPtr.hpp>

#include <VoxieBackend/VoxieBackend.hpp>

#include <VoxieClient/ObjectExport/ExportedObject.hpp>

#include <QtCore/QAtomicInt>
#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QProcess>
#include <QtCore/QSharedPointer>

// TODO: Keeping an Operation object alive should not cause the progress bar to
// be displayed after the operation is finished

// TODO: Expose DBus signals for finished / cancelled etc.

/*
Operations can have the following classes:

- A class OperationFoo inheriting from vx::io::Operation, which tracks the state
of the operation itself
- A class OperationFoo::Result inheriting from vx::io::Operation::ResultSuccess
which is the actual result value returned by the operation in the success case
- A class OperationFoo::ResultWrapper inheriting from vx::OperationResult which
is a future for the operation result (and is exported over DBus)
 */

// TODO: Should the Operation class be created by the OperationResult class?
// Would probably prevent combining different types in the wrong way

namespace vx {
class OperationResult;

namespace io {
class VOXIEBACKEND_EXPORT Operation : public vx::RefCountedObject {
  Q_OBJECT
  REFCOUNTEDOBJ_DECL(Operation)

  friend class vx::OperationResult;

  QAtomicInt cancelledVal;

  QMutex progressMutex;
  bool progressUpdating = false;
  bool progressPending = false;
  float progressPendingValue;
  void emitProgressChanged();

  QString description_;

  bool isFinished_ = false;
  bool finishCalled_ = false;
  bool beforeFinishedCalled_ = false;

  // Only used for extension operations
  QSharedPointer<QString> scriptOutput_;

  QMutex operationResultMutex;
  QWeakPointer<OperationResult> operationResult_;

 public:
  class VOXIEBACKEND_EXPORT ResultBase {
   public:
    virtual ~ResultBase();
    virtual bool isError() const = 0;
  };
  class VOXIEBACKEND_EXPORT ResultError : public ResultBase {
    QSharedPointer<vx::Exception> error_;

   public:
    ResultError(const QSharedPointer<vx::Exception>& error);
    ~ResultError() override;

    bool isError() const final override;
    const QSharedPointer<vx::Exception>& error() const { return error_; }
  };
  class VOXIEBACKEND_EXPORT ResultSuccess : public ResultBase {
   public:
    ~ResultSuccess() override;
    bool isError() const final override;
  };

  Operation();
  ~Operation();

  // thread-safe
  void updateProgress(float progress);

 public Q_SLOTS:
  void cancel();

 public:
  // thread-safe
  bool isCancelled() { return cancelledVal.load() != 0; }
  // thread-safe
  void throwIfCancelled() {
    if (isCancelled()) throw OperationCancelledException();
  }

  void setDescription(const QString& desc);
  const QString& description();

  bool isFinished() const { return isFinished_; }

  void finish(const QSharedPointer<ResultBase>& result);

  // Only used for extension operations

  // The output from the script. The string is updated in the main thread.
  // TODO: Should this be here?
  const QSharedPointer<QString>& scriptOutput() const { return scriptOutput_; }

 Q_SIGNALS:
  void progressChanged(float progress);
  void descriptionChanged(const QString& description);
  void cancelled();

  // This signal is emitted when finish() is called, but before the operation
  // is considered finished. (The handler for this signal might e.g. handle
  // store the results some data structure).
  // TODO: Use onBeforeFinished* instead of this signal in most places
  void beforeFinished(const QSharedPointer<ResultBase>& result);

  // This signal is called afterward, when isFinished() returns true
  // TODO: Use onFinished* instead of this signal in most places
  void finished(const QSharedPointer<ResultBase>& result);

 public:
  // Call fun before the operation is finished. If the beforeFinished() was
  // already emitted, call fun immediately.
  void onBeforeFinishedError(
      QObject* obj,
      const vx::SharedFunPtr<void(const QSharedPointer<ResultError>& result)>&
          fun);
  // Is called with a nullptr if there was no error
  void onBeforeFinished(
      QObject* obj,
      const vx::SharedFunPtr<void(const QSharedPointer<ResultError>& result)>&
          fun);
  void onBeforeFinishedSuccess(QObject* obj,
                               const vx::SharedFunPtr<void()>& fun);

  // Call fun once the operation is finished. If the operation is already
  // finished, call fun immediately.
  void onFinishedError(
      QObject* obj,
      const vx::SharedFunPtr<void(const QSharedPointer<ResultError>& result)>&
          fun);
  // Is called with a nullptr if there was no error
  void onFinished(
      QObject* obj,
      const vx::SharedFunPtr<void(const QSharedPointer<ResultError>& result)>&
          fun);
  void onFinishedSuccess(QObject* obj, const vx::SharedFunPtr<void()>& fun);

  // TODO: add runExtension() method which does most of the stuff from
  // ExtensionExporter::exportData?

  /**
   * Start an operation which will run f on a background thread
   *
   * When f throws an Exception, this will be returned as an error of the
   * operation. Otherwise the return value of f will be used as result of the
   * operation.
   */
  static void runThreaded(
      const QSharedPointer<Operation>& op, const QString& description,
      const vx::SharedFunPtr<QSharedPointer<ResultSuccess>()>& fun);
  template <typename OpType>
  static QSharedPointer<typename OpType::ResultWrapper> runThreaded(
      const QString& description,
      const vx::SharedFunPtr<QSharedPointer<typename OpType::Result>(
          const QSharedPointer<OpType>&)>& fun) {
    auto op = OpType::create();
    auto result = OpType::ResultWrapper::create(op);

    runThreaded(op, description, [fun, op] { return fun(op); });

    return result;
  }

  /**
   * Set the context to 'delayed reply' and return the DBus method once the
   * operation has finished. If the operation fails, an error will be returned,
   * otherwise an empty reply.
   */
  void dbusDelayedReturn(QDBusContext* context);

  QSharedPointer<ResultError> resultError();

 private:
  // Is non-null when isFinished() is true and the result was an error
  QSharedPointer<ResultError> resultError_;
};

}  // namespace io

/**
 * An operation which does not return any data, only Success or Error.
 */
class VOXIEBACKEND_EXPORT OperationSimple : public vx::io::Operation {
  Q_OBJECT
  REFCOUNTEDOBJ_DECL(OperationSimple)

 public:
  using Result = Operation::ResultSuccess;

  using ResultWrapper = OperationResult;

  OperationSimple();
  ~OperationSimple();
};

}  // namespace vx
