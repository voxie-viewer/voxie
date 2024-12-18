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

#include "Exception.hpp"

#include <VoxieClient/DebugOptions.hpp>

#include <QtCore/QDebug>
#include <QtCore/QSharedPointer>
#include <QtCore/QThreadStorage>

using namespace vx;

static QThreadStorage<std::function<void(Exception)>*> handler;

Q_DECLARE_METATYPE(QSharedPointer<vx::Exception>)

// Make sure the exception name complies with the DBus specification for error
// names
// https://dbus.freedesktop.org/doc/dbus-specification.html

static QString fixExceptionName(const QString& name) {
  auto parts = name.split('.');

  QString result;
  for (const auto& part : parts) {
    QString str;
    for (const auto& c : part) {
      if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') ||
          (c >= 'a' && c <= 'z') || c == '_') {
        if (c >= '0' && c <= '9' && str == "") str += "_";
        str += c;
      } else {
        str += QString("_%1").arg(c.unicode(), 4, 16, QChar('0'));
      }
    }
    if (str == "") str = "_";

    if (result != "") result += ".";
    result += str;
  }

  if (parts.size() == 1) {
    result = "Error." + result;
  }

  return result;
}

Exception::Exception(const QString& name, const QString& message,
                     const QString& additional)
    : name_(fixExceptionName(name)),
      message_(message),
      additional_(additional) {
  what_ = "Exception: " + this->name() + ": " + this->message();
  whatUtf8_ = what_.toUtf8();

  qRegisterMetaType<QSharedPointer<Exception>>();
}

Exception::~Exception() {}

DBusDefaultReturnValueType Exception::handle(QDBusContext* context) {
  if (handler.localData()) (*handler.localData())(*this);

  if (context->calledFromDBus()) {
    if (verboseDBusExceptions()) {
      auto msg = context->message();
      qWarning() << "Returning error over DBus:" << msg.service() << msg.path()
                 << msg.interface() << msg.member() << name() << message();
    }

    context->sendErrorReply(name(), message());
  } else if (!handler.localData()) {
    qWarning() << "Cannot handle D-Bus exception because there is no context:"
               << name() << message();
  }

  return vx::dbusDefaultReturnValue();
}

namespace {
struct PushHandler {
  std::function<void(Exception)>* storedValue;
  PushHandler(const std::function<void(Exception)>& handler) {
    storedValue = ::handler.localData();
    // Cast away const-ness here because QThreadStorage seems to be broken with
    // a pointer-to-const
    ::handler.localData() =
        const_cast<std::function<void(Exception)>*>(&handler);
  }
  ~PushHandler() { ::handler.localData() = storedValue; }
};
}  // namespace

void Exception::executeWithHandler(
    const std::function<void()>& code,
    const std::function<void(Exception)>& handler) {
  PushHandler push(handler);
  code();
}

const char* Exception::what() const throw() { return whatUtf8_.data(); }

namespace vx {
void waitForDBusPendingCall(const QDBusPendingCall& call) {
  // call.waitForFinished(); // This will not handle incoming DBus call while
  // waiting for the reply
  QEventLoop loop;
  QDBusPendingCallWatcher watcher(call);
  QObject::connect(&watcher, &QDBusPendingCallWatcher::finished, &loop,
                   [&loop](QDBusPendingCallWatcher*) { loop.quit(); });
  if (call.isFinished()) {
    QObject obj;
    QObject::connect(
        &obj, &QObject::destroyed, &loop, [&loop] { loop.exit(); },
        Qt::QueuedConnection);
  }  // this will destroy obj and trigger the lambda function inside the loop
  loop.exec();
  if (!call.isFinished()) {
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.Error",
        "QEventLoop returned but QDBusPendingCall is not finished");
  }
}

bool Exception::verboseDBusExceptions() {
  return vx::debug_option::Log_DBus_Error()->get();
}
}  // namespace vx
