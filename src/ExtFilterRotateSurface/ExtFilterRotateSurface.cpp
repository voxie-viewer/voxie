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

#include <QtCore/QCommandLineParser>
#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QString>

#include <QDebug>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>
#include <QtDBus/QDBusError>
#include <QtDBus/QDBusPendingReply>

int main(int argc, char* argv[]) {
  try {
    if (argc < 1)
      throw vx::Exception(
          "de.uni_stuttgart.Voxie.ExtFilterExtractFeaturePoints.Error",
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

      auto inSurfacePath = vx::dbusGetVariantValue<QDBusObjectPath>(
          properties
              ["de.uni_stuttgart.Voxie.Filter.RotateSurface.InputSurface"]);
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

      QMap<QString, QDBusVariant> inputProperties =
          vx::dbusGetVariantValue<QMap<QString, QDBusVariant>>(
              pars[inSurfacePath]["Properties"]);
      std::tuple<double, double, double> translation =
          vx::dbusGetVariantValue<std::tuple<double, double, double>>(
              inputProperties
                  ["de.uni_stuttgart.Voxie.MovableDataNode.Translation"]);
      std::tuple<double, double, double, double> rotation =
          vx::dbusGetVariantValue<std::tuple<double, double, double, double>>(
              inputProperties
                  ["de.uni_stuttgart.Voxie.MovableDataNode.Rotation"]);

      QSharedPointer<vx::RefObjWrapper<de::uni_stuttgart::Voxie::DataVersion>>
          srf_version;

      vx::RefObjWrapper<de::uni_stuttgart::Voxie::SurfaceDataTriangleIndexed>
          srf(dbusClient, HANDLEDBUSPENDINGREPLY(
                              dbusClient->CreateSurfaceDataTriangleIndexed(
                                  dbusClient.clientPath(), triangles.size<0>(),
                                  vertices.size<0>(), QDBusObjectPath("/"),
                                  true, QMap<QString, QDBusVariant>())));
      auto srf_data = makeSharedQObject<de::uni_stuttgart::Voxie::Data>(
          dbusClient.uniqueName(), srf.path().path(), dbusClient.connection());
      {
        vx::RefObjWrapper<de::uni_stuttgart::Voxie::ExternalDataUpdate> update(
            dbusClient,
            HANDLEDBUSPENDINGREPLY(srf_data->CreateUpdate(
                dbusClient.clientPath(), QMap<QString, QDBusVariant>())));
        vx::Array2<float> srf_vertices(
            HANDLEDBUSPENDINGREPLY(srf->GetVerticesWritable(
                update.path(), QMap<QString, QDBusVariant>())));

        vx::Array2<uint> srf_triangles(
            HANDLEDBUSPENDINGREPLY(srf->GetTrianglesWritable(
                update.path(), QMap<QString, QDBusVariant>())));

        double shift[3];
        shift[0] = std::get<0>(translation);
        shift[1] = std::get<1>(translation);
        shift[2] = std::get<2>(translation);

        double qr = std::get<0>(rotation);
        double qi = std::get<1>(rotation);
        double qj = std::get<2>(rotation);
        double qk = std::get<3>(rotation);

        double m[9];
        m[0] = 2 * (qj * qj + qk * qk);
        m[1] = 2 * (qi * qj - qk * qr);
        m[2] = 2 * (qi * qk + qj * qr);
        m[3] = 2 * (qi * qj + qk * qr);
        m[4] = 2 * (qi * qi + qk * qk);
        m[5] = 2 * (qj * qk - qi * qr);
        m[6] = 2 * (qi * qk - qj * qr);
        m[7] = 2 * (qj * qk + qi * qr);
        m[8] = 2 * (qi * qi + qj * qj);
        m[0] = 1 - m[0];
        m[4] = 1 - m[4];
        m[8] = 1 - m[8];

        for (size_t index = 0; index < vertices.size<0>(); index++) {
          // rotate and translate
          for (int i = 0; i < 3; i++) {
            srf_vertices(index, i) = 0;
            for (int j = 0; j < 3; j++) {
              srf_vertices(index, i) +=
                  m[j + 3 * i] * (vertices(index, j) + shift[j]);
            }
          }
        }

        for (size_t index = 0; index < triangles.size<0>(); index++) {
          for (int coord = 0; coord < 3; coord++) {
            srf_triangles(index, coord) = triangles(index, coord);
          }
        }

        srf_version = createQSharedPointer<
            vx::RefObjWrapper<de::uni_stuttgart::Voxie::DataVersion>>(
            dbusClient,
            HANDLEDBUSPENDINGREPLY(update->Finish(
                dbusClient.clientPath(), QMap<QString, QDBusVariant>())));
      }
      auto outputPath = vx::dbusGetVariantValue<QDBusObjectPath>(
          properties["de.uni_stuttgart.Voxie.Output"]);

      QMap<QString, QDBusVariant> outputResult;
      outputResult["Data"] = vx::dbusMakeVariant<QDBusObjectPath>(srf.path());

      outputResult["DataVersion"] =
          vx::dbusMakeVariant<QDBusObjectPath>(srf_version->path());

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
