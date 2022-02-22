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

#include <ExtFilterMarchingCubes33/MarchingCubes33.hpp>

#include <QtCore/QCommandLineParser>
#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QString>

#include <QDebug>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>
#include <QtDBus/QDBusError>
#include <QtDBus/QDBusPendingReply>

inline QVector3D toQtVector(const vx::TupleVector<double, 3>& vec) {
  return QVector3D(std::get<0>(vec), std::get<1>(vec), std::get<2>(vec));
}

int main(int argc, char* argv[]) {
  try {
    // Setting up the external filter
    if (argc < 2)
      throw vx::Exception(
          "de.uni_stuttgart.Voxie.ExtFilterMarchingCubes33.Error",
          "argc is smaller than 2");
    QCommandLineParser parser;
    parser.setApplicationDescription("Example filter for modifying a surface");
    parser.addHelpOption();

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
      // Getting the input data
      auto filterPath = op.op().filterObject();
      auto parameters = op.op().parameters();

      auto properties = vx::dbusGetVariantValue<QMap<QString, QDBusVariant>>(
          parameters[filterPath]["Properties"]);

      auto inputPath = vx::dbusGetVariantValue<QDBusObjectPath>(
          properties["de.uni_stuttgart.Voxie.Input"]);

      auto inputDataPath = vx::dbusGetVariantValue<QDBusObjectPath>(
          parameters[inputPath]["Data"]);

      auto inputVolumeData =
          makeSharedQObject<de::uni_stuttgart::Voxie::VolumeData>(
              dbusClient.uniqueName(), inputDataPath.path(),
              dbusClient.connection());
      auto inputDataVoxel =
          makeSharedQObject<de::uni_stuttgart::Voxie::VolumeDataVoxel>(
              dbusClient.uniqueName(), inputDataPath.path(),
              dbusClient.connection());
      auto isovalue =
          properties["de.uni_stuttgart.Voxie.Threshold"].variant().toFloat();
      auto outputPath = vx::dbusGetVariantValue<QDBusObjectPath>(
          properties["de.uni_stuttgart.Voxie.Output"]);

      // Extract the actual data out of the input data
      vx::Array3<const float> inputVolume(HANDLEDBUSPENDINGREPLY(
          inputDataVoxel->GetDataReadonly(QMap<QString, QDBusVariant>())));

      vx::TupleVector<quint64, 3> dimensions = inputDataVoxel->arrayShape();

      // Run Marching Cubes
      MarchingCubes mc;
      mc.extract(inputVolume, dimensions,
                 toQtVector(inputVolumeData->volumeOrigin()),
                 toQtVector(inputDataVoxel->gridSpacing()), isovalue);

      // Creating the output
      size_t triangleCount = mc.GetTriangles()->size();

      vx::RefObjWrapper<de::uni_stuttgart::Voxie::SurfaceDataTriangleIndexed>
          srf2t(dbusClient, HANDLEDBUSPENDINGREPLY(
                                dbusClient->CreateSurfaceDataTriangleIndexed(
                                    dbusClient.clientPath(), triangleCount, 0,
                                    QDBusObjectPath("/"), true,
                                    QMap<QString, QDBusVariant>())));
      auto srf2t_data = makeSharedQObject<de::uni_stuttgart::Voxie::Data>(
          dbusClient.uniqueName(), srf2t.path().path(),
          dbusClient.connection());

      {
        vx::RefObjWrapper<de::uni_stuttgart::Voxie::ExternalDataUpdate> update(
            dbusClient,
            HANDLEDBUSPENDINGREPLY(srf2t_data->CreateUpdate(
                dbusClient.clientPath(), QMap<QString, QDBusVariant>())));
        vx::Array2<uint32_t> srf2t_triangles(
            HANDLEDBUSPENDINGREPLY(srf2t->GetTrianglesWritable(
                update.path(), QMap<QString, QDBusVariant>())));

        for (size_t i = 0; i < triangleCount; i++)
          for (size_t j = 0; j < 3; j++)
            srf2t_triangles(i, j) = (*mc.GetTriangles())[i].indices[j];
      }
      QSharedPointer<vx::RefObjWrapper<de::uni_stuttgart::Voxie::DataVersion>>
          srf2_version;
      size_t vertexCount = mc.GetVertices()->size();

      vx::RefObjWrapper<de::uni_stuttgart::Voxie::SurfaceDataTriangleIndexed>
          srf2(dbusClient,
               HANDLEDBUSPENDINGREPLY(
                   dbusClient->CreateSurfaceDataTriangleIndexed(
                       dbusClient.clientPath(), triangleCount, vertexCount,
                       srf2t.path(), false, QMap<QString, QDBusVariant>())));
      auto srf2_data = makeSharedQObject<de::uni_stuttgart::Voxie::Data>(
          dbusClient.uniqueName(), srf2.path().path(), dbusClient.connection());
      {
        vx::RefObjWrapper<de::uni_stuttgart::Voxie::ExternalDataUpdate> update(
            dbusClient,
            HANDLEDBUSPENDINGREPLY(srf2_data->CreateUpdate(
                dbusClient.clientPath(), QMap<QString, QDBusVariant>())));
        vx::Array2<float> srf2_vertices(
            HANDLEDBUSPENDINGREPLY(srf2->GetVerticesWritable(
                update.path(), QMap<QString, QDBusVariant>())));

        for (size_t i = 0; i < vertexCount; i++) {
          srf2_vertices(i, 0) = (*mc.GetVertices())[i][0];
          srf2_vertices(i, 1) = (*mc.GetVertices())[i][1];
          srf2_vertices(i, 2) = (*mc.GetVertices())[i][2];
        }

        srf2_version = createQSharedPointer<
            vx::RefObjWrapper<de::uni_stuttgart::Voxie::DataVersion>>(
            dbusClient,
            HANDLEDBUSPENDINGREPLY(update->Finish(
                dbusClient.clientPath(), QMap<QString, QDBusVariant>())));
      }

      QMap<QString, QDBusVariant> outputResult;
      outputResult["Data"] = vx::dbusMakeVariant<QDBusObjectPath>(srf2.path());
      outputResult["DataVersion"] =
          vx::dbusMakeVariant<QDBusObjectPath>(srf2_version->path());

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
