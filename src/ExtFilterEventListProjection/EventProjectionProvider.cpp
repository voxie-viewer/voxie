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

#include "EventProjectionProvider.hpp"

#include "SpecialImageProvider.hpp"

// TODO fix include path
#include "../ExtDataT3R/EventListBufferGroup.hpp"
#include "../ExtFilterEventListClustering/DBusNumericUtil.hpp"

#include <VoxieClient/DBusAdaptors.hpp>
#include <VoxieClient/DBusUtil.hpp>
#include <VoxieClient/JsonDBus.hpp>
#include <VoxieClient/ObjectExport/DBusCallUtil.hpp>
#include <VoxieClient/QtUtil.hpp>
#include <VoxieClient/VoxieDBus.hpp>

#include <QtConcurrent/QtConcurrent>
#include <QtCore/QByteArray>
#include <QtCore/QFile>
#include <QtCore/QFuture>
#include <QtGui/QColor>
#include <QtGui/QImage>

using namespace vx;
using namespace vx::t3r;

class TomographyRawData2DAccessorOperationsAdaptorImpl
    : public TomographyRawData2DAccessorOperationsAdaptor {
 public:
  using Opt = QMap<QString, QDBusVariant>;

  TomographyRawData2DAccessorOperationsAdaptorImpl(
      EventProjectionProvider* provider)
      : TomographyRawData2DAccessorOperationsAdaptor(provider),
        provider(provider) {}

 public Q_SLOTS:
  QStringList GetAvailableGeometryTypes(const Opt& options) override {
    try {
      ExportedObject::checkOptions(options);
      return QStringList("ConeBeamCT");
    } catch (Exception& e) {
      e.handle(provider);
      return QStringList();
    }
  }

  QList<Opt> GetAvailableImageKinds(const Opt& options) override {
    try {
      ExportedObject::checkOptions(options);
      auto kinds = QList<Opt>();
      // TODO support/advertise different kinds of images?
      kinds.append(vx::emptyOptions());
      return kinds;
    } catch (Exception& e) {
      e.handle(provider);
      return QList<Opt>();
    }
  }

  QStringList GetAvailableStreams(const Opt& options) override {
    try {
      ExportedObject::checkOptions(options);
      // TODO return different stream names?
      return QStringList("");
    } catch (Exception& e) {
      e.handle(provider);
      return QStringList();
    }
  }

  QMap<QString, QDBusVariant> GetGeometryData(const QString& geometryType,
                                              const Opt& options) override {
    try {
      ExportedObject::checkOptions(options);
      // TODO return geometry data?
      if (geometryType != "ConeBeamCT") {
        throw vx::Exception(
            "de.uni_stuttgart.Voxie.ExtFilterEventListProjection.Error",
            "Invalid geometry type");
      }
      return QMap<QString, QDBusVariant>();
    } catch (Exception& e) {
      return e.handle(provider);
    }
  }

  std::tuple<quint64, quint64> GetImageShape(const QString& stream,
                                             qulonglong id,
                                             const Opt& options) override {
    Q_UNUSED(id);
    try {
      ExportedObject::checkOptions(options);
      if (stream != "") {
        throw vx::Exception(
            "de.uni_stuttgart.Voxie.ExtFilterEventListProjection.Error",
            "Invalid stream identifier");
      }
      return std::tuple<quint64, quint64>(
          provider->getProjectionSettings().imageWidth,
          provider->getProjectionSettings().imageHeight);
    } catch (Exception& e) {
      e.handle(provider);
      return std::tuple<quint64, quint64>(0, 0);
    }
  }

  QMap<QString, QDBusVariant> GetMetadata(const Opt& options) override {
    try {
      ExportedObject::checkOptions(options);
      // TODO return metadata?
      return QMap<QString, QDBusVariant>();
    } catch (Exception& e) {
      return e.handle(provider);
    }
  }

  qulonglong GetNumberOfImages(const QString& stream,
                               const Opt& options) override {
    try {
      ExportedObject::checkOptions(options);
      if (stream != "") {
        throw vx::Exception(
            "de.uni_stuttgart.Voxie.ExtFilterEventListProjection.Error",
            "Invalid stream identifier");
      }
      return provider->getStreamCount();
    } catch (Exception& e) {
      e.handle(provider);
      return 0;
    }
  }

  QMap<QString, QDBusVariant> GetPerImageMetadata(const QString& stream,
                                                  qulonglong id,
                                                  const Opt& options) override {
    try {
      Q_UNUSED(id);
      ExportedObject::checkOptions(options);
      if (stream != "") {
        throw vx::Exception(
            "de.uni_stuttgart.Voxie.ExtFilterEventListProjection.Error",
            "Invalid stream identifier");
      }
      return provider->getStreamMetadata(id);
    } catch (Exception& e) {
      return e.handle(provider);
    }
  }

  QString ReadImages(const Opt& imageKind,
                     const QList<std::tuple<QString, quint64>>& images,
                     const std::tuple<quint64, quint64>& inputRegionStart,
                     const std::tuple<QString, QDBusObjectPath>& output,
                     qulonglong firstOutputImageId,
                     const std::tuple<quint64, quint64>& outputRegionStart,
                     const std::tuple<quint64, quint64>& regionSize,
                     const Opt& options) override {
    try {
      // TODO check/support image kind?
      Q_UNUSED(imageKind);

      ProjectionRegionSettings region;
      region.outputImageID = firstOutputImageId;
      region.outputX = std::get<0>(outputRegionStart);
      region.outputY = std::get<1>(outputRegionStart);
      region.inputX = std::get<0>(inputRegionStart);
      region.inputY = std::get<1>(inputRegionStart);
      region.width = std::get<0>(regionSize);
      region.height = std::get<1>(regionSize);

      ExportedObject::checkOptions(options, "AllowIncompleteData");

      auto allowIncompleteData = ExportedObject::getOptionValueOrDefault<bool>(
          options, "AllowIncompleteData", false);

      // TODO support AllowIncompleteData = false
      if (!allowIncompleteData) {
        throw vx::Exception(
            "de.uni_stuttgart.Voxie.ExtFilterEventListProjection.Error",
            "Support for AllowIncompleteData = false is not yet implemented");
      }

      // TODO support more than 1 image
      if (images.size() != 1) {
        throw vx::Exception(
            "de.uni_stuttgart.Voxie.ExtFilterEventListProjection.Error",
            "Exactly 1 image must be specified for ReadImages");
      }

      QString stream = std::get<0>(images[0]);
      quint64 imageID = std::get<1>(images[0]);

      if (stream != "") {
        throw vx::Exception(
            "de.uni_stuttgart.Voxie.ExtFilterEventListProjection.Error",
            "Invalid stream identifier");
      }

      vx::handleDBusCallOnBackgroundThread(
          provider, [this, imageID, region, output]() {
            auto version = provider->projectStream(
                imageID, region, std::get<0>(output), std::get<1>(output));

            return QList<QDBusVariant>{vx::dbusMakeVariant<QString>(version)};
          });
    } catch (Exception& e) {
      e.handle(provider);
    }
    return "";
  }

 private:
  EventProjectionProvider* provider;
};

