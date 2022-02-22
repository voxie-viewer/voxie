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

#include <VoxieClient/ObjectExport/BusManager.hpp>
#include <VoxieClient/ObjectExport/Client.hpp>
#include <VoxieClient/ObjectExport/ClientManager.hpp>

#include <VoxieClient/ClaimedOperation.hpp>
#include <VoxieClient/DBusClient.hpp>
#include <VoxieClient/DBusProxies.hpp>
#include <VoxieClient/DBusUtil.hpp>
#include <VoxieClient/Exception.hpp>
#include <VoxieClient/Exceptions.hpp>
#include <VoxieClient/MappedBuffer.hpp>
#include <VoxieClient/QtUtil.hpp>
#include <VoxieClient/RefCountHolder.hpp>

#include <QCommandLineParser>

Q_NORETURN static void error(const QString& str) {
  throw vx::Exception("de.uni_stuttgart.Voxie.ExtDataT3R.Error", str);
}

int main(int argc, char* argv[]) {
  try {
    QCoreApplication app(argc, argv);
    app.setApplicationName("ExtDataT3R");
    app.setApplicationVersion("1.0");

    if (argc < 1) {
      error("argc is smaller than 1");
    }

    QCommandLineParser parser;
    parser.setApplicationDescription(
        "Voxie script for lazy-loading and "
        "processing raw Timepix3 event list files");
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addOptions(vx::DBusClient::options());
    parser.addOptions(vx::ClaimedOperationBase::options());
    QCommandLineOption voxieImportFilename("voxie-import-filename",
                                           "t3r file to load.", "filename");
    parser.addOption(voxieImportFilename);

    parser.process(app);

    if (!parser.isSet(vx::ClaimedOperationBase::voxieOperationOption())) {
      error("--voxie-operation is not set");
    }

    if (parser.isSet(vx::ClaimedOperationBase::voxieActionOption()) &&
        parser.value(vx::ClaimedOperationBase::voxieActionOption()) !=
            "Import") {
      error("--voxie-action is not 'Import'");
    }

    QString operationPath =
        parser.value(vx::ClaimedOperationBase::voxieOperationOption());

    if (!parser.isSet(voxieImportFilename)) {
      error("--voxie-import-filename is not set");
    }

    QString filename = parser.value(voxieImportFilename);

    vx::initDBusTypes();

    vx::DBusClient dbusClient(parser);

    auto op = QSharedPointer<vx::ClaimedOperation<
        de::uni_stuttgart::Voxie::ExternalOperationImport>>::
        create(dbusClient, QDBusObjectPath(operationPath));

    // TODO: Support for peer-to-peer connections
    vx::getBusManager()->addConnection(makeSharedQObject<vx::BusConnection>(
        QDBusConnection::sessionBus(), true));
    // auto dbusService =
    //     makeSharedQObject<vx::DBusServiceBus>(QDBusConnection::sessionBus());

    vx::ClientManager clientManager;

    op->forwardExc([&]() {
      vx::t3r::LazyLoader lazyLoader(dbusClient);

      if (filename.toLower().endsWith(".t3r")) {
        lazyLoader.openSingleStream(filename, op);
      } else if (filename.toLower().endsWith(".timepixraw.json")) {
        lazyLoader.openMultiStream(filename, op);
      } else {
        error("Invalid file extension (must be *.t3r or *.timepixraw.json");
      }

      lazyLoader.createAccessor();

      HANDLEDBUSPENDINGREPLY(op->op().Finish(lazyLoader.getAccessorPath(),
                                             lazyLoader.getVersionPath(),
                                             vx::emptyOptions()));
    });

    // Destroy operation object to indicate completion (and remove progress bar)
    op.reset();

    return app.exec();
  } catch (vx::Exception& error) {
    QTextStream(stderr) << error.name() << ": " << error.message() << endl
                        << flush;
    return 1;
  }
}
