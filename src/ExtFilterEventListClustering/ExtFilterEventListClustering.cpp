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

#include "T3RClusterLazyLoader.hpp"

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

#include <memory>

Q_NORETURN static void error(const QString& str) {
  throw vx::Exception("de.uni_stuttgart.Voxie.EventListClustering.Error", str);
}

int main(int argc, char* argv[]) {
  try {
    QCoreApplication app(argc, argv);
    app.setApplicationName("EventListClustering");
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

    parser.process(app);

    vx::initDBusTypes();

    vx::DBusClient dbusClient(parser);

    auto op = std::make_unique<vx::ClaimedOperation<
        de::uni_stuttgart::Voxie::ExternalOperationRunFilter>>(
        dbusClient,
        vx::ClaimedOperationBase::getOperationPath(parser, "RunFilter"));

    // TODO: Support for peer-to-peer connections
    vx::getBusManager()->addConnection(makeSharedQObject<vx::BusConnection>(
        QDBusConnection::sessionBus(), true));
    // auto dbusService =
    //     makeSharedQObject<vx::DBusServiceBus>(QDBusConnection::sessionBus());

    vx::ClientManager clientManager;

    op->forwardExc([&]() {
      auto filterPath = op->op().filterObject();
      auto pars = op->op().parameters();

      auto properties = vx::dbusGetVariantValue<QMap<QString, QDBusVariant>>(
          pars[filterPath]["Properties"]);

      QString prefix = "de.uni_stuttgart.Voxie.Filter.EventListClustering.";

      auto outputPath = vx::dbusGetVariantValue<QDBusObjectPath>(
          properties[prefix + "OutputClusterList"]);
      auto inputPath = vx::dbusGetVariantValue<QDBusObjectPath>(
          properties[prefix + "InputEventList"]);

      auto inputDataPath =
          vx::dbusGetVariantValue<QDBusObjectPath>(pars[inputPath]["Data"]);

      auto inputEventListDataAccessorOperations = makeSharedQObject<
          de::uni_stuttgart::Voxie::EventListDataAccessorOperations>(
          dbusClient.uniqueName(), inputDataPath.path(),
          dbusClient.connection());

      double nanoseconds = 16.0 / 25.0;

      vx::t3r::ClusteringSettings settings;
      settings.temporalMargin = vx::dbusGetVariantValue<double>(
                                    properties[prefix + "TemporalMargin"]) *
                                nanoseconds;
      settings.xrf.enabled =
          vx::dbusGetVariantValue<bool>(properties[prefix + "XRFCorrection"]);
      settings.xrf.minEnergy =
          vx::dbusGetVariantValue<double>(properties[prefix + "XRFMinEnergy"]);
      settings.xrf.maxEnergy =
          vx::dbusGetVariantValue<double>(properties[prefix + "XRFMaxEnergy"]);
      settings.xrf.temporalMargin =
          vx::dbusGetVariantValue<double>(
              properties[prefix + "XRFTemporalMargin"]) *
          nanoseconds;
      settings.xrf.distanceLimit = vx::dbusGetVariantValue<double>(
          properties[prefix + "XRFDistanceLimit"]);
      settings.passthrough =
          vx::dbusGetVariantValue<bool>(properties[prefix + "Passthrough"]);

      vx::t3r::ClusterLazyLoader lazyLoader(dbusClient);
      lazyLoader.setClusteringSettings(settings);
      lazyLoader.setInputAccessor(inputEventListDataAccessorOperations);

      lazyLoader.createOutputAccessor();

      QMap<QString, QDBusVariant> outputResult;
      outputResult["Data"] = vx::dbusMakeVariant<QDBusObjectPath>(
          lazyLoader.getOutputAccessorPath());
      outputResult["DataVersion"] =
          vx::dbusMakeVariant<QDBusObjectPath>(lazyLoader.getVersionPath());

      QMap<QDBusObjectPath, QMap<QString, QDBusVariant>> result;
      result[outputPath] = outputResult;

      HANDLEDBUSPENDINGREPLY(op->op().Finish(result, vx::emptyOptions()));
    });

    op.reset();

    return app.exec();
  } catch (vx::Exception& error) {
    QTextStream(stderr) << error.name() << ": " << error.message() << endl
                        << flush;
    return 1;
  }
}
