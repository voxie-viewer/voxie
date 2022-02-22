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

#include <ExtFilterAddNormalAttribute/AddNormalAttribute.hpp>

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
          "de.uni_stuttgart.Voxie.ExtFilterAddNormalAttribute.Error",
          "argc is smaller than 1");

    QCommandLineParser parser;
    parser.setApplicationDescription(
        "Filter calculating surface (vertex) normals");
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

      auto inSurfacePath = vx::dbusGetVariantValue<QDBusObjectPath>(
          properties["de.uni_stuttgart.Voxie.Filter.AddNormalAttribute."
                     "InputSurface"]);
      auto inSurfaceDataPath =
          vx::dbusGetVariantValue<QDBusObjectPath>(pars[inSurfacePath]["Data"]);
      auto inSurfaceData = makeSharedQObject<
          de::uni_stuttgart::Voxie::SurfaceDataTriangleIndexed>(
          dbusClient.uniqueName(), inSurfaceDataPath.path(),
          dbusClient.connection());

      vx::Array2<const float> vertices(HANDLEDBUSPENDINGREPLY(
          inSurfaceData->GetVerticesReadonly(QMap<QString, QDBusVariant>())));

      vx::Array2<const uint> triangles(HANDLEDBUSPENDINGREPLY(
          inSurfaceData->GetTrianglesReadonly(QMap<QString, QDBusVariant>())));

      auto method = vx::dbusGetVariantValue<QString>(
          properties
              ["de.uni_stuttgart.Voxie.Filter.AddNormalAttribute.Method"]);

      // output surface
      auto outputPath = vx::dbusGetVariantValue<QDBusObjectPath>(
          properties["de.uni_stuttgart.Voxie.Output"]);

      QList<std::tuple<
          QString, QString, quint64, std::tuple<QString, quint32, QString>,
          QString, QMap<QString, QDBusVariant>, QMap<QString, QDBusVariant>>>
          attributes{std::make_tuple(
              "de.uni_stuttgart.Voxie.SurfaceAttribute.Normal",
              "de.uni_stuttgart.Voxie.SurfaceAttributeKind.Vertex",
              3,  // componentCount_
              std::make_tuple("float", 32, "native"), "Normals",
              QMap<QString, QDBusVariant>(), QMap<QString, QDBusVariant>())};

      QMap<QString, QDBusVariant> options;
      options.insert("Attributes", vx::dbusMakeVariant(attributes));

      vx::RefObjWrapper<de::uni_stuttgart::Voxie::SurfaceDataTriangleIndexed>
          outputSurface(
              dbusClient,
              HANDLEDBUSPENDINGREPLY(
                  dbusClient->CreateSurfaceDataTriangleIndexed(
                      dbusClient.clientPath(), triangles.size<0>(),
                      vertices.size<0>(), inSurfaceDataPath, true, options)));

      auto outputSurfaceData =
          makeSharedQObject<de::uni_stuttgart::Voxie::Data>(
              dbusClient.uniqueName(), outputSurface.path().path(),
              dbusClient.connection());

      vx::RefObjWrapper<de::uni_stuttgart::Voxie::ExternalDataUpdate> update(
          dbusClient,
          HANDLEDBUSPENDINGREPLY(outputSurfaceData->CreateUpdate(
              dbusClient.clientPath(), QMap<QString, QDBusVariant>())));
      {
        vx::Array2<float> outputNormals(
            HANDLEDBUSPENDINGREPLY(outputSurface->GetAttributeWritable(
                update.path(), "de.uni_stuttgart.Voxie.SurfaceAttribute.Normal",
                QMap<QString, QDBusVariant>())));

        vx::Array2<uint> outputTriangles(
            HANDLEDBUSPENDINGREPLY(outputSurface->GetTrianglesWritable(
                update.path(), QMap<QString, QDBusVariant>())));
        for (uint32_t i = 0; i < outputTriangles.size<0>(); i++) {
          for (int vertex = 0; vertex < 3; vertex++) {
            outputTriangles(i, vertex) = triangles(i, vertex);
          }
        }

        vx::Array2<float> outputVertices(
            HANDLEDBUSPENDINGREPLY(outputSurface->GetVerticesWritable(
                update.path(), QMap<QString, QDBusVariant>())));

        for (uint32_t i = 0; i < outputVertices.size<0>(); i++) {
          for (int coord = 0; coord < 3; coord++) {
            outputVertices(i, coord) = vertices(i, coord);
          }
        }

        AddNormalAttribute filter;

        if (method ==
            "de.uni_stuttgart.Voxie.Filter.AddNormalAttribute.Method."
            "Triangles") {
          filter.compute(vertices, triangles, outputNormals, op);
        }
        if (method ==
            "de.uni_stuttgart.Voxie.Filter.AddNormalAttribute.Method.Volume") {
          auto inVolumePath = vx::dbusGetVariantValue<QDBusObjectPath>(
              properties["de.uni_stuttgart.Voxie.Filter.AddNormalAttribute."
                         "InputVolume"]);
          auto inVolumeDataPath = vx::dbusGetVariantValue<QDBusObjectPath>(
              pars[inVolumePath]["Data"]);
          auto inVolumeData =
              makeSharedQObject<de::uni_stuttgart::Voxie::VolumeData>(
                  dbusClient.uniqueName(), inVolumeDataPath.path(),
                  dbusClient.connection());
          auto inVolumeDataVoxel =
              makeSharedQObject<de::uni_stuttgart::Voxie::VolumeDataVoxel>(
                  dbusClient.uniqueName(), inVolumeDataPath.path(),
                  dbusClient.connection());

          vx::Array3<const float> volume(
              HANDLEDBUSPENDINGREPLY(inVolumeDataVoxel->GetDataReadonly(
                  QMap<QString, QDBusVariant>())));
          QVector3D spacing = toQtVector(inVolumeDataVoxel->gridSpacing());
          QVector3D origin = toQtVector(inVolumeData->volumeOrigin());

          filter.compute(vertices, volume, spacing, origin, outputNormals, op);
        }
      }

      // Define Output
      QMap<QString, QDBusVariant> outputResult;
      outputResult["Data"] =
          vx::dbusMakeVariant<QDBusObjectPath>(outputSurface.path());
      //          vx::dbusMakeVariant<QDBusObjectPath>(inSurfaceDataPath);

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
