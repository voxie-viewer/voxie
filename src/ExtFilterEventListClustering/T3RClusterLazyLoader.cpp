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

#include "T3RClusterLazyLoader.hpp"

#include "ClusteredEventListProvider.hpp"

#include <VoxieClient/DBusClient.hpp>
#include <VoxieClient/VoxieDBus.hpp>

namespace vx {
namespace t3r {

ClusterLazyLoader::ClusterLazyLoader(vx::DBusClient& dbusClient)
    : dbusClient(dbusClient),
      provider(ClusteredEventListProvider::create(dbusClient)) {
  QObject::connect(provider.data(), &QObject::destroyed,
                   QCoreApplication::quit);
}

ClusterLazyLoader::~ClusterLazyLoader() {}

void ClusterLazyLoader::setClusteringSettings(ClusteringSettings settings) {
  provider->setClusteringSettings(settings);
}

void ClusterLazyLoader::setInputAccessor(
    QSharedPointer<de::uni_stuttgart::Voxie::EventListDataAccessorOperations>
        accessor) {
  provider->setInputAccessor(accessor);
}

void ClusterLazyLoader::createOutputAccessor() {
  outputDataWrapper = outputDataWrapper.create(
      dbusClient,
      HANDLEDBUSPENDINGREPLY(dbusClient->CreateEventListDataAccessor(
          dbusClient.clientPath(),
          std::tuple<QString, QDBusObjectPath>(
              dbusClient.connection().baseService(), provider->getPath()),
          vx::emptyOptions())));

  versionWrapper = versionWrapper.create(
      dbusClient,
      HANDLEDBUSPENDINGREPLY((*outputDataWrapper)
                                 ->GetCurrentVersion(dbusClient.clientPath(),
                                                     vx::emptyOptions())));
}

const QDBusObjectPath& ClusterLazyLoader::getOutputAccessorPath() const {
  return outputDataWrapper->path();
}

const QDBusObjectPath& ClusterLazyLoader::getVersionPath() const {
  return versionWrapper->path();
}

void ClusterLazyLoader::error(const QString& str) {
  throw vx::Exception("de.uni_stuttgart.Voxie.ExtDataT3R.Error", str);
}

}  // namespace t3r
}  // namespace vx
