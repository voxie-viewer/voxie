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

#include <QDebug>
#include <QtCore/QCommandLineParser>
#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QString>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>
#include <QtDBus/QDBusError>
#include <QtDBus/QDBusPendingReply>
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

#include "Cell.hpp"
#include "HausdorffDistance.hpp"

int main(int argc, char* argv[]) {
  try {
    if (argc < 2)
      throw vx::Exception(
          "de.uni_stuttgart.Voxie.ExtFilterHausdorffDistance.Error",
          "argc is smaller than 2");
    // create parser
    QCommandLineParser parser;
    parser.setApplicationDescription(
        "Filter for calculating Hausdorff Distance");
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

      // get properties
      auto properties = vx::dbusGetVariantValue<QMap<QString, QDBusVariant>>(
          pars[filterPath]["Properties"]);

      // get Nominal-Surface
      auto nominalInputPath = vx::dbusGetVariantValue<QDBusObjectPath>(
          properties
              ["de.uni_stuttgart.Voxie.Filter.HausdorffDistance.NominalInput"]);

      auto nominalInputDataPath = vx::dbusGetVariantValue<QDBusObjectPath>(
          pars[nominalInputPath]["Data"]);

      auto nominalInputData =
          makeSharedQObject<de::uni_stuttgart::Voxie::SurfaceData>(
              dbusClient.uniqueName(), nominalInputDataPath.path(),
              dbusClient.connection());

      auto srfT_nominal = makeSharedQObject<
          de::uni_stuttgart::Voxie::SurfaceDataTriangleIndexed>(
          dbusClient.uniqueName(), nominalInputDataPath.path(),
          dbusClient.connection());

      // get Actual-Surface
      auto actualInputPath = vx::dbusGetVariantValue<QDBusObjectPath>(
          properties
              ["de.uni_stuttgart.Voxie.Filter.HausdorffDistance.ActualInput"]);
      auto actualInputDataPath = vx::dbusGetVariantValue<QDBusObjectPath>(
          pars[actualInputPath]["Data"]);

      auto actualInputData =
          makeSharedQObject<de::uni_stuttgart::Voxie::SurfaceData>(
              dbusClient.uniqueName(), actualInputDataPath.path(),
              dbusClient.connection());

      auto srfT_actual = makeSharedQObject<
          de::uni_stuttgart::Voxie::SurfaceDataTriangleIndexed>(
          dbusClient.uniqueName(), actualInputDataPath.path(),
          dbusClient.connection());

      // get vertices of Surfaces
      vx::Array2<const uint32_t> triangles_nominal(HANDLEDBUSPENDINGREPLY(
          srfT_nominal->GetTrianglesReadonly(QMap<QString, QDBusVariant>())));

      vx::Array2<const float> vertices_nominal(HANDLEDBUSPENDINGREPLY(
          srfT_nominal->GetVerticesReadonly(QMap<QString, QDBusVariant>())));

      vx::Array2<const uint32_t> triangles_actual(HANDLEDBUSPENDINGREPLY(
          srfT_actual->GetTrianglesReadonly(QMap<QString, QDBusVariant>())));

      vx::Array2<const float> vertices_actual(HANDLEDBUSPENDINGREPLY(
          srfT_actual->GetVerticesReadonly(QMap<QString, QDBusVariant>())));

      // output surface
      auto outputPath = vx::dbusGetVariantValue<QDBusObjectPath>(
          properties["de.uni_stuttgart.Voxie.Output"]);

      QList<std::tuple<
          QString, QString, quint64, std::tuple<QString, quint32, QString>,
          QString, QMap<QString, QDBusVariant>, QMap<QString, QDBusVariant>>>
          attributes{std::make_tuple(
              "de.uni_stuttgart.Voxie.SurfaceAttribute.Distance",
              "de.uni_stuttgart.Voxie.SurfaceAttributeKind.Triangle",
              1,  // change '.Triangle' to '.Vertex' when vertex coloring works
              std::make_tuple("float", 32, "native"), "Distances",
              QMap<QString, QDBusVariant>(), QMap<QString, QDBusVariant>())};

      QMap<QString, QDBusVariant> options;
      options.insert("Attributes", vx::dbusMakeVariant(attributes));

      vx::RefObjWrapper<de::uni_stuttgart::Voxie::SurfaceDataTriangleIndexed>
          outputSurface(
              dbusClient,
              HANDLEDBUSPENDINGREPLY(
                  dbusClient->CreateSurfaceDataTriangleIndexed(
                      dbusClient.clientPath(), triangles_nominal.size<0>(),
                      vertices_nominal.size<0>(), nominalInputDataPath, false,
                      options)));

      auto outputSurfaceData =
          makeSharedQObject<de::uni_stuttgart::Voxie::Data>(
              dbusClient.uniqueName(), outputSurface.path().path(),
              dbusClient.connection());

      vx::RefObjWrapper<de::uni_stuttgart::Voxie::ExternalDataUpdate> update(
          dbusClient,
          HANDLEDBUSPENDINGREPLY(outputSurfaceData->CreateUpdate(
              dbusClient.clientPath(), QMap<QString, QDBusVariant>())));

      vx::Array2<float> outputDistances(
          HANDLEDBUSPENDINGREPLY(outputSurface->GetAttributeWritable(
              update.path(), "de.uni_stuttgart.Voxie.SurfaceAttribute.Distance",
              QMap<QString, QDBusVariant>())));

      vx::Array2<float> outputVertices(
          HANDLEDBUSPENDINGREPLY(outputSurface->GetVerticesWritable(
              update.path(), QMap<QString, QDBusVariant>())));
      for (size_t i = 0; i < vertices_nominal.size<0>(); i++) {
        outputVertices(i, 0) = vertices_nominal(i, 0);
        outputVertices(i, 1) = vertices_nominal(i, 1);
        outputVertices(i, 2) = vertices_nominal(i, 2);
      }

      HausdorffDistance filter(triangles_nominal, vertices_nominal,
                               triangles_actual, vertices_actual,
                               outputDistances);
      filter.run(op);

      // Define Output
      QMap<QString, QDBusVariant> outputResult;
      outputResult["Data"] =
          vx::dbusMakeVariant<QDBusObjectPath>(outputSurface.path());

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
