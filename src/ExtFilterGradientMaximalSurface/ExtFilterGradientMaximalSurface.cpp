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

#include "GradientMaximalSurfaceOperation.hpp"

inline QVector3D toQtVector(const vx::TupleVector<double, 3>& vec) {
  return QVector3D(std::get<0>(vec), std::get<1>(vec), std::get<2>(vec));
}

int main(int argc, char* argv[]) {
  try {
    if (argc < /*3*/ 0)
      throw vx::Exception(
          "de.uni_stuttgart.Voxie.ExtFilterGradientMaximalSurface.Error",
          "argc is smaller than 3");

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

      // get volume input
      auto volumeInputPath = vx::dbusGetVariantValue<QDBusObjectPath>(
          properties["de.uni_stuttgart.Voxie.Filter.GradientMaximalSurface."
                     "InputVolume"]);
      auto volumeInputDataPath = vx::dbusGetVariantValue<QDBusObjectPath>(
          pars[volumeInputPath]["Data"]);

      auto volumeInputData =
          makeSharedQObject<de::uni_stuttgart::Voxie::VolumeData>(
              dbusClient.uniqueName(), volumeInputDataPath.path(),
              dbusClient.connection());
      auto volumeInputDataVoxel =
          makeSharedQObject<de::uni_stuttgart::Voxie::VolumeDataVoxel>(
              dbusClient.uniqueName(), volumeInputDataPath.path(),
              dbusClient.connection());
      auto outputPath = vx::dbusGetVariantValue<QDBusObjectPath>(
          properties
              ["de.uni_stuttgart.Voxie.Filter.GradientMaximalSurface.Output"]);

      auto size = volumeInputDataVoxel->arrayShape();
      QMap<QString, QDBusVariant> options;

      QSharedPointer<vx::RefObjWrapper<de::uni_stuttgart::Voxie::DataVersion>>
          volume_version;

      vx::RefObjWrapper<de::uni_stuttgart::Voxie::VolumeDataVoxel> volume(
          dbusClient,
          HANDLEDBUSPENDINGREPLY(dbusClient->CreateVolumeDataVoxel(
              dbusClient.clientPath(), size, volumeInputData->dataType(),
              volumeInputData->volumeOrigin(),
              volumeInputDataVoxel->gridSpacing(), options)));
      auto volume_data = makeSharedQObject<de::uni_stuttgart::Voxie::Data>(
          dbusClient.uniqueName(), volume.path().path(),
          dbusClient.connection());

      // get surface input
      auto surfaceInputPath = vx::dbusGetVariantValue<QDBusObjectPath>(
          properties["de.uni_stuttgart.Voxie.Filter.GradientMaximalSurface."
                     "InputSurface"]);
      auto surfaceInputDataPath = vx::dbusGetVariantValue<QDBusObjectPath>(
          pars[surfaceInputPath]["Data"]);

      auto inputSurface = makeSharedQObject<
          de::uni_stuttgart::Voxie::SurfaceDataTriangleIndexed>(
          dbusClient.uniqueName(), surfaceInputDataPath.path(),
          dbusClient.connection());

      vx::Array2<const float> vertices(HANDLEDBUSPENDINGREPLY(
          inputSurface->GetVerticesReadonly(QMap<QString, QDBusVariant>())));
      vx::Array2<const uint32_t> triangles(HANDLEDBUSPENDINGREPLY(
          inputSurface->GetTrianglesReadonly(QMap<QString, QDBusVariant>())));

      vx::RefObjWrapper<de::uni_stuttgart::Voxie::SurfaceDataTriangleIndexed>
          outputSurface(dbusClient,
                        HANDLEDBUSPENDINGREPLY(
                            dbusClient->CreateSurfaceDataTriangleIndexed(
                                dbusClient.clientPath(), triangles.size<0>(),
                                vertices.size<0>(), surfaceInputDataPath, false,
                                QMap<QString, QDBusVariant>())));

      auto outputSurfaceData =
          makeSharedQObject<de::uni_stuttgart::Voxie::Data>(
              dbusClient.uniqueName(), outputSurface.path().path(),
              dbusClient.connection());

      vx::RefObjWrapper<de::uni_stuttgart::Voxie::ExternalDataUpdate>
          updateVertices(
              dbusClient,
              HANDLEDBUSPENDINGREPLY(outputSurfaceData->CreateUpdate(
                  dbusClient.clientPath(), QMap<QString, QDBusVariant>())));

      vx::Array2<float> outputVertices(
          HANDLEDBUSPENDINGREPLY(outputSurface->GetVerticesWritable(
              updateVertices.path(), QMap<QString, QDBusVariant>())));

      // copy vertices from input to output
      for (size_t i = 0; i < vertices.size<0>(); i++) {
        outputVertices(i, 0) = vertices(i, 0);
        outputVertices(i, 1) = vertices(i, 1);
        outputVertices(i, 2) = vertices(i, 2);
      }

      {
        // TODO: Why does this create an update for the volume?
        vx::RefObjWrapper<de::uni_stuttgart::Voxie::ExternalDataUpdate>
            updateVolume(
                dbusClient,
                HANDLEDBUSPENDINGREPLY(volume_data->CreateUpdate(
                    dbusClient.clientPath(), QMap<QString, QDBusVariant>())));

        vx::Array3<const float> inputVolume(
            HANDLEDBUSPENDINGREPLY(volumeInputDataVoxel->GetDataReadonly(
                QMap<QString, QDBusVariant>())));

        auto samplingPointCount = vx::dbusGetVariantValue<qlonglong>(
            properties["de.uni_stuttgart.Voxie.GradientMaximalSurface."
                       "SamplingPoints"]);

        auto samplingDistance = vx::dbusGetVariantValue<double>(
            properties["de.uni_stuttgart.Voxie.GradientMaximalSurface."
                       "SamplingDistance"]);

        GradientMaximalSurfaceOperation filter(
            inputVolume, outputVertices,
            toQtVector(volumeInputData->volumeOrigin()),
            toQtVector(volume->gridSpacing()),
            samplingPointCount < 2 ? 2 : samplingPointCount, samplingDistance);
        filter.run(op);

        volume_version = createQSharedPointer<
            vx::RefObjWrapper<de::uni_stuttgart::Voxie::DataVersion>>(
            dbusClient,
            HANDLEDBUSPENDINGREPLY(updateVolume->Finish(
                dbusClient.clientPath(), QMap<QString, QDBusVariant>())));
      }

      QMap<QString, QDBusVariant> outputResult;
      outputResult["Data"] =
          vx::dbusMakeVariant<QDBusObjectPath>(outputSurface.path());
      // TODO: This should also call updateVertices->Finish() and return the
      // version

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
