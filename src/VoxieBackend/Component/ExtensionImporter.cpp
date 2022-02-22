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

#include "ExtensionImporter.hpp"

#include <VoxieBackend/IO/Operation.hpp>
#include <VoxieBackend/IO/OperationImport.hpp>
#include <VoxieBackend/IO/OperationRegistry.hpp>
#include <VoxieBackend/IO/OperationResult.hpp>

#include <VoxieBackend/Component/Extension.hpp>
#include <VoxieBackend/Component/ExternalOperation.hpp>

#include <VoxieBackend/Property/PropertyBase.hpp>
#include <VoxieBackend/Property/PropertyType.hpp>

#include <VoxieClient/SharedFunPtr.hpp>

#include <QtCore/QFileInfo>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>

using namespace vx;
using namespace vx::io;

QSharedPointer<vx::io::ExtensionImporter> vx::io::parseImporter(
    const QJsonObject& json,
    const vx::SharedFunPtr<QList<QSharedPointer<PropertyBase>>(
        const QJsonObject&)>& parseProperties) {
  QString name = json["Name"].toString();
  QString displayName = json["DisplayName"].toString();
  QString description = json["Description"].toString();
  QList<QString> patterns;
  for (const auto& entry : json["Patterns"].toArray())
    patterns << entry.toString();
  auto properties = parseProperties(json["Properties"].toObject());

  (void)description;  // TODO: use description?

  FilenameFilter filter(displayName, patterns);

  auto importer =
      makeSharedQObject<ExtensionImporter>(name, filter, properties);
  importer->setSelf(importer);
  return importer;
}

ExtensionImporter::ExtensionImporter(
    const QString& name, const FilenameFilter& filter,
    const QList<QSharedPointer<PropertyBase>>& properties)
    : Importer(name, filter, properties) {}
ExtensionImporter::~ExtensionImporter() {}

QSharedPointer<OperationResultImport> ExtensionImporter::import(
    const QString& fileName, bool doCreateNode,
    const QMap<QString, QVariant>& properties) {
  // Use absolute filename because script uses a different working directory
  auto absFileName = QFileInfo(fileName).absoluteFilePath();

  auto op = OperationImport::create();
  auto result = OperationImport::ResultWrapper::create(op, doCreateNode);

  op->setDescription("Import " + fileName);
  OperationRegistry::instance()->addOperation(op);

  QMap<QString, QDBusVariant> propertiesDBus;
  for (const auto& key : properties.keys()) {
    auto prop = getProperty(key, false);
    propertiesDBus[key] = prop->type()->rawToDBus(properties[key]);
  }
  for (const auto& prop : this->properties()) {
    if (!propertiesDBus.contains(prop->name()))
      throw vx::Exception(
          "de.uni_stuttgart.Voxie.InternalError",
          "Missing importer property value for " + prop->name());
  }

  auto exOp = ExternalOperationImport::create(op, absFileName, propertiesDBus,
                                              "Import " + fileName);

  auto ext = qSharedPointerDynamicCast<Extension>(this->container());
  // Should always be in an extension
  if (!ext) {
    throw Exception("de.uni_stuttgart.Voxie.InternalError",
                    "extension is nullptr in importData()");
  }

  // TODO: Use Extension::start()
  QStringList args;
  args << "--voxie-import-filename=" + absFileName;

  // if (debuggerSupportEnabled->isChecked())
  //   ext->startOperationDebug(exOp, args);
  // else
  ext->startOperation(exOp, args);

  return result;
}
