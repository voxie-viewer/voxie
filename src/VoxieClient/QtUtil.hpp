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

#include <VoxieClient/VoxieClient.hpp>

#include <QtCore/QObject>
#include <QtCore/QRunnable>
#include <QtCore/QSharedPointer>

#include <functional>
#include <type_traits>

// QList::toStdVector() was deprecated in Qt 5.14, but in many cases using this
// is much easier than calling the std::vector constructor with two arguments.
template <typename T>
std::vector<T> toStdVector(const QVector<T>& vector) {
  return std::vector<T>(vector.begin(), vector.end());
}
template <typename T>
std::vector<T> toStdVector(const QList<T>& list) {
  return std::vector<T>(list.begin(), list.end());
}

// QVector::fromStdVector() was deprecated in Qt 5.14, but the replacement is
// not available before Qt 5.14.
template <typename T>
QVector<T> toQVector(const std::vector<T>& vector) {
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
  return QVector<T>::fromStdVector(vector);
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
}
template <typename T>
QList<T> toQList(const std::vector<T>& vector) {
  return toQVector(vector).toList();
}

// QSet::fromList() was deprecated in Qt 5.14, but the replacement is
// not available before Qt 5.14.
template <typename T>
QSet<T> toQSet(const QList<T>& list) {
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
  return QSet<T>::fromList(list);
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
}

template <typename T, typename... U>
static inline QSharedPointer<T> createQSharedPointer(U&&... par) {
  // Creating a QSharedPointer for a QObject with a default deallocator is
  // almost always the wrong thing to do: If a background thread drops the last
  // reference, the destructor would be run on a background thread. Use
  // makeSharedQObject<>() instead.
  static_assert(!std::is_base_of<QObject, T>::value,
                "Trying to call createQSharedPointer for a QObject");

  // QSharedPointer<T>::create() is broken: If the constructor throws an
  // exception, it will call the objects destructor.
  // https://bugreports.qt.io/browse/QTBUG-49824
  return QSharedPointer<T>(new T(std::forward<U>(par)...));
}

namespace vx {
class RefCountedObject;

/**
 * Make sure that the code is running on the main thread.
 */
VOXIECLIENT_EXPORT void checkOnMainThread(const char* what);

/**
 * Make sure that the code is *not* running on the main thread. This might be
 * usedful for avoiding deadlocks.
 */
VOXIECLIENT_EXPORT void checkNotOnMainThread(const char* what);
}  // namespace vx

// Allocate T and free it using deleteLater() when the reference count drops to
// zero.
template <typename T, typename... U>
static inline QSharedPointer<T> makeSharedQObject(U&&... par) {
  static_assert(
      std::is_base_of<QObject, T>::value,
      "makeSharedQObject requires its first template argument to be a QObject");

  vx::checkOnMainThread("makeSharedQObject");

  // For objects derived from RefCountedObject, the createBase() / create()
  // methods should be used instead of makeSharedQObject() (which would not call
  // initialize() etc.).
  static_assert(!std::is_base_of<vx::RefCountedObject, T>::value,
                "Trying to call makeSharedQObject for a vx::RefCountedObject");

  // TODO: Use deleteLater() only when the objects thread is not the current
  // thread? (Same then for RefCountedObject)

  return QSharedPointer<T>(new T(std::forward<U>(par)...),
                           [](QObject* obj) { obj->deleteLater(); });
}

namespace vx {
VOXIECLIENT_EXPORT QObject* getMainThreadObject();
VOXIECLIENT_EXPORT Qt::ConnectionType getConnectionTypeForExecute(QObject* obj);

// Execute function fun on the thread of object obj. If obj gets destroyed
// before fun is executed it will not be executed at all. The execution will
// always be delayed, even if the method is called from obj's thread.
template <typename T>
inline void enqueueOnThread(QObject* obj, T&& fun) {
  QObject srcObj;
  QObject::connect(&srcObj, &QObject::destroyed, obj, std::move(fun),
                   Qt::QueuedConnection);
}

template <typename T>
inline void enqueueOnMainThread(T&& fun) {
  enqueueOnThread(vx::getMainThreadObject(), std::move(fun));
}

namespace QtUtilPrivate {
VOXIECLIENT_EXPORT Q_NORETURN void throwFailedExecuteFunction();

template <typename T>
struct ExecuteOnThreadHelper {
  template <typename Func>
  static decltype(auto) execute(QObject* obj, Func&& fun) {
    std::exception_ptr error;
    bool executed = false;
    std::remove_reference_t<decltype(fun())> returnValue{};
    {
      QObject srcObj;
      QObject::connect(
          &srcObj, &QObject::destroyed, obj,
          [fun = std::move(fun), &error, &executed, &returnValue]() {
            try {
              returnValue = fun();
              executed = true;
            } catch (...) {
              error = std::current_exception();
            }
          },
          vx::getConnectionTypeForExecute(obj));
    }
    if (error) std::rethrow_exception(error);
    if (!executed) throwFailedExecuteFunction();
    return returnValue;
  }
};

template <>
struct ExecuteOnThreadHelper<void> {
  template <typename Func>
  static void execute(QObject* obj, Func&& fun) {
    std::exception_ptr error;
    bool executed = false;
    {
      QObject srcObj;
      QObject::connect(
          &srcObj, &QObject::destroyed, obj,
          [fun = std::move(fun), &error, &executed]() {
            try {
              fun();
              executed = true;
            } catch (...) {
              error = std::current_exception();
            }
          },
          vx::getConnectionTypeForExecute(obj));
    }
    if (error) std::rethrow_exception(error);
    if (!executed) throwFailedExecuteFunction();
  }
};

}  // namespace QtUtilPrivate

// Execute function fun on the thread of object obj. If obj gets destroyed
// before fun is executed it will not be executed at all. If the method is
// called from obj's thread, it will be executed directly, without being
// enqueued. If the method is called from another thread, it will wait until
// the function has executed.
template <typename T>
inline decltype(auto) executeOnThread(QObject* obj, T&& fun) {
  return QtUtilPrivate::ExecuteOnThreadHelper<decltype(fun())>::execute(
      obj, std::move(fun));
}

template <typename T>
inline decltype(auto) executeOnMainThread(T&& fun) {
  return executeOnThread<T>(vx::getMainThreadObject(), std::move(fun));
}

VOXIECLIENT_EXPORT void enqueueOnBackgroundThread(std::function<void()> fun);

/**
 * QRunnable subclass that supports being constructed
 * from a lambda (or any other callable object).
 */
template <typename Function>
class FunctionalRunnable : public QRunnable {
 public:
  FunctionalRunnable(Function function) : function(std::move(function)) {}

  void run() override { this->function(); }

 private:
  Function function;
};

template <typename Function>
FunctionalRunnable<Function>* functionalRunnable(Function function) {
  return new FunctionalRunnable<Function>(std::move(function));
}
}  // namespace vx