EventProjectionProvider::EventProjectionProvider(vx::DBusClient& dbusClient)
    : RefCountedObject("EventListProjection"), dbusClient(dbusClient) {
  new TomographyRawData2DAccessorOperationsAdaptorImpl(this);
}

void EventProjectionProvider::setInputAccessor(
    QSharedPointer<de::uni_stuttgart::Voxie::EventListDataAccessorOperations>
        accessor) {
  this->accessor = accessor;

  streamCount = accessor->streamCount();
  streams.resize(streamCount);
  for (StreamID id = 0; id < streamCount; ++id) {
    QList<std::tuple<QString, std::tuple<QString, quint32, QString>, QString,
                     QMap<QString, QDBusVariant>, QMap<QString, QDBusVariant>>>
        attributes;
    qlonglong minimumTimestamp = 0, maximumTimestamp = 0;
    HANDLEDBUSPENDINGREPLY(accessor->GetStreamInfo(
        id, vx::emptyOptions(), minimumTimestamp, maximumTimestamp, attributes,
        streams[id].metadata));
  }
}

QSharedPointer<de::uni_stuttgart::Voxie::EventListDataAccessorOperations>
EventProjectionProvider::getInputAccessor() const {
  return accessor;
}

void EventProjectionProvider::setProjectionSettings(
    ProjectionSettings projectionSettings) {
  this->projectionSettings = projectionSettings;

  if (projectionSettings.enableWhiteImage && !whiteImageProvider) {
    whiteImageProvider = decltype(whiteImageProvider)::create(
        *this, projectionSettings.imageWidth, projectionSettings.imageHeight);
  }
}

