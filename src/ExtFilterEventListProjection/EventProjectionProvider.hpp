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

#include "EventProjectionCommon.hpp"
#include "EventProjectionTask.hpp"
#include "ProjectionImage.hpp"
#include "ProjectionSettings.hpp"

#include <VoxieClient/DBusClient.hpp>
#include <VoxieClient/DBusProxies.hpp>
#include <VoxieClient/ObjectExport/ExportedObject.hpp>

#include <QPair>
#include <QSharedPointer>
#include <QThreadStorage>

#include <array>
#include <vector>

namespace vx {
namespace t3r {

class EventListBufferGroup;
class SpecialImageProvider;

class EventProjectionProvider : public RefCountedObject {
  Q_OBJECT
  REFCOUNTEDOBJ_DECL(EventProjectionProvider)

 public:
  EventProjectionProvider(vx::DBusClient& dbusClient);

  void setInputAccessor(
      QSharedPointer<de::uni_stuttgart::Voxie::EventListDataAccessorOperations>
          accessor);

  QSharedPointer<de::uni_stuttgart::Voxie::EventListDataAccessorOperations>
  getInputAccessor() const;

  void setProjectionSettings(ProjectionSettings projectionSettings);
  const ProjectionSettings& getProjectionSettings() const;

  quint64 getStreamCount() const;
  QMap<QString, QDBusVariant> getStreamMetadata(StreamID streamID) const;

  QString projectStream(qulonglong streamID, ProjectionRegionSettings region,
                        QString service, QDBusObjectPath outputPath);

  QSharedPointer<
      vx::RefObjWrapper<de::uni_stuttgart::Voxie::ExternalDataUpdate>>
  createUpdate(QSharedPointer<de::uni_stuttgart::Voxie::Data> proxyData);

  QSharedPointer<vx::RefObjWrapper<de::uni_stuttgart::Voxie::DataVersion>>
  finishUpdate(
      QSharedPointer<
          vx::RefObjWrapper<de::uni_stuttgart::Voxie::ExternalDataUpdate>>
          update,
      bool complete, double progress,
      const QSharedPointer<vx::Exception>& error =
          QSharedPointer<vx::Exception>());

  vx::DBusClient& dbusClient;

 private:
  QSharedPointer<de::uni_stuttgart::Voxie::EventListDataAccessorOperations>
      accessor;
  qulonglong streamCount = 0;

  struct StreamData {
    QMap<QString, QDBusVariant> metadata;
    QAtomicInt activeTasks;
  };

  std::vector<StreamData> streams;

  ProjectionSettings projectionSettings;
  QSharedPointer<SpecialImageProvider> whiteImageProvider;

  friend class EventProjectionWorker;
};

}  // namespace t3r
}  // namespace vx
