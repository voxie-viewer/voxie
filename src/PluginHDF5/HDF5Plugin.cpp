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

// QDBusConnection should be included as early as possible:
// https://bugreports.qt.io/browse/QTBUG-48351 /
// https://bugreports.qt.io/browse/QTBUG-48377
#include <QtDBus/QDBusConnection>

#include "HDF5Plugin.hpp"

#include <PluginHDF5/HDFExporter.hpp>
#include <PluginHDF5/HDFImporter.hpp>
#include <PluginHDF5/HDFSliceExporter.hpp>

using namespace vx::io;
using namespace vx::plugin;

HDF5Plugin::HDF5Plugin(QObject* parent) : QGenericPlugin(parent) {}

QObject* HDF5Plugin::create(const QString& key, const QString& specification) {
  (void)key;
  (void)specification;
  return nullptr;
}

QList<QSharedPointer<Importer>> HDF5Plugin::importers() {
  QList<QSharedPointer<vx::io::Importer>> list;
  list.append(makeSharedQObject<HDFImporter>());
  return list;
}

QList<QSharedPointer<vx::io::Exporter>> HDF5Plugin::exporters() {
  QList<QSharedPointer<vx::io::Exporter>> list;
  list.append(makeSharedQObject<HDFExporter>());
  return list;
}

QList<QSharedPointer<SliceExporter>> HDF5Plugin::sliceExporters() {
  QList<QSharedPointer<vx::io::SliceExporter>> list;
  list.append(makeSharedQObject<HDFSliceExporter>());
  return list;
}
