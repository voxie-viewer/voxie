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

#include "ClientWrapper.hpp"

#include <VoxieClient/Exception.hpp>
#include <VoxieClient/QtUtil.hpp>

using namespace vx;

ClientWrapper::ClientWrapper(const QSharedPointer<BusConnection>& connection,
                             const QString& uniqueConnectionName)
    : connection_(connection), uniqueConnectionName_(uniqueConnectionName) {
  cmProxy = makeSharedQObject<de::uni_stuttgart::Voxie::ClientManager>(
      uniqueConnectionName, "/de/uni_stuttgart/Voxie/ClientManager",
      this->connection()->connection());
  clientPath = HANDLEDBUSPENDINGREPLY(
      cmProxy->CreateClient(QMap<QString, QDBusVariant>()));
  clientProxy = makeSharedQObject<de::uni_stuttgart::Voxie::Client>(
      uniqueConnectionName, clientPath.path(),
      this->connection()->connection());
}
ClientWrapper::~ClientWrapper() {
  auto res = cmProxy->DestroyClient(clientPath, QMap<QString, QDBusVariant>());
  waitForDBusPendingCall(res);  // Ignore errors
}
const QSharedPointer<BusConnection>& ClientWrapper::connection() const {
  return connection_;
}
const QString& ClientWrapper::uniqueConnectionName() const {
  return uniqueConnectionName_;
}
const QSharedPointer<de::uni_stuttgart::Voxie::Client>&
ClientWrapper::client() {
  return clientProxy;
}
