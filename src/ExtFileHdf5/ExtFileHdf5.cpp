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

#include <ExtFileHdf5/HDFExporter.hpp>
#include <ExtFileHdf5/HDFImporter.hpp>

#include <VoxieClient/ObjectExport/BusManager.hpp>
#include <VoxieClient/ObjectExport/ClientManager.hpp>

Q_NORETURN static void error(const QString& str) {
  throw vx::Exception("de.uni_stuttgart.Voxie.FileFormat.Hdf5.Error", str);
}

QCommandLineOption voxieImportFilename("voxie-import-filename", "File to load.",
                                       "filename");

int main(int argc, char* argv[]) {
  try {
    if (argc < 1) error("argc is smaller than 1");

    QCommandLineParser parser;
    parser.setApplicationDescription(
        "Voxie extension for accessing HDF5 files");
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addOptions(vx::DBusClient::options());
    parser.addOptions(vx::ClaimedOperationBase::options());
    parser.addOption(voxieImportFilename);

    QStringList args;
    for (char** arg = argv; *arg; arg++) args.push_back(*arg);
    int argc0 = 1;
    char* args0[2] = {argv[0], NULL};
    QCoreApplication app(argc0, args0);
    parser.process(args);

    QString action = "";
    if (parser.isSet(vx::ClaimedOperationBase::voxieActionOption()))
      action = parser.value(vx::ClaimedOperationBase::voxieActionOption());

    if (action == "Import") {
      if (!parser.isSet(vx::ClaimedOperationBase::voxieOperationOption()))
        error("--voxie-operation is not set");
      QString operationPath =
          parser.value(vx::ClaimedOperationBase::voxieOperationOption());

      vx::initDBusTypes();

      vx::DBusClient dbusClient(parser);

      auto op = std::make_unique<vx::ClaimedOperation<
          de::uni_stuttgart::Voxie::ExternalOperationImport>>(
          dbusClient, QDBusObjectPath(operationPath));

      // TODO: Support for peer-to-peer connections
      auto busManager = vx::getBusManager();
      busManager->addConnection(makeSharedQObject<vx::BusConnection>(
          QDBusConnection::sessionBus(), true));
      // auto dbusService =
      //     makeSharedQObject<vx::DBusServiceBus>(QDBusConnection::sessionBus());

      vx::ClientManager clientManager;

      op->forwardExc([&]() {
        auto data_version = import(dbusClient, *op);

        HANDLEDBUSPENDINGREPLY(op->op().Finish(std::get<0>(data_version).path(),
                                               std::get<1>(data_version).path(),
                                               vx::emptyOptions()));
      });

      op.reset();

      // TODO: Move this code somewhere else, use also e.g. for
      // EventListProjection
      if (busManager->haveNonSingletonObjects()) {
        QObject::connect(busManager,
                         &BusManager::allNonSingletonObjectsDestroyed, &app,
                         &QCoreApplication::quit);
        return app.exec();
      }
    } else if (action == "Export") {
      if (!parser.isSet(vx::ClaimedOperationBase::voxieOperationOption()))
        error("--voxie-operation is not set");
      QString operationPath =
          parser.value(vx::ClaimedOperationBase::voxieOperationOption());
      // if (!parser.isSet(voxieExportFilename))
      //   error("--voxie-export-filename is not set");
      // QString filename = parser.value(voxieExportFilename);

      vx::initDBusTypes();

      vx::DBusClient dbusClient(parser);

      vx::ClaimedOperation<de::uni_stuttgart::Voxie::ExternalOperationExport>
          op(dbusClient, QDBusObjectPath(operationPath));

      op.forwardExc([&]() {
        auto name = op.opGen().name();

        if (name == "de.uni_stuttgart.Voxie.FileFormat.Hdf5.ExportVolume") {
          exportDataVolume(dbusClient, op);
        } else if (name == "de.uni_stuttgart.Voxie.FileFormat.Hdf5.ExportRaw") {
          exportDataRaw(dbusClient, op);
        } else {
          error("Unknown exporter: " + name);
        }

        HANDLEDBUSPENDINGREPLY(op.op().Finish(vx::emptyOptions()));
      });
    } else {
      error("--voxie-action is not 'Import' or 'Export'");
    }

    return 0;
  } catch (vx::Exception& error) {
    QTextStream(stderr) << error.name() << ": " << error.message() << endl
                        << flush;
    return 1;
  }
}
