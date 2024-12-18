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

#include <VoxieClient/QtUtil.hpp>
#include <VoxieClient/VoxieClient.hpp>

#include <QtCore/QMutex>
#include <QtCore/QThreadPool>

namespace vx {
class Task;

// TODO:
namespace io {
class Operation;
}
using CancellationToken = io::Operation;

namespace Intern {
VOXIECLIENT_EXPORT Q_NORETURN void raiseRunParallelInternalError(
    const char* info);
VOXIECLIENT_EXPORT void runParallelSetProgress(Task* task, double progress);
VOXIECLIENT_EXPORT void runParallelSetFinished(Task* task);
VOXIECLIENT_EXPORT void runParallelThrowIfCancelled(
    CancellationToken* cancellationToken);
}  // namespace Intern

// Run with static scheduling, f is called with (min, max) indices
template <typename F>
void runParallelStaticRange(size_t itemCount, const F& f) {
  if (itemCount == 0)
    // Nothing to do
    return;

  size_t threadCount = QThread::idealThreadCount();
  if (threadCount > itemCount) threadCount = itemCount;
  if (threadCount == 0)
    Intern::raiseRunParallelInternalError("threadCount == 0");

  // Note: The last thread might have fewer itemCount.
  size_t itemsPerThread = (itemCount - 1 + threadCount) / threadCount;

  QThreadPool pool;
  std::vector<std::exception_ptr> error(threadCount);
  for (size_t i = 0; i < threadCount; i++) {
    // Note: Because of the waitForDone(), capturing by reference should be ok
    // here
    // But i must be captured by value
    pool.start(functionalRunnable([&, i]() {
      try {
        size_t min = i * itemsPerThread;
        size_t max = min + itemsPerThread;
        if (max > itemCount) max = itemCount;

        f(min, max);
      } catch (...) {
        // TODO: Cancel other threads somehow?
        error[i] = std::current_exception();
      }
    }));
  }
  pool.waitForDone();
  for (size_t i = 0; i < threadCount; i++)
    if (error[i]) std::rethrow_exception(error[i]);
}

// Run with static scheduling, f is called once per thread with a callback, the
// callback then calls f2 (passed to the callback) for each item
template <typename F>
void runParallelStaticPrepare(Task* task, CancellationToken* cancellationToken,
                              size_t itemCount, const F& f) {
  QMutex mutex;
  size_t finishedItems = 0;

  vx::Intern::runParallelSetProgress(task, 0.0);
  runParallelStaticRange(itemCount, [&](size_t first, size_t last) {
    // Run prepare function
    f([&](const auto& f2) {
      // After prepare
      for (size_t i = first; i < last; i++) {
        vx::Intern::runParallelThrowIfCancelled(cancellationToken);
        f2(i);
        if (task) {
          QMutexLocker locker(&mutex);
          finishedItems += 1;
          vx::Intern::runParallelSetProgress(task, 1.0 * finishedItems / itemCount);
        }
      }
    });
  });
  vx::Intern::runParallelSetFinished(task);
}

// Run with static scheduling, f is called for each item
template <typename F>
void runParallelStatic(Task* task, CancellationToken* cancellationToken,
                       size_t itemCount, const F& f) {
  QMutex mutex;
  size_t finishedItems = 0;

  vx::Intern::runParallelSetProgress(task, 0.0);
  runParallelStaticRange(itemCount, [&](size_t first, size_t last) {
    for (size_t i = first; i < last; i++) {
      vx::Intern::runParallelThrowIfCancelled(cancellationToken);
      f(i);
      if (task) {
        QMutexLocker locker(&mutex);
        finishedItems += 1;
        vx::Intern::runParallelSetProgress(task, 1.0 * finishedItems / itemCount);
      }
    }
  });
  vx::Intern::runParallelSetFinished(task);
}

// Run with dynamic scheduling, f is called once per thread with a callback, the
// callback then calls f2 (passed to the callback) for each item
template <typename F>
void runParallelDynamicPrepare(Task* task, CancellationToken* cancellationToken,
                               size_t itemCount, const F& f) {
  vx::Intern::runParallelSetProgress(task, 0.0);

  if (itemCount == 0) {
    // Nothing to do
    vx::Intern::runParallelSetFinished(task);
    return;
  }

  size_t threadCount = QThread::idealThreadCount();
  if (threadCount > itemCount) threadCount = itemCount;
  if (threadCount == 0)
    Intern::raiseRunParallelInternalError("threadCount == 0");

  QThreadPool pool;
  QMutex mutex;
  size_t pos = 0;
  size_t finishedItems = 0;
  std::vector<std::exception_ptr> error(threadCount);
  size_t errorPos = threadCount;
  for (size_t i = 0; i < threadCount; i++) {
    // Note: Because of the waitForDone(), capturing by reference should be ok
    // here
    // But i must be captured by value
    pool.start(functionalRunnable([&, i]() {
      try {
        // Run prepare function
        f([&](const auto& f2) {
          // After prepare
          bool itemDone = false;
          for (;;) {
            vx::Intern::runParallelThrowIfCancelled(cancellationToken);

            size_t item;
            {
              QMutexLocker locker(&mutex);

              if (itemDone) {
                finishedItems += 1;
                vx::Intern::runParallelSetProgress(task, 1.0 * finishedItems / itemCount);
              }

              // If another thread failed, stop now
              if (errorPos != threadCount) break;

              // If all items have been worked on, stop
              if (pos >= itemCount) break;

              item = pos;
              pos++;
              // Release mutex
            }

            f2(item);
            itemDone = true;
          }
        });
      } catch (...) {
        error[i] = std::current_exception();
        {
          QMutexLocker locker(&mutex);
          // Note: Only the first error is considered
          if (errorPos == threadCount) errorPos = i;
        }
      }
    }));
  }
  pool.waitForDone();
  if (errorPos != threadCount) std::rethrow_exception(error[errorPos]);
  if (pos != itemCount)
    Intern::raiseRunParallelInternalError("pos != threadCount");

  vx::Intern::runParallelSetFinished(task);
}

// Run with dynamic scheduling, f is called for each item
template <typename F>
void runParallelDynamic(Task* task, CancellationToken* cancellationToken,
                        size_t itemCount, const F& f) {
  runParallelDynamicPrepare(task, cancellationToken, itemCount,
                            [&](const auto& cb) { cb(f); });
}

}  // namespace vx
