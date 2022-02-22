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

#include "ClientManager.hpp"

#include <VoxieClient/ObjectExport/BusManager.hpp>
#include <VoxieClient/ObjectExport/Client.hpp>

using namespace vx;

class ClientManagerAdaptorImpl : public ClientManagerAdaptor {
  Q_OBJECT

  ClientManager* object;

 public:
  ClientManagerAdaptorImpl(ClientManager* object)
      : ClientManagerAdaptor(object), object(object) {}
  ~ClientManagerAdaptorImpl() {}

  QDBusObjectPath CreateClient(
      const QMap<QString, QDBusVariant>& options) override;
  QDBusObjectPath CreateIndependentClient(
      const QMap<QString, QDBusVariant>& options) override;
  void DestroyClient(const QDBusObjectPath& client,
                     const QMap<QString, QDBusVariant>& options) override;
};

QDBusObjectPath ClientManagerAdaptorImpl::CreateClient(
    const QMap<QString, QDBusVariant>& options) {
  try {
    ExportedObject::checkOptions(options);

    if (!object->calledFromDBus()) {
      throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                          "ClientManagerAdaptorImpl::CreateClient() called "
                          "from non-DBus context");
    }

    auto connection = getBusManager()->findConnection(object->connection());
    QString name = object->message().service();

    if (connection->isBus()) {
      if (name == "")
        throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                            "Got empty service name on bus connection");
    } else {
      if (name != "")
        qWarning() << "Got non-empty service name for connection"
                   << connection->connectionName() << ":" << name;
    }

    Client* client = new Client(this, name, connection);

    return ExportedObject::getPath(client);
  } catch (Exception& e) {
    e.handle(object);
    return ExportedObject::getPath(nullptr);
  }
}

QDBusObjectPath ClientManagerAdaptorImpl::CreateIndependentClient(
    const QMap<QString, QDBusVariant>& options) {
  try {
    ExportedObject::checkOptions(options);

    auto connection = getBusManager()->findConnection(object->connection());

    Client* client = new Client(this, "", connection);

    return ExportedObject::getPath(client);
  } catch (Exception& e) {
    e.handle(object);
    return ExportedObject::getPath(nullptr);
  }
}

void ClientManagerAdaptorImpl::DestroyClient(
    const QDBusObjectPath& client, const QMap<QString, QDBusVariant>& options) {
  try {
    ExportedObject::checkOptions(options);

    Client* clientPtr =
        qobject_cast<Client*>(ExportedObject::lookupWeakObject(client));

    if (!clientPtr) {
      throw Exception("de.uni_stuttgart.Voxie.ClientNotFound",
                      "The client cannot be found, might be already destroyed");
    }

    delete clientPtr;

    return;
  } catch (Exception& e) {
    e.handle(object);
    return;
  }
}

ClientManager::ClientManager()
    : ExportedObject("ClientManager", nullptr, true) {
  new ClientManagerAdaptorImpl(this);
}
ClientManager::~ClientManager() {}

#include "ClientManager.moc"
