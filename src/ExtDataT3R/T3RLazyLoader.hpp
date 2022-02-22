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

#include <QDBusObjectPath>
#include <QObject>
#include <QSharedPointer>

#include <VoxieClient/ClaimedOperation.hpp>
#include <VoxieClient/DBusClient.hpp>
#include <VoxieClient/DBusProxies.hpp>

namespace vx {

class DBusClient;

namespace t3r {

class EventListProvider;
class EventListReader;

class LazyLoader : public QObject {
  Q_OBJECT

 public:
  using OperationPointer = QSharedPointer<
      vx::ClaimedOperation<de::uni_stuttgart::Voxie::ExternalOperationImport>>;

  LazyLoader(vx::DBusClient& dbusClient);
  virtual ~LazyLoader();

  void openSingleStream(QString filename, OperationPointer op);
  void openMultiStream(QString filename, OperationPointer op);

  void createAccessor();

  const QDBusObjectPath& getAccessorPath() const;
  const QDBusObjectPath& getVersionPath() const;

 private:
  void openStreamImpl(QString filename, QMap<QString, QDBusVariant> metadata,
                      float minProgress, float maxProgress,
                      OperationPointer op);
  void buildCache(EventListReader& reader, QString cacheFilename);

  void error(const QString& str);

  vx::DBusClient& dbusClient;
  QSharedPointer<EventListProvider> provider;

  QSharedPointer<vx::RefObjWrapper<de::uni_stuttgart::Voxie::Data>> dataWrapper;
  QSharedPointer<vx::RefObjWrapper<de::uni_stuttgart::Voxie::DataVersion>>
      versionWrapper;
};

}  // namespace t3r
}  // namespace vx
