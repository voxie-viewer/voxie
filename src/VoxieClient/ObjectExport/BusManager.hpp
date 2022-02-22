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
#include <QtCore/QPointer>
#include <QtCore/QString>

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusServer>

namespace vx {
class ExportedObject;

class VOXIECLIENT_EXPORT DBusService : public QObject {
  Q_OBJECT

 public:
  virtual void addArgumentsTo(QStringList& args) = 0;
};

// TODO: Support busses other than the session bus?
class VOXIECLIENT_EXPORT DBusServiceBus : public DBusService {
  QDBusConnection connection_;

 public:
  DBusServiceBus(const QDBusConnection& connection);

  void addArgumentsTo(QStringList& args) override;

  QDBusConnection& connection() { return connection_; }
};

class VOXIECLIENT_EXPORT DBusServicePeer : public DBusService {
  QDBusServer* server_;

 public:
  DBusServicePeer(QDBusServer* server);

  void addArgumentsTo(QStringList& args) override;

  QDBusServer* server() const { return server_; }

  static QSharedPointer<DBusServicePeer> create();
};

class VOXIECLIENT_EXPORT BusConnection : public QObject {
  Q_OBJECT

  QString connectionName_;
  QDBusConnection connection_;
  bool isBus_;

 public:
  BusConnection(const QDBusConnection& connection, bool isBus);
  ~BusConnection();

  const QString& connectionName() const { return connectionName_; }
  QDBusConnection& connection() { return connection_; }
  bool isBus() const { return isBus_; }

 Q_SIGNALS:
  void disconnected();
};

class VOXIECLIENT_EXPORT BusManager : public QObject {
  Q_OBJECT

  QList<std::tuple<QString, QPointer<ExportedObject>,
                   QDBusConnection::RegisterOptions>>
      entries;
  QList<QSharedPointer<BusConnection>> connections;

 public:
  BusManager();

  void addConnection(const QSharedPointer<BusConnection>& connection);

  void registerObject(ExportedObject* object,
                      QDBusConnection::RegisterOptions options =
                          QDBusConnection::ExportAdaptors);

  void sendEverywhere(const QDBusMessage& msg);

  QDBusServer* startServer();

  QSharedPointer<BusConnection> findConnection(
      const QDBusConnection& connection);
};

// TODO: Move to Root class?
VOXIECLIENT_EXPORT BusManager* getBusManager();

}  // namespace vx
