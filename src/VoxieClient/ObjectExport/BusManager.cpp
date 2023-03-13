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

#include "BusManager.hpp"

#include <VoxieClient/DBusConnectionUtil.hpp>
#include <VoxieClient/QtUtil.hpp>

#include <VoxieClient/ObjectExport/ExportedObject.hpp>

#include <QtDBus/QDBusAbstractInterface>

using namespace vx;

static bool verboseTracking = false;
// static bool verboseTracking = true;
// extern char* program_invocation_short_name;
// static bool verboseTracking = QString(program_invocation_short_name) !=
// "voxie";

namespace vx {
namespace intern {
class OrgFreedesktopDBusLocalInterface : public QDBusAbstractInterface {
  Q_OBJECT

 public:
  static inline const char* staticInterfaceName() {
    return "org.freedesktop.DBus.Local";
  }

 public:
  OrgFreedesktopDBusLocalInterface(const QString& service, const QString& path,
                                   const QDBusConnection& connection,
                                   QObject* parent = nullptr)
      : QDBusAbstractInterface(service, path, staticInterfaceName(), connection,
                               parent) {}
  ~OrgFreedesktopDBusLocalInterface() {}

 Q_SIGNALS:
  void Disconnected();
};
}  // namespace intern
}  // namespace vx

DBusServiceBus::DBusServiceBus(const QDBusConnection& connection)
    : connection_(connection) {}
void DBusServiceBus::addArgumentsTo(QStringList& args) {
  // args.append("--voxie-bus-address=" + ...);
  args.append("--voxie-bus-name=" + connection().baseService());
}

DBusServicePeer::DBusServicePeer(QDBusServer* server) : server_(server) {}
void DBusServicePeer::addArgumentsTo(QStringList& args) {
  args.append("--voxie-peer-address=" + server()->address());
}

QSharedPointer<DBusServicePeer> DBusServicePeer::create() {
  auto server = getBusManager()->startServer();
  auto service = makeSharedQObject<DBusServicePeer>(server);
  QObject::connect(service.data(), &QObject::destroyed, server,
                   &QObject::deleteLater);
  return service;
}

BusConnection::BusConnection(const QDBusConnection& connection, bool isBus)
    : connectionName_(connection.name()),
      connection_(connection),
      isBus_(isBus) {
  auto intf = new vx::intern::OrgFreedesktopDBusLocalInterface(
      "", "/org/freedesktop/DBus/Local", this->connection(), this);
  QObject::connect(intf,
                   &vx::intern::OrgFreedesktopDBusLocalInterface::Disconnected,
                   this, &BusConnection::disconnected);
}
BusConnection::~BusConnection() {}

BusManager::BusManager() {}

void BusManager::addConnection(
    const QSharedPointer<BusConnection>& connection) {
  vx::checkOnMainThread("BusManager::addConnection");

  QObject::connect(
      connection.data(), &BusConnection::disconnected, this,
      [this, connection]() {
        // qDebug() << "Disconnected" << connection->connectionName();

        for (int i = 0; i < connections.size(); i++) {
          if (connections[i] == connection) {
            connections.removeAt(i);
            return;
          }
        }

        qWarning()
            << "Error while removing connection: Could not find connection"
            << connection->connectionName();
      });
  connections << connection;

  for (const auto& entry : entries) {
    ExportedObject* obj = std::get<1>(entry);
    if (!obj) {
      qWarning() << "BusManager::addConnection(): Got destroyed object in list";
    } else {
      if (!connection->connection().registerObject(std::get<0>(entry), obj,
                                                   std::get<2>(entry))) {
        qWarning() << "Failed to register object" << obj << "on connection"
                   << connection->connectionName();
      }
    }
  }
}

void BusManager::registerObject(ExportedObject* object, bool isSingleton,
                                QDBusConnection::RegisterOptions options) {
  vx::checkOnMainThread("BusManager::registerObject");

  auto path = object->getPath();

  QObject::connect(object, &QObject::destroyed, this, [this, path, object]() {
    vx::checkOnMainThread("BusManager::registerObject callback");

    // Note: The QPointer has already been invalidated at this point

    auto oldNonSingletonObjects = nonSingletonObjects;
    for (int i = 0; i < entries.size(); i++) {
      while (i < entries.size() && (std::get<1>(entries[i]) == object ||
                                    std::get<1>(entries[i]) == nullptr)) {
        if (!std::get<3>(entries[i])) {
          if (!nonSingletonObjects) {
            qCritical() << "nonSingletonObjects is 0";
          } else {
            nonSingletonObjects--;
          }
        }
        entries.removeAt(i);
      }
    }
    if (verboseTracking)
      qDebug() << "After destroying exported objects: Remaining are"
               << nonSingletonObjects << "/" << entries.count();
    if (oldNonSingletonObjects != 0 && nonSingletonObjects == 0)
      Q_EMIT allNonSingletonObjectsDestroyed();
  });

  if (verboseTracking)
    qDebug() << "Registering object" << path.path() << object
             << "isSingleton:" << isSingleton;
  entries << std::make_tuple(path.path(), object, options, isSingleton);
  if (!isSingleton) nonSingletonObjects++;

  for (auto& connection : connections) {
    if (!connection->connection().registerObject(path.path(), object,
                                                 options)) {
      qWarning() << "Failed to register object" << object << "on connection"
                 << connection->connectionName();
    }
  }
}

bool BusManager::haveNonSingletonObjects() {
  vx::checkOnMainThread("BusManager::haveNonSingletonObjects()");

  return nonSingletonObjects != 0;
}

void BusManager::sendEverywhere(const QDBusMessage& msg) {
  for (auto& connection : connections) connection->connection().send(msg);
}

QDBusServer* BusManager::startServer() {
  auto server = new QDBusServer;
  connect(server, &QDBusServer::newConnection, this,
          [this](QDBusConnection connection) {
            // qDebug() << "New DBus peer connection" << connection.name()
            //         << connection.connectionCapabilities();
            setupPeerDBusConnection(connection);
            // qDebug() << "New DBus peer connection 2" << connection.name()
            //         << connection.connectionCapabilities();
            this->addConnection(
                makeSharedQObject<BusConnection>(connection, false));
          });
  return server;
}

QSharedPointer<BusConnection> BusManager::findConnection(
    const QDBusConnection& connection) {
  vx::checkOnMainThread("BusManager::findConnection");

  auto name = connection.name();

  for (auto& conn : connections) {
    if (conn->connectionName() == name) return conn;
  }

  throw vx::Exception(
      "de.uni_stuttgart.Voxie.Error",
      "Could not find DBus connection with name '" + name + "'");
}

BusManager* vx::getBusManager() {
  static BusManager manager;
  return &manager;
}

#include "BusManager.moc"
