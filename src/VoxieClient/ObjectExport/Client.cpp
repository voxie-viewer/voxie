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

#include "Client.hpp"

#include <VoxieClient/DBusAdaptors.hpp>
#include <VoxieClient/Exception.hpp>

#include <VoxieClient/ObjectExport/BusManager.hpp>

#include <limits>

#include <QtCore/QDebug>
#include <QtCore/QMetaType>

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>
#include <QtDBus/QDBusMetaType>
#include <QtDBus/QDBusServiceWatcher>

using namespace vx;

namespace vx {
namespace {
class ClientAdaptorImpl : public ClientAdaptor {
  Client* object;

 public:
  ClientAdaptorImpl(Client* object) : ClientAdaptor(object), object(object) {}
  ~ClientAdaptorImpl() override {}

  QString dBusConnectionName() const override {
    try {
      return object->connection()->connectionName();
    } catch (Exception& e) {
      e.handle(object);
      return "";
    }
  }

  QString uniqueConnectionName() const override {
    try {
      return object->uniqueConnectionName();
    } catch (Exception& e) {
      e.handle(object);
      return "";
    }
  }

  void DecRefCount(const QDBusObjectPath& o) override {
    try {
      object->decRefCount(o);
    } catch (Exception& e) {
      e.handle(object);
      return;
    }
  }
  QMap<QDBusObjectPath, quint64> GetReferencedObjects(
      const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options);

      return object->getReferencedObjects();
    } catch (Exception& e) {
      e.handle(object);
      return QMap<QDBusObjectPath, quint64>();
    }
  }
  void IncRefCount(const QDBusObjectPath& o) override {
    try {
      object->incRefCount(o);
    } catch (Exception& e) {
      e.handle(object);
      return;
    }
  }
};

}  // namespace
}  // namespace vx

Client::Client(QObject* parent, const QString& uniqueConnectionName,
               const QSharedPointer<BusConnection>& connection)
    : ExportedObject("Client", parent),
      uniqueConnectionName_(uniqueConnectionName),
      connection_(connection) {
  new ClientAdaptorImpl(this);

  // qDebug() << "QQQ" << name << connection->connectionName();
  QObject::connect(connection.data(), &BusConnection::disconnected, this,
                   &QObject::deleteLater);

  if (uniqueConnectionName != "" && connection->isBus()) {
    QDBusServiceWatcher* watcher = new QDBusServiceWatcher(
        uniqueConnectionName, connection->connection(),
        QDBusServiceWatcher::WatchForUnregistration, this);
    connect(watcher, &QDBusServiceWatcher::serviceUnregistered, this,
            &QObject::deleteLater);
    auto reply = connection->connection().interface()->isServiceRegistered(
        uniqueConnectionName);
    if (!reply.isValid() || !reply.value()) deleteLater();
  }
}
Client::~Client() {}

void Client::decRefCount(QDBusObjectPath o) {
  QSharedPointer<ExportedObject> obj = RefCountedObject::tryLookupObject(o);
  if (!obj) {
    throw Exception("de.uni_stuttgart.Voxie.ObjectNotFound",
                    "Cannot find object " + o.path());
  }

  QMap<QObject*, Reference>::iterator it = references.find(obj.data());
  if (it == references.end())
    throw Exception("de.uni_stuttgart.Voxie.ReferenceNotFound",
                    "Client " + getPath().path() +
                        " does not hold a reference to object " + o.path());

  if (!--it->refCount) {
    references.remove(obj.data());
  }
  return;
}

void Client::incRefCount(QDBusObjectPath o) {
  QSharedPointer<ExportedObject> obj = RefCountedObject::tryLookupObject(o);
  if (!obj)
    throw Exception("de.uni_stuttgart.Voxie.ObjectNotFound",
                    "Object " + o.path() + " not found");
  incRefCount(obj);
}

void Client::incRefCount(const QSharedPointer<ExportedObject>& obj) {
  QMap<QObject*, Reference>::iterator it = references.find(obj.data());
  if (it == references.end()) {
    Reference ref;
    ref.refCount = 1;
    ref.target = obj;
    references.insert(obj.data(), ref);
  } else {
    if (it->refCount == std::numeric_limits<quint64>::max())
      throw Exception("de.uni_stuttgart.Voxie.Overflow",
                      "Reference counter overflow");
    it->refCount++;
  }
}

QMap<QDBusObjectPath, quint64> Client::getReferencedObjects() {
  QMap<QDBusObjectPath, quint64> map;

  for (const auto& ref : references) {
    map.insert(ref.target->getPath(), ref.refCount);
  }

  return map;
}
