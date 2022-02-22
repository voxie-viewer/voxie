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

#include <ExtFilterIterativeClosestPoint/IterativeClosestPoint.hpp>

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
          "de.uni_stuttgart.Voxie.ExtFilterIterativeClosestPoint.Error",
          "argc is smaller than 1");

    QCommandLineParser parser;
    parser.setApplicationDescription(
        "Filter for matching two point clouds of same object");
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
      auto refSurfacePath = vx::dbusGetVariantValue<QDBusObjectPath>(
          properties["de.uni_stuttgart.Voxie.Filter.IterativeClosestPoint."
                     "ReferencePoints"]);
      auto refSurfaceDataPath = vx::dbusGetVariantValue<QDBusObjectPath>(
          pars[refSurfacePath]["Data"]);
      auto refSurfaceData = makeSharedQObject<
          de::uni_stuttgart::Voxie::SurfaceDataTriangleIndexed>(
          dbusClient.uniqueName(), refSurfaceDataPath.path(),
          dbusClient.connection());

      auto inSurfacePath = vx::dbusGetVariantValue<QDBusObjectPath>(
          properties["de.uni_stuttgart.Voxie.Filter.IterativeClosestPoint."
                     "InputPoints"]);
      auto inSurfaceDataPath =
          vx::dbusGetVariantValue<QDBusObjectPath>(pars[inSurfacePath]["Data"]);
      auto inSurfaceData = makeSharedQObject<
          de::uni_stuttgart::Voxie::SurfaceDataTriangleIndexed>(
          dbusClient.uniqueName(), inSurfaceDataPath.path(),
          dbusClient.connection());
      auto outputPath = vx::dbusGetVariantValue<QDBusObjectPath>(
          properties["de.uni_stuttgart.Voxie.Output"]);

      uint32_t numSamples =
          properties
              ["de.uni_stuttgart.Voxie.Filter.IterativeClosestPoint.Samples"]
                  .variant()
                  .toUInt();
      float supportRadius = properties
                                ["de.uni_stuttgart.Voxie.Filter."
                                 "IterativeClosestPoint.SupportRadius"]
                                    .variant()
                                    .toFloat();
      QString method = vx::dbusGetVariantValue<QString>(
          properties
              ["de.uni_stuttgart.Voxie.Filter.IterativeClosestPoint.Method"]);

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

      {
        IterativeClosestPoint filter;

        vx::Array2<const float> inputSurface1(
            HANDLEDBUSPENDINGREPLY(refSurfaceData->GetVerticesReadonly(
                QMap<QString, QDBusVariant>())));

        vx::Array2<const float> normals1(
            HANDLEDBUSPENDINGREPLY(refSurfaceData->GetAttributeReadonly(
                "de.uni_stuttgart.Voxie.SurfaceAttribute.Normal",
                QMap<QString, QDBusVariant>())));

        vx::Array2<const float> inputSurface2(HANDLEDBUSPENDINGREPLY(
            inSurfaceData->GetVerticesReadonly(QMap<QString, QDBusVariant>())));

        vx::Array2<const float> normals2(
            HANDLEDBUSPENDINGREPLY(inSurfaceData->GetAttributeReadonly(
                "de.uni_stuttgart.Voxie.SurfaceAttribute.Normal",
                QMap<QString, QDBusVariant>())));

        if (method ==
                "de.uni_stuttgart.Voxie.Filter.IterativeClosestPoint.Method."
                "FPFH" ||
            method ==
                "de.uni_stuttgart.Voxie.Filter.IterativeClosestPoint.Method."
                "PFH") {
          auto refKeyPointPath = vx::dbusGetVariantValue<QDBusObjectPath>(
              properties["de.uni_stuttgart.Voxie.Filter.IterativeClosestPoint."
                         "ReferenceKeyPoints"]);
          auto refKeyPointDataPath = vx::dbusGetVariantValue<QDBusObjectPath>(
              pars[refKeyPointPath]["Data"]);
          auto refKeyPointData = makeSharedQObject<
              de::uni_stuttgart::Voxie::SurfaceDataTriangleIndexed>(
              dbusClient.uniqueName(), refKeyPointDataPath.path(),
              dbusClient.connection());

          auto inKeyPointPath = vx::dbusGetVariantValue<QDBusObjectPath>(
              properties["de.uni_stuttgart.Voxie.Filter.IterativeClosestPoint."
                         "InputKeyPoints"]);
          auto inKeyPointDataPath = vx::dbusGetVariantValue<QDBusObjectPath>(
              pars[inKeyPointPath]["Data"]);
          auto inKeyPointData = makeSharedQObject<
              de::uni_stuttgart::Voxie::SurfaceDataTriangleIndexed>(
              dbusClient.uniqueName(), inKeyPointDataPath.path(),
              dbusClient.connection());

          vx::Array2<const float> keyPoints1(
              HANDLEDBUSPENDINGREPLY(refKeyPointData->GetVerticesReadonly(
                  QMap<QString, QDBusVariant>())));

          vx::Array2<const float> keyPoints2(
              HANDLEDBUSPENDINGREPLY(inKeyPointData->GetVerticesReadonly(
                  QMap<QString, QDBusVariant>())));

          filter.setKeyPoints(&keyPoints1, &keyPoints2);
        }

        filter.setNumSamples(numSamples);
        filter.compute(inputSurface1, normals1, inputSurface2, normals2,
                       supportRadius, method, translation, rotation, op);
      }

      QMap<QString, QDBusVariant> outputResult;
      outputResult["Data"] =
          vx::dbusMakeVariant<QDBusObjectPath>(inSurfaceDataPath);

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
