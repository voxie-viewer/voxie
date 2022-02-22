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

#include <algorithm>
#include <array>
#include <cmath>
#include <future>
#include <iostream>
#include <list>
#include <numeric>
#include <vector>

#include "MultiMaterialVolumeSegmentation.hpp"
#include "Voxel.hpp"

int main(int argc, char* argv[]) {
  try {
    if (argc < 1)
      throw vx::Exception(
          "de.uni_stuttgart.Voxie.ExtFilterMultiMaterialVolumeSegmentation."
          "Error",
          "argc is smaller than 1");

    QCommandLineParser parser;
    parser.setApplicationDescription(
        "Calculate exact material of each voxel in a volume and label "
        "accordingly");
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
      // qDebug() << filterPath.path();

      auto properties = vx::dbusGetVariantValue<QMap<QString, QDBusVariant>>(
          pars[filterPath]["Properties"]);
      //        qDebug() << properties;

      std::string thresholds =
          vx::dbusGetVariantValue<QString>(
              properties["de.uni_stuttgart.Voxie.Filter."
                         "MultiMaterialVolumeSegmentation.Thresholds"])
              .toStdString();

      size_t iterations = vx::dbusGetVariantValue<qlonglong>(
          properties["de.uni_stuttgart.Voxie.Filter."
                     "MultiMaterialVolumeSegmentation.MaxIterations"]);

      float thresholdDifferenceSignificance = vx::dbusGetVariantValue<double>(
          properties
              ["de.uni_stuttgart.Voxie.Filter.MultiMaterialVolumeSegmentation."
               "ThresholdDifferenceSignificance"]);

      float epsilon = vx::dbusGetVariantValue<double>(
          properties["de.uni_stuttgart.Voxie.Filter."
                     "MultiMaterialVolumeSegmentation.Epsilon"]);

      auto inputPath = vx::dbusGetVariantValue<QDBusObjectPath>(
          properties["de.uni_stuttgart.Voxie.Input"]);
      auto inputDataPath =
          vx::dbusGetVariantValue<QDBusObjectPath>(pars[inputPath]["Data"]);

      auto inputData = makeSharedQObject<de::uni_stuttgart::Voxie::VolumeData>(
          dbusClient.uniqueName(), inputDataPath.path(),
          dbusClient.connection());
      auto inputDataVoxel =
          makeSharedQObject<de::uni_stuttgart::Voxie::VolumeDataVoxel>(
              dbusClient.uniqueName(), inputDataPath.path(),
              dbusClient.connection());
      auto outputPath = vx::dbusGetVariantValue<QDBusObjectPath>(
          properties["de.uni_stuttgart.Voxie.Output"]);

      auto size = inputDataVoxel->arrayShape();
      QMap<QString, QDBusVariant> options;

      QSharedPointer<vx::RefObjWrapper<de::uni_stuttgart::Voxie::DataVersion>>
          volume_version;

      vx::RefObjWrapper<de::uni_stuttgart::Voxie::VolumeDataVoxel> volume(
          dbusClient,
          HANDLEDBUSPENDINGREPLY(dbusClient->CreateVolumeDataVoxel(
              dbusClient.clientPath(), size,
              std::make_tuple("uint", 8, "native"), inputData->volumeOrigin(),
              inputDataVoxel->gridSpacing(), options)));
      auto volume_data = makeSharedQObject<de::uni_stuttgart::Voxie::Data>(
          dbusClient.uniqueName(), volume.path().path(),
          dbusClient.connection());
      {
        vx::RefObjWrapper<de::uni_stuttgart::Voxie::ExternalDataUpdate> update(
            dbusClient,
            HANDLEDBUSPENDINGREPLY(volume_data->CreateUpdate(
                dbusClient.clientPath(), QMap<QString, QDBusVariant>())));

        vx::Array3<const float> inputVolume(HANDLEDBUSPENDINGREPLY(
            inputDataVoxel->GetDataReadonly(QMap<QString, QDBusVariant>())));

        vx::Array3<uint8_t> volumeData(
            HANDLEDBUSPENDINGREPLY(volume->GetDataWritable(
                update.path(), QMap<QString, QDBusVariant>())));

        MultiMaterialVolumeSegmentation filter(
            std::cref(inputVolume), std::cref(volumeData), thresholds,
            iterations, thresholdDifferenceSignificance, epsilon);
        filter.run();

        volume_version = createQSharedPointer<
            vx::RefObjWrapper<de::uni_stuttgart::Voxie::DataVersion>>(
            dbusClient,
            HANDLEDBUSPENDINGREPLY(update->Finish(
                dbusClient.clientPath(), QMap<QString, QDBusVariant>())));
      }

      QMap<QString, QDBusVariant> outputResult;
      outputResult["Data"] =
          vx::dbusMakeVariant<QDBusObjectPath>(volume.path());
      outputResult["DataVersion"] =
          vx::dbusMakeVariant<QDBusObjectPath>(volume_version->path());

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