const ProjectionSettings& EventProjectionProvider::getProjectionSettings()
    const {
  return projectionSettings;
}

quint64 EventProjectionProvider::getStreamCount() const { return streamCount; }

QMap<QString, QDBusVariant> EventProjectionProvider::getStreamMetadata(
    StreamID streamID) const {
  if (streamID >= streams.size()) {
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.ExtFilterEventListProjection.Error",
        QString("Image ID %1 out of range").arg(streamID));
  }

  return streams[streamID].metadata;
}

QString EventProjectionProvider::projectStream(qulonglong streamID,
                                               ProjectionRegionSettings region,
                                               QString service,
                                               QDBusObjectPath outputPath) {
  if (streamID >= streams.size()) {
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.ExtFilterEventListProjection.Error",
        QString("Image ID %1 out of range").arg(streamID));
  }

  if (whiteImageProvider) {
    whiteImageProvider->initialize("WhiteImage");
  }

  // Initialize output proxy
  auto proxy = vx::executeOnMainThread([&]() {
    return makeSharedQObject<
        de::uni_stuttgart::Voxie::TomographyRawData2DRegular>(
        service, outputPath.path(), dbusClient.connection());
  });

  auto proxyData = vx::executeOnMainThread([&]() {
    return makeSharedQObject<de::uni_stuttgart::Voxie::Data>(
        service, outputPath.path(), dbusClient.connection());
  });

  // Create/finish update to get initial version string and set the progress to
  // 0%
  QString versionString;
  {
    versionString =
        (*finishUpdate(createUpdate(proxyData), false, 0.0))->versionString();
  }

  // TODO this should be done on the task itself, storing the "Incomplete" flag
  // in the TomographyRawData2DRegular's metadata
  streams[streamID].activeTasks++;

  auto task = vx::executeOnMainThread([&]() {
    // The task will delete itself (using deleteLater) once it is complete
    return new EventProjectionTask(*this);
  });

  // Reduce active task counter on stream once task is complete
  QObject::connect(task, &EventProjectionTask::destroyed, this,
                   [this, streamID]() { streams[streamID].activeTasks--; });

  if (whiteImageProvider && whiteImageProvider->isValid()) {
    task->setWhiteImageProvider(whiteImageProvider);
  }

  task->setStreamID(streamID);
  task->setOutputAccessor(proxy, proxyData);
  task->setRegionSettings(region);
  task->projectStream();

  return versionString;
}

QSharedPointer<vx::RefObjWrapper<de::uni_stuttgart::Voxie::ExternalDataUpdate>>
EventProjectionProvider::createUpdate(
    QSharedPointer<de::uni_stuttgart::Voxie::Data> proxyData) {
  return QSharedPointer<
      vx::RefObjWrapper<de::uni_stuttgart::Voxie::ExternalDataUpdate>>::
      create(dbusClient, HANDLEDBUSPENDINGREPLY(proxyData->CreateUpdate(
                             dbusClient.clientPath(), vx::emptyOptions())));
}

QSharedPointer<vx::RefObjWrapper<de::uni_stuttgart::Voxie::DataVersion>>
EventProjectionProvider::finishUpdate(
    QSharedPointer<
        vx::RefObjWrapper<de::uni_stuttgart::Voxie::ExternalDataUpdate>>
        update,
    bool complete, double progress,
    const QSharedPointer<vx::Exception>& error) {
  auto options = vx::emptyOptions();
  if (!complete) {
    QJsonObject status{
        {"Progress", progress},
    };
    if (error)
      status["Error"] = QJsonObject{
          {"Name", error->name()},
          {"Message", error->message()},
      };
    options["Metadata"] =
        dbusMakeVariant<QMap<QString, QDBusVariant>>(jsonToDBus(QJsonObject{
            {"Status", status},
        }));
  }

  return QSharedPointer<
      vx::RefObjWrapper<de::uni_stuttgart::Voxie::DataVersion>>::
      create(dbusClient, HANDLEDBUSPENDINGREPLY((*update)->Finish(
                             dbusClient.clientPath(), options)));
}
