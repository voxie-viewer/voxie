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

#include <VoxieClient/DBusProxies.hpp>
#include <VoxieClient/Exception.hpp>
#include <VoxieClient/RefCountHolder.hpp>
#include <VoxieClient/VoxieClient.hpp>

#include <QtCore/QCommandLineOption>
#include <QtCore/QCommandLineParser>

#include <QtDBus/QDBusConnection>

namespace vx {
class VOXIECLIENT_EXPORT DBusClient {
  QDBusConnection connection_;
  QString uniqueName_;
  QSharedPointer<de::uni_stuttgart::Voxie::Instance> instance_;
  QSharedPointer<de::uni_stuttgart::Voxie::ClientManager> clientManager_;
  QDBusObjectPath clientPath_;
  QSharedPointer<de::uni_stuttgart::Voxie::Client> client_;

 public:
  static const QCommandLineOption& voxieBusAddressOption();
  static const QCommandLineOption& voxieBusNameOption();
  static const QCommandLineOption& voxiePeerAddressOption();
  static const QList<QCommandLineOption>& options();

  DBusClient(const QCommandLineParser& options);
  ~DBusClient();

  DBusClient(const DBusClient&) = delete;
  DBusClient& operator=(const DBusClient&) = delete;

  QDBusConnection connection() { return connection_; }
  const QString& uniqueName() const { return uniqueName_; }
  de::uni_stuttgart::Voxie::Instance* instance() { return instance_.data(); }
  de::uni_stuttgart::Voxie::ClientManager* clientManager() {
    return clientManager_.data();
  }
  const QDBusObjectPath& clientPath() const { return clientPath_; }
  const QSharedPointer<de::uni_stuttgart::Voxie::Client>& client() const {
    return client_;
  }

  de::uni_stuttgart::Voxie::Instance* operator->() { return instance(); }
};

// This class is fully defined in the .hpp file, is not marked with
// VOXIECLIENT_EXPORT
template <typename IntfType>
class RefObjWrapper {
  RefCountHolder refCnt;
  QDBusObjectPath path_;
  IntfType interface_;

 public:
  RefObjWrapper(DBusClient& client, const QDBusObjectPath& path)
      : refCnt(client.client(), path),
        path_(path),
        interface_(client.uniqueName(), path.path(), client.connection()) {
    if (!interface_.isValid())
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Error while getting external object " + path.path() +
                              ": " + interface_.lastError().name() + ": " +
                              interface_.lastError().message());
  }
  ~RefObjWrapper() {}

  const QDBusObjectPath& path() const { return path_; }

  IntfType* interface() { return &interface_; }
  IntfType* operator->() { return interface(); }
};
}  // namespace vx
