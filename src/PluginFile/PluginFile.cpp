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

#include "PluginFile.hpp"

#include <PluginFile/ExportEventListCsv.hpp>
#include <PluginFile/ExportFtr.hpp>
#include <PluginFile/ExportPly.hpp>
#include <PluginFile/ExportStl.hpp>
#include <PluginFile/ExportTableCsv.hpp>

using namespace vx::file;

PluginFile::PluginFile() : QGenericPlugin() {}

QObject* PluginFile::create(const QString& key, const QString& specification) {
  (void)key;
  (void)specification;
  return nullptr;
}

QList<QSharedPointer<vx::io::Exporter>> PluginFile::exporters() {
  // qDebug() << "PluginFile::exporters()";
  QList<QSharedPointer<vx::io::Exporter>> list;
  list.append(makeSharedQObject<ExportFtr>());
  // Note: This exporter is unused (ExtFilePly is used instead)
  // list.append(makeSharedQObject<ExportPly>());
  list.append(makeSharedQObject<ExportStl>());
  list.append(makeSharedQObject<ExportTableCsv>());
  list.append(makeSharedQObject<ExportEventListCsv>());
  return list;
}
