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

#include "QtUtil.hpp"

#include <VoxieClient/Exception.hpp>

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QMutex>
#include <QtCore/QThread>
#include <QtCore/QThreadPool>

namespace vx {
void checkOnMainThread(const char* what) {
  if (QThread::currentThread() != QCoreApplication::instance()->thread()) {
    // TODO: throw an exception instead?
    qCritical() << "Attempting to call" << what << "on non-main thread"
                << QCoreApplication::instance()->thread();
  }
}

void checkNotOnMainThread(const char* what) {
  if (QThread::currentThread() == QCoreApplication::instance()->thread()) {
    // This 'only' causes deadlocks (in some cases), not as critical as in
    // checkOnMainThread()
    qWarning() << "Attempting to call" << what << "on main thread";
  }
}

Q_NORETURN void vx::QtUtilPrivate::throwFailedExecuteFunction() {
  throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                      "Failed to execute function on thread");
}

QObject* getMainThreadObject() { return QCoreApplication::instance(); }

Qt::ConnectionType getConnectionTypeForExecute(QObject* obj) {
  return QThread::currentThread() == obj->thread()
             ? Qt::DirectConnection
             : Qt::BlockingQueuedConnection;
}

class BackgroundThreadPool {
 public:
  static BackgroundThreadPool& getInstance() {
    static BackgroundThreadPool instance;
    return instance;
  }

  void start(std::function<void()> fun) {
    auto* runnable = functionalRunnable([=]() { fun(); });

    if (!pool.tryStart(runnable)) {
      // QThreadPool's functions themselves are thread-safe, but we want to
      // avoid lost-update problems when increasing the thread count. Therefore,
      // we lock the mutex in this case.
      QMutexLocker locker(&mutex);

      // Add an extra available thread
      pool.setMaxThreadCount(pool.maxThreadCount() + 1);

      // Start the thread
      pool.start(runnable);
    }
  }

 private:
  BackgroundThreadPool(){};
  QThreadPool pool;
  QMutex mutex;
};

void enqueueOnBackgroundThread(std::function<void()> fun) {
  BackgroundThreadPool::getInstance().start(fun);
}

}  // namespace vx
