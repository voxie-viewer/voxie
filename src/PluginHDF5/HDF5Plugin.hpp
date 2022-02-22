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

#include <Voxie/Component/Interfaces.hpp>

#include <QtGui/QGenericPlugin>

class HDF5Plugin : public QGenericPlugin,
                   public vx::plugin::IImporterPlugin,
                   public vx::plugin::IExporterPlugin,
                   public vx::plugin::ISliceExportPlugin {
  Q_OBJECT
  Q_INTERFACES(vx::plugin::IImporterPlugin)
  Q_INTERFACES(vx::plugin::IExporterPlugin)
  Q_INTERFACES(vx::plugin::ISliceExportPlugin)

  Q_PLUGIN_METADATA(IID
                    "org.qt-project.Qt.QGenericPluginPrototypeInterface" FILE
                    "HDF5.json")

 public:
  HDF5Plugin(QObject* parent = 0);

  virtual QObject* create(const QString& key,
                          const QString& specification) override;

  virtual QList<QSharedPointer<vx::io::Importer>> importers() override;

  virtual QList<QSharedPointer<vx::io::Exporter>> exporters() override;

  virtual QList<QSharedPointer<vx::io::SliceExporter>> sliceExporters()
      override;
};
