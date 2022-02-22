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

#include "T3RLazyLoader.hpp"

#include "EventListCacheReader.hpp"
#include "EventListCacheWriter.hpp"
#include "EventListProvider.hpp"
#include "EventListReader.hpp"

#include <VoxieClient/DBusClient.hpp>
#include <VoxieClient/JsonDBus.hpp>
#include <VoxieClient/VoxieDBus.hpp>

namespace vx {
namespace t3r {

LazyLoader::LazyLoader(vx::DBusClient& dbusClient)
    : dbusClient(dbusClient), provider(EventListProvider::create(dbusClient)) {
  QObject::connect(provider.data(), &QObject::destroyed,
                   QCoreApplication::quit);
}

LazyLoader::~LazyLoader() {}

void LazyLoader::openSingleStream(QString filename, OperationPointer op) {
  openStreamImpl(filename, vx::emptyOptions(), 0.f, 1.f, op);
}

void LazyLoader::openMultiStream(QString filename, OperationPointer op) {
  QFile inputFile(filename, provider.data());
  if (!inputFile.open(QFile::ReadOnly)) {
    error(QString("Failed to open file '%1'").arg(filename));
  }

  auto json = QJsonDocument::fromJson(inputFile.readAll());
  if (!json.isObject()) {
    error(QString("Timepix JSON metadata must contain an object"));
  }

  TimepixRawMetadata rawMetadata;
  rawMetadata.read(json.object());

  QJsonObject metadata = rawMetadata.getMetadata();
  QDir workingDirectory = QFileInfo(inputFile).dir();
  metadata["WorkingDirectory"] = workingDirectory.absolutePath();
  provider->setMetadata(jsonToDBus(metadata));

  static const QString supportedType = "TimepixRawData";
  if (rawMetadata.getType() != supportedType) {
    error(
        QString("Timepix JSON metadata type mismatch (expected '%1', got '%2')")
            .arg(supportedType)
            .arg(rawMetadata.getType()));
  }

  std::size_t streamID = 0;
  std::size_t streamCount = rawMetadata.getTotalStreamCount();

  for (const auto& stream : rawMetadata.getStreams()) {
    for (const auto& dataFile : stream.getData()) {
      float minProgress = float(streamID) / streamCount;
      float maxProgress = float(streamID + 1) / streamCount;

      openStreamImpl(workingDirectory.filePath(dataFile),
                     jsonToDBus(stream.getMetadata()), minProgress, maxProgress,
                     op);
      streamID++;
    }
  }
}

void LazyLoader::createAccessor() {
  dataWrapper = dataWrapper.create(
      dbusClient,
      HANDLEDBUSPENDINGREPLY(dbusClient->CreateEventListDataAccessor(
          dbusClient.clientPath(),
          std::tuple<QString, QDBusObjectPath>(
              dbusClient.connection().baseService(), provider->getPath()),
          vx::emptyOptions())));

  versionWrapper = versionWrapper.create(
      dbusClient,
      HANDLEDBUSPENDINGREPLY((*dataWrapper)
                                 ->GetCurrentVersion(dbusClient.clientPath(),
                                                     vx::emptyOptions())));
}

const QDBusObjectPath& LazyLoader::getAccessorPath() const {
  return dataWrapper->path();
}

const QDBusObjectPath& LazyLoader::getVersionPath() const {
  return versionWrapper->path();
}

void LazyLoader::openStreamImpl(QString filename,
                                QMap<QString, QDBusVariant> metadata,
                                float minProgress, float maxProgress,
                                OperationPointer op) {
  HANDLEDBUSPENDINGREPLY(
      op->opGen().SetProgress(minProgress, vx::emptyOptions()));

  QFile* inputFile = new QFile(filename, provider.data());
  if (!inputFile->open(QFile::ReadOnly)) {
    error(QString("Failed to open file '%1'").arg(filename));
  }

  auto reader = QSharedPointer<EventListReader>::create(inputFile);
  auto cacheReader = QSharedPointer<EventListCacheReader>::create();

  auto cacheFilename = filename + "cache";
  QFile cacheFile(cacheFilename);

  if (!cacheFile.open(QFile::ReadOnly) || !cacheReader->open(&cacheFile)) {
    cacheFile.close();
    // TODO set intermediate progress during cache-building process
    buildCache(*reader, cacheFilename);
    if (!cacheFile.open(QFile::ReadOnly) || !cacheReader->open(&cacheFile)) {
      error(QString("Failed to open file '%1'").arg(filename));
    }
  }

  EventListProvider::Stream stream;
  stream.reader = reader;
  stream.cache = cacheReader;
  stream.metadata = metadata;
  provider->addStream(stream);

  HANDLEDBUSPENDINGREPLY(
      op->opGen().SetProgress(maxProgress, vx::emptyOptions()));
}

void LazyLoader::buildCache(EventListReader& reader, QString cacheFilename) {
  QFile cacheWriteFile(cacheFilename);
  if (!cacheWriteFile.open(QFile::WriteOnly | QFile::Truncate)) {
    error(QString("Failed to open file '%1' for writing").arg(cacheFilename));
  }

  EventListCacheWriter cacheWriter(reader, &cacheWriteFile);
  cacheWriter.write();
  cacheWriteFile.close();
}

void LazyLoader::error(const QString& str) {
  throw vx::Exception("de.uni_stuttgart.Voxie.ExtDataT3R.Error", str);
}

}  // namespace t3r
}  // namespace vx
