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

#include "ExtensionExporter.hpp"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>

#include <VoxieBackend/IO/Operation.hpp>
#include <VoxieBackend/IO/OperationRegistry.hpp>
#include <VoxieBackend/IO/OperationResult.hpp>

#include <VoxieBackend/Component/Extension.hpp>
#include <VoxieBackend/Component/ExternalOperation.hpp>

using namespace vx;
using namespace vx::io;

QSharedPointer<vx::io::ExtensionExporter> vx::io::parseExporter(
    const QJsonObject& json) {
  QString name = json["Name"].toString();
  QString displayName = json["DisplayName"].toString();
  QString description = json["Description"].toString();
  QList<QString> patterns;
  for (const auto& entry : json["Patterns"].toArray())
    patterns << entry.toString();
  QList<QString> targetPrototypeNames;
  for (const auto& targetName : json["TargetPrototypeNames"].toArray())
    targetPrototypeNames << targetName.toString();

  (void)description;  // TODO: use description?

  FilenameFilter filter(displayName, patterns);

  auto exporter =
      makeSharedQObject<ExtensionExporter>(name, filter, targetPrototypeNames);
  exporter->setSelf(exporter);
  return exporter;
}

ExtensionExporter::ExtensionExporter(const QString& name,
                                     const FilenameFilter& filter,
                                     const QList<QString>& targetPrototypeNames)
    : Exporter(name, filter, targetPrototypeNames) {}
ExtensionExporter::~ExtensionExporter() {}

QSharedPointer<OperationResult> ExtensionExporter::exportData(
    const QSharedPointer<vx::Data>& data, const QString& fileName) {
  auto op = Operation::create();
  auto result = OperationResult::create(op);

  op->setDescription("Export " + fileName);
  OperationRegistry::instance()->addOperation(op);

  auto exOp =
      ExternalOperationExport::create(op, fileName, "Export " + fileName, data);

  auto ext = qSharedPointerDynamicCast<Extension>(this->container());
  // Should always be in an extension
  if (!ext) {
    throw Exception("de.uni_stuttgart.Voxie.InternalError",
                    "extension is nullptr in exportData()");
  }
  // if (debuggerSupportEnabled->isChecked())
  //   ext->startOperationDebug(exOp);
  // else
  ext->startOperation(exOp);

  return result;
}
