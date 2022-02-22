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
#include <VoxieClient/DBusUtil.hpp>
#include <VoxieClient/Exception.hpp>
#include <VoxieClient/Exceptions.hpp>
#include <VoxieClient/MappedBuffer.hpp>
#include <VoxieClient/QtUtil.hpp>
#include <VoxieClient/RefCountHolder.hpp>

#include <ExtFilterModifySurface/BilateralFiltering.hpp>
#include <ExtFilterModifySurface/FastEffectiveDPFilter.hpp>
#include <ExtFilterModifySurface/FeatureConvincedDenoising.hpp>
#include <ExtFilterModifySurface/MeanNormalFiltering.hpp>
#include <ExtFilterModifySurface/NoiseApplicator.hpp>
#include <ExtFilterModifySurface/ProgressiveMeshDecimation.hpp>
#include <ExtFilterModifySurface/TaubinFiltering.hpp>

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
      throw vx::Exception("de.uni_stuttgart.Voxie.ExtFilterModifySurface.Error",
                          "argc is smaller than 1");

    QCommandLineParser parser;
    parser.setApplicationDescription("Example filter for modifying a surface");
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

      const auto properties =
          vx::dbusGetVariantValue<QMap<QString, QDBusVariant>>(
              pars[filterPath]["Properties"]);
      auto filterName =
          vx::dbusGetVariantValue<QString>(pars[filterPath]["PrototypeName"]);

      auto inputPath = vx::dbusGetVariantValue<QDBusObjectPath>(
          properties["de.uni_stuttgart.Voxie.Input"]);
      auto outputPath = vx::dbusGetVariantValue<QDBusObjectPath>(
          properties["de.uni_stuttgart.Voxie.Output"]);
      auto inputDataPath =
          vx::dbusGetVariantValue<QDBusObjectPath>(pars[inputPath]["Data"]);

      auto inputData = makeSharedQObject<de::uni_stuttgart::Voxie::SurfaceData>(
          dbusClient.uniqueName(), inputDataPath.path(),
          dbusClient.connection());

      auto srfT = makeSharedQObject<
          de::uni_stuttgart::Voxie::SurfaceDataTriangleIndexed>(
          dbusClient.uniqueName(), inputDataPath.path(),
          dbusClient.connection());

      vx::Array2<const uint32_t> triangles(HANDLEDBUSPENDINGREPLY(
          srfT->GetTrianglesReadonly(QMap<QString, QDBusVariant>())));
      vx::Array2<const float> vertices(HANDLEDBUSPENDINGREPLY(
          srfT->GetVerticesReadonly(QMap<QString, QDBusVariant>())));

      auto triangleCount = triangles.size<0>();

      QScopedPointer<ProgressiveMeshDecimation> pmdFilter;
      if (filterName ==
          "de.uni_stuttgart.Voxie.Filter.Surface.ProgressiveMeshDecimation") {
        auto pmd = vx::dbusGetVariantValue<double>(
            properties["de.uni_stuttgart.Voxie.Filter.Surface."
                       "ProgressiveMeshDecimation.PMDReduction"]);
        auto pmdAngle = vx::dbusGetVariantValue<double>(
            properties["de.uni_stuttgart.Voxie.Filter.Surface."
                       "ProgressiveMeshDecimation.PMDAngle"]);

        pmdFilter.reset(new ProgressiveMeshDecimation());

        pmdFilter->compute(vertices, triangles, pmd, pmdAngle, op);

        triangleCount = pmdFilter->getRemainingTriangleCount();
      }

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
            srf2t_triangles(i, j) = triangles(i, j);

        if (pmdFilter) {
          pmdFilter->getTrianglesResults(srf2t_triangles);
        }
        vx::RefObjWrapper<de::uni_stuttgart::Voxie::DataVersion> version(
            dbusClient,
            HANDLEDBUSPENDINGREPLY(update->Finish(
                dbusClient.clientPath(), QMap<QString, QDBusVariant>())));
      }
      QSharedPointer<vx::RefObjWrapper<de::uni_stuttgart::Voxie::DataVersion>>
          srf2_version;
      auto vertexCount = vertices.size<0>();

      if (pmdFilter) {
        vertexCount = pmdFilter->getRemainingVertexCount();
      }
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
          srf2_vertices(i, 0) = vertices(i, 0);
          srf2_vertices(i, 1) = vertices(i, 1);
          srf2_vertices(i, 2) = vertices(i, 2);
        }

        if (pmdFilter) {
          pmdFilter->getVerticesResults(srf2_vertices);
        } else if (filterName ==
                   "de.uni_stuttgart.Voxie.Filter.Surface.TaubinFiltering") {
          auto iterationsTaub = vx::dbusGetVariantValue<qint64>(
              properties
                  ["de.uni_stuttgart.Voxie.Filter.Surface.TaubinFiltering."
                   "TaubinIterations"]);
          auto att = vx::dbusGetVariantValue<double>(
              properties
                  ["de.uni_stuttgart.Voxie.Filter.Surface.TaubinFiltering."
                   "Attenuation"]);

          TaubinFiltering filter(iterationsTaub, att);
          filter.compute(srf2_vertices, triangles, op);
        } else if (filterName ==
                   "de.uni_stuttgart.Voxie.Filter.Surface."
                   "MeanNormalFiltering") {
          auto iterationsMNF = vx::dbusGetVariantValue<qint64>(
              properties["de.uni_stuttgart.Voxie.Filter.Surface."
                         "MeanNormalFiltering.MeanIterations"]);

          MeanNormalFiltering filter;
          filter.compute(srf2_vertices, triangles, iterationsMNF, op);
        } else if (filterName ==
                   "de.uni_stuttgart.Voxie.Filter.Surface.BilateralFiltering") {
          auto sigmaSpacial = vx::dbusGetVariantValue<double>(
              properties["de.uni_stuttgart.Voxie.Filter.Surface."
                         "BilateralFiltering.BilateralSigmaSpacial"]);
          auto sigmaSignal = vx::dbusGetVariantValue<double>(
              properties["de.uni_stuttgart.Voxie.Filter.Surface."
                         "BilateralFiltering.BilateralSigmaSignal"]);

          BilateralFiltering filter(sigmaSpacial, sigmaSignal);
          filter.compute(srf2_vertices, triangles, op);
        } else if (filterName ==
                   "de.uni_stuttgart.Voxie.Filter.Surface."
                   "FastEffectiveDPFilter") {
          auto fedIterations = vx::dbusGetVariantValue<qint64>(
              properties["de.uni_stuttgart.Voxie.Filter.Surface."
                         "FastEffectiveDPFilter.FEDIterations"]);
          auto fedThreshold = vx::dbusGetVariantValue<double>(
              properties["de.uni_stuttgart.Voxie.Filter.Surface."
                         "FastEffectiveDPFilter.FEDThreshold"]);

          FastEffectiveDPFilter filter;
          filter.compute(srf2_vertices, triangles, fedThreshold, fedIterations,
                         op);
        } else if (filterName ==
                   "de.uni_stuttgart.Voxie.Filter.Surface."
                   "FeatureConvincedDenoising") {
          auto fcdExtensions = vx::dbusGetVariantValue<qint64>(
              properties["de.uni_stuttgart.Voxie.Filter.Surface."
                         "FeatureConvincedDenoising.FCDExtensions"]);
          // TODO: This is supposed to also have an "iterations" parameter

          FeatureConvincedDenoising filter(srf2_vertices, triangles);
          filter.compute(srf2_vertices, triangles, fcdExtensions, op);
        } else if (filterName ==
                   "de.uni_stuttgart.Voxie.Filter.Surface.NoiseApplicator") {
          auto noise = vx::dbusGetVariantValue<double>(
              properties
                  ["de.uni_stuttgart.Voxie.Filter.Surface.NoiseApplicator."
                   "NoiseLvl"]);

          NoiseApplicator filter;
          filter.addNoise(srf2_vertices, triangles, noise);
        } else {
          throw vx::Exception(
              "de.uni_stuttgart.Voxie.ExtFilterModifySurface.Error",
              "Unknown filter prototype name: '" + filterName + "'");
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
