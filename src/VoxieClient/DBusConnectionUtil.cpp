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

// #define private public
// #include <QtDBus/QDBusConnection>
// #undef private
// #include <QtDBus/private/qdbusconnection_p.h>

#include "DBusConnectionUtil.hpp"

#include <QtCore/QDebug>

#include <VoxieClient/Exception.hpp>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
#include <QtCore/QVersionNumber>
#endif

// From Qt5.11
class FakeQDBusConnectionPrivate : public QObject {
 public:
  QAtomicInt ref;
  QDBusConnection::ConnectionCapabilities capabilities;
  QString name;

  /*
  static void* getDPtr(QObject* obj) {
    return ((FakeQDBusConnectionPrivate*)obj)->d_ptr;
  }
  */
};

// This is a hack to workaround https://bugreports.qt.io/browse/QTBUG-85396
// Note that this assumes things about the memory layout of QDBusConnection and
// QDBusConnectionPrivate and assumes that the connection actually support file
// descriptor passing.
void vx::setupPeerDBusConnection(QDBusConnection& connection) {
  auto qtVersionStr = qVersion();
  if (!qtVersionStr) {
    qCritical() << "qVersion() return nullptr";
    return;
  }
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
  int suffixIndex = -1;
  auto qtVersion = QVersionNumber::fromString(qtVersionStr, &suffixIndex);
  if (strlen(qtVersionStr) != (size_t)suffixIndex) {
    qCritical() << "Failed to parse return value from qVersion():"
                << qtVersionStr;
    return;
  }
  if (qtVersion >= QVersionNumber(5, 15, 1)) {
    // Starting with Qt 5.15.1 the bug should be fixed (at least partially, but
    // that is sufficient)
    // qDebug() << "Not using DBus peer workaround";
    return;
  }
#else
  // Code for Qt 5.5
  qDebug() << "XX";
  auto qtVersion = QString(qtVersionStr).split('.');
  QList<int> qtVersionInt;
  for (int i = 0; i < qtVersion.size(); i++) {
    bool ok = false;
    qtVersionInt << qtVersion[i].toInt(&ok);
    if (!ok) {
      qCritical() << "Error paring return value from qVersion():"
                  << qtVersionStr;
      return;
    }
  }
  bool hasFix;
  int neededVersion0 = 5;
  int neededVersion1 = 15;
  int neededVersion2 = 1;
  if (qtVersionInt.size() <= 0 || qtVersionInt[0] < neededVersion0)
    hasFix = false;
  else if (qtVersionInt[0] > neededVersion0)
    hasFix = true;
  else if (qtVersionInt.size() <= 1 || qtVersionInt[1] < neededVersion1)
    hasFix = false;
  else if (qtVersionInt[1] > neededVersion1)
    hasFix = true;
  else if (qtVersionInt.size() <= 2 || qtVersionInt[2] < neededVersion2)
    hasFix = false;
  else
    hasFix = true;
  if (hasFix) {
    // Starting with Qt 5.15.1 the bug should be fixed (at least partially, but
    // that is sufficient)
    // qDebug() << "Not using DBus peer workaround";
    return;
  }
  // qDebug() << qtVersionInt;
#endif
  // qDebug() << "Using DBus peer workaround";

  (void)connection;

#ifdef Q_OS_UNIX
  // This assumes that d is the first value
  auto priv = *((FakeQDBusConnectionPrivate**)&connection);

  // qDebug() << &connection.d->capabilities << &priv->capabilities;

  if (priv->capabilities != connection.connectionCapabilities())
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.InternalError",
        "priv->capabilities != connection.connectionCapabilities()");
  priv->capabilities |= QDBusConnection::UnixFileDescriptorPassing;
  if (priv->capabilities != connection.connectionCapabilities())
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.InternalError",
        "priv->capabilities != connection.connectionCapabilities() (2)");
#endif
}
