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

#include "DBusClient.hpp"

#include <VoxieClient/DBusConnectionUtil.hpp>
#include <VoxieClient/Exception.hpp>
#include <VoxieClient/QtUtil.hpp>

#include <QtDBus/QDBusConnectionInterface>

using namespace vx;

namespace vx {
namespace {
QCommandLineOption voxieBusAddressOptionValue(
    "voxie-bus-address", "Address of the bus to use to connect to Voxie.",
    "bus-address");
QCommandLineOption voxieBusNameOptionValue("voxie-bus-name",
                                           "Bus name of voxie.", "bus-name");
QCommandLineOption voxiePeerAddressOptionValue(
    "voxie-peer-address",
    "Address of the DBus peer to use to connect to Voxie.", "peer-address");
QList<QCommandLineOption> optionsValue{voxieBusAddressOptionValue,
                                       voxieBusNameOptionValue,
                                       voxiePeerAddressOptionValue};
}  // namespace
}  // namespace vx

const QCommandLineOption& DBusClient::voxieBusAddressOption() {
  return voxieBusAddressOptionValue;
}
const QCommandLineOption& DBusClient::voxieBusNameOption() {
  return voxieBusNameOptionValue;
}
const QCommandLineOption& DBusClient::voxiePeerAddressOption() {
  return voxiePeerAddressOptionValue;
}
const QList<QCommandLineOption>& DBusClient::options() { return optionsValue; }

DBusClient::DBusClient(const QCommandLineParser& options)
    : connection_(
          options.isSet(voxieBusAddressOption())
              ? QDBusConnection::connectToBus(
                    options.value(voxieBusAddressOption()), "voxie")
              : options.isSet(voxiePeerAddressOption())
                    ? QDBusConnection::connectToPeer(
                          options.value(voxiePeerAddressOption()), "voxie")
                    : QDBusConnection::sessionBus()) {
  // qDebug() << "CONN1" << connection_.connectionCapabilities();
  setupPeerDBusConnection(connection_);
  // qDebug() << "CONN2" << connection_.connectionCapabilities();
  if (!connection_.isConnected())
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.Error",
        "Connecting to DBus failed: " + connection_.lastError().name() + ": " +
            connection_.lastError().message());

  if (options.isSet(voxiePeerAddressOption())) {
    uniqueName_ = "";
  } else {
    if (options.isSet(voxieBusNameOption())) {
      uniqueName_ = options.value(voxieBusNameOption());
    } else {
      uniqueName_ = "de.uni_stuttgart.Voxie";
    }
    if (!uniqueName_.startsWith(":"))
      uniqueName_ = HANDLEDBUSPENDINGREPLY(
          connection_.interface()->serviceOwner(uniqueName_));
  }

  // TODO: The objects created by makeSharedQObject() will rely on the main loop
  // to clean them up, but there might not be any main loop. How should this be
  // handled?

  clientManager_ = makeSharedQObject<de::uni_stuttgart::Voxie::ClientManager>(
      uniqueName(), "/de/uni_stuttgart/Voxie/ClientManager", connection());
  clientManager_->setTimeout(INT_MAX);
  if (!clientManager_->isValid())
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Error while getting clientManager object: " +
                            clientManager_->lastError().name() + ": " +
                            clientManager_->lastError().message());

  instance_ = makeSharedQObject<de::uni_stuttgart::Voxie::Instance>(
      uniqueName(), "/de/uni_stuttgart/Voxie", connection());
  instance_->setTimeout(INT_MAX);
  if (!instance_->isValid())
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Error while getting instance object: " +
                            instance_->lastError().name() + ": " +
                            instance_->lastError().message());

  clientPath_ = HANDLEDBUSPENDINGREPLY(
      clientManager()->CreateClient(QMap<QString, QDBusVariant>()));
  client_ = makeSharedQObject<de::uni_stuttgart::Voxie::Client>(
      uniqueName(), clientPath().path(), connection());
  if (!client_->isValid())
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.Error",
        "Error while getting client object: " + client_->lastError().name() +
            ": " + client_->lastError().message());
}

DBusClient::~DBusClient() {
  auto reply = clientManager()->DestroyClient(clientPath_,
                                              QMap<QString, QDBusVariant>());
  if (reply.isError())
    qWarning() << "Error while destroying client:" << reply.error().name()
               << ":" << reply.error().message();
}
