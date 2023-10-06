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

// QDBusConnection should be included as early as possible:
// https://bugreports.qt.io/browse/QTBUG-48351 /
// https://bugreports.qt.io/browse/QTBUG-48377
#include <QtDBus/QDBusConnection>

#include <Main/Root.hpp>
#include <VoxieClient/DBusAdaptors.hpp>

namespace vx {
class Utilities;

class Instance : public vx::ExportedObject {
  Q_OBJECT

  Root* root_;

  QSharedPointer<Utilities> utilities_;

 public:
  Instance(Root* root);
  ~Instance();

  Root* root() const { return root_; }
  const QSharedPointer<Utilities>& utilities() const { return utilities_; }
};

class InstanceAdaptorImpl : public InstanceAdaptor {
  Q_OBJECT

  Instance* object;
  Root* root;

 public:
  InstanceAdaptorImpl(Instance* object);
  ~InstanceAdaptorImpl();

  QDBusObjectPath gui() const override;
  QDBusObjectPath utilities() const override;
  QDBusObjectPath components() const override;

  QList<QDBusObjectPath> ListPrototypes(
      const QMap<QString, QDBusVariant>& options) override;

  QDBusObjectPath GetComponent(
      const QString& componentType, const QString& name,
      const QMap<QString, QDBusVariant>& options) override;

  QList<QDBusObjectPath> ListPlugins(
      const QMap<QString, QDBusVariant>& options) override;
  QDBusObjectPath GetPluginByName(
      const QString& name, const QMap<QString, QDBusVariant>& options) override;

  QList<QDBusObjectPath> ListNodes(
      const QMap<QString, QDBusVariant>& options) override;
  QList<QDBusObjectPath> ListObjects(
      const QMap<QString, QDBusVariant>& options) override;

  QDBusObjectPath CreateImage(
      const QDBusObjectPath& client, const vx::TupleVector<quint64, 2>& size,
      quint64 componentCount,
      const std::tuple<QString, quint32, QString>& dataType,
      const QMap<QString, QDBusVariant>& options) override;

  QDBusObjectPath CreateSurfaceDataTriangleIndexed(
      const QDBusObjectPath& client, qulonglong triangleCount,
      qulonglong vertexCount, const QDBusObjectPath& triangleSource,
      bool triangleWritable,
      const QMap<QString, QDBusVariant>& options) override;

  QDBusObjectPath CreateVolumeDataVoxel(
      const QDBusObjectPath& client, const vx::TupleVector<quint64, 3>& size,
      const std::tuple<QString, quint32, QString>& dataType,
      const vx::TupleVector<double, 3>& gridOrigin,
      const vx::TupleVector<double, 3>& gridSpacing,
      const QMap<QString, QDBusVariant>& options) override;

  QDBusObjectPath CreateTomographyRawData2DRegular(
      const QDBusObjectPath& client,
      const std::tuple<quint64, quint64>& imageShape, qulonglong imageCount,
      const std::tuple<QString, quint32, QString>& dataType,
      const QMap<QString, QDBusVariant>& options) override;

  QDBusObjectPath CreateTomographyRawData2DAccessor(
      const QDBusObjectPath& client,
      const std::tuple<QString, QDBusObjectPath>& provider,
      const QMap<QString, QDBusVariant>& options) override;

  QDBusObjectPath CreateEventListDataBuffer(
      const QDBusObjectPath& client, qulonglong capacity,
      const QList<std::tuple<QString, std::tuple<QString, quint32, QString>,
                             QString, QMap<QString, QDBusVariant>,
                             QMap<QString, QDBusVariant>>>& attributes,
      const QMap<QString, QDBusVariant>& options) override;

  QDBusObjectPath CreateEventListDataAccessor(
      const QDBusObjectPath& client,
      const std::tuple<QString, QDBusObjectPath>& backend,
      const QMap<QString, QDBusVariant>& options) override;

  QDBusObjectPath CreateTableData(
      const QDBusObjectPath& client,
      const QList<std::tuple<QString, QDBusObjectPath, QString,
                             QMap<QString, QDBusVariant>,
                             QMap<QString, QDBusVariant>>>& columns,
      const QMap<QString, QDBusVariant>& options) override;

  QDBusObjectPath CreateContainerData(
      const QDBusObjectPath& client, const QString& name,
      const QMap<QString, QDBusVariant>& options) override;

  QDBusObjectPath CreateVolumeSeriesData(
      const QDBusObjectPath& client, const QList<QDBusObjectPath>& dimensions,
      const vx::TupleVector<double, 3>& volumeOrigin,
      const vx::TupleVector<double, 3>& volumeSize,
      const QMap<QString, QDBusVariant>& options) override;

  QDBusObjectPath CreateSeriesDimension(
      const QDBusObjectPath& client, const QString& name,
      const QString& displayName, const QDBusObjectPath& type,
      const QDBusVariant& entries,
      const QMap<QString, QDBusVariant>& options) override;

  QDBusObjectPath CreateGeometricPrimitiveData(
      const QDBusObjectPath& client,
      const QMap<QString, QDBusVariant>& options) override;

  void Quit(const QMap<QString, QDBusVariant>& options) override;

  QDBusObjectPath OpenFile(const QString& file,
                           const QMap<QString, QDBusVariant>& options) override;
  QDBusObjectPath Import(const QDBusObjectPath& client, const QString& fileName,
                         const QMap<QString, QDBusVariant>& options) override;

  QMap<QString, QDBusVariant> versionInformation() const override;

  QDBusObjectPath RunAllFilters(
      const QDBusObjectPath& client,
      const QMap<QString, QDBusVariant>& options) override;

  QDBusObjectPath GetDebugOption(
      const QString& name, const QMap<QString, QDBusVariant>& options) override;
  QList<QDBusObjectPath> ListDebugOptions(
      const QMap<QString, QDBusVariant>& options) override;
};

}  // namespace vx
