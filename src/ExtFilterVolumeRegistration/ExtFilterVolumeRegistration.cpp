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

#include <VoxieClient/Array.hpp>
#include <VoxieClient/ClaimedOperation.hpp>
#include <VoxieClient/DBusClient.hpp>
#include <VoxieClient/DBusProxies.hpp>
#include <VoxieClient/DBusTypeList.hpp>
#include <VoxieClient/DBusUtil.hpp>
#include <VoxieClient/Exception.hpp>
#include <VoxieClient/Exceptions.hpp>
#include <VoxieClient/MappedBuffer.hpp>
#include <VoxieClient/QtUtil.hpp>
#include <VoxieClient/RefCountHolder.hpp>

#include <ExtFilterVolumeRegistration/VolumeRegistration.hpp>

#include <QtCore/QCommandLineParser>
#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QString>

#include <QDebug>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>
#include <QtDBus/QDBusError>
#include <QtDBus/QDBusPendingReply>

#include <QtGui/QVector3D>

inline QVector3D toQtVector(const vx::TupleVector<double, 3>& vec) {
  return QVector3D(std::get<0>(vec), std::get<1>(vec), std::get<2>(vec));
}

int main(int argc, char* argv[]) {
  try {
    if (argc < 1)
      throw vx::Exception(
          "de.uni_stuttgart.Voxie.ExtFilterVolumeRegistration.Error",
          "argc is smaller than 1");

    QCommandLineParser parser;
    parser.setApplicationDescription(
        "Filter for matching two volumes of same object");
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addOptions(vx::DBusClient::options());
    parser.addOptions(vx::ClaimedOperationBase::options());

    QStringList args;
    for (char** arg = argv; *arg; arg++) args.push_back(*arg);
    int argc0 = 1;
    char* args0[2] = {argv[0], NULL};
    QCoreApplication app(argc0, args0);
    parser.process(args);

    vx::initDBusTypes();

    vx::DBusClient dbusClient(parser);

    vx::ClaimedOperation<de::uni_stuttgart::Voxie::ExternalOperationRunFilter>
        op(dbusClient,
           vx::ClaimedOperationBase::getOperationPath(parser, "RunFilter"));
    op.forwardExc([&]() {
      auto filterPath = op.op().filterObject();
      auto pars = op.op().parameters();

      auto properties = vx::dbusGetVariantValue<QMap<QString, QDBusVariant>>(
          pars[filterPath]["Properties"]);

      auto refVolumePath = vx::dbusGetVariantValue<QDBusObjectPath>(
          properties["de.uni_stuttgart.Voxie.Filter.VolumeRegistration."
                     "ReferenceVolume"]);
      auto refVolumeDataPath =
          vx::dbusGetVariantValue<QDBusObjectPath>(pars[refVolumePath]["Data"]);
      auto refVolumeData =
          makeSharedQObject<de::uni_stuttgart::Voxie::VolumeData>(
              dbusClient.uniqueName(), refVolumeDataPath.path(),
              dbusClient.connection());
      auto refVolumeDataVoxel =
          makeSharedQObject<de::uni_stuttgart::Voxie::VolumeDataVoxel>(
              dbusClient.uniqueName(), refVolumeDataPath.path(),
              dbusClient.connection());

      auto inVolumePath = vx::dbusGetVariantValue<QDBusObjectPath>(
          properties
              ["de.uni_stuttgart.Voxie.Filter.VolumeRegistration.InputVolume"]);
      auto inVolumeDataPath =
          vx::dbusGetVariantValue<QDBusObjectPath>(pars[inVolumePath]["Data"]);
      auto inVolumeData =
          makeSharedQObject<de::uni_stuttgart::Voxie::VolumeData>(
              dbusClient.uniqueName(), inVolumeDataPath.path(),
              dbusClient.connection());
      auto inVolumeDataVoxel =
          makeSharedQObject<de::uni_stuttgart::Voxie::VolumeDataVoxel>(
              dbusClient.uniqueName(), inVolumeDataPath.path(),
              dbusClient.connection());
      auto outputPath = vx::dbusGetVariantValue<QDBusObjectPath>(
          properties["de.uni_stuttgart.Voxie.Output"]);

      std::tuple<double, double, double> translation;
      std::tuple<double, double, double, double> rotation;

      {
        vx::Array3<const float> inputVolume1(
            HANDLEDBUSPENDINGREPLY(refVolumeDataVoxel->GetDataReadonly(
                QMap<QString, QDBusVariant>())));

        vx::Array3<const float> inputVolume2(HANDLEDBUSPENDINGREPLY(
            inVolumeDataVoxel->GetDataReadonly(QMap<QString, QDBusVariant>())));
        QVector3D spacing1 = toQtVector(refVolumeDataVoxel->gridSpacing());
        QVector3D origin1 = toQtVector(refVolumeData->volumeOrigin());
        QVector3D spacing2 = toQtVector(inVolumeDataVoxel->gridSpacing());
        QVector3D origin2 = toQtVector(inVolumeData->volumeOrigin());
        VolumeRegistration filter;
        filter.compute(inputVolume1, inputVolume2, spacing1, spacing2, origin1,
                       origin2, translation, rotation, op);
      }

      QMap<QString, QDBusVariant> outputResult;
      outputResult["Data"] =
          vx::dbusMakeVariant<QDBusObjectPath>(inVolumeDataPath);
      // QDBusObjectPath dataVersion =
      // vx::dbusGetVariantValue<QDBusObjectPath>(pars[inVolumePath]["DataVersion"]);
      // outputResult["DataVersion"] =
      // vx::dbusMakeVariant<QDBusObjectPath>(dataVersion);

      QMap<QString, QDBusVariant> outputProperties;
      outputProperties["de.uni_stuttgart.Voxie.MovableDataNode.Translation"] =
          vx::dbusMakeVariant<std::tuple<double, double, double>>(translation);
      outputProperties["de.uni_stuttgart.Voxie.MovableDataNode.Rotation"] =
          vx::dbusMakeVariant<std::tuple<double, double, double, double>>(
              rotation);
      outputResult["Properties"] =
          vx::dbusMakeVariant<QMap<QString, QDBusVariant>>(outputProperties);

      QMap<QDBusObjectPath, QMap<QString, QDBusVariant>> result;
      result[outputPath] = outputResult;

      HANDLEDBUSPENDINGREPLY(
          op.op().Finish(result, QMap<QString, QDBusVariant>()));
    });
    return 0;
  } catch (vx::Exception& error) {
    QTextStream(stderr) << error.name() << ": " << error.message() << endl
                        << flush;
    return 1;
  }
}
