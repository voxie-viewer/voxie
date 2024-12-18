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

#include "VerifySurface.hpp"

#include <VoxieClient/Array.hpp>
#include <VoxieClient/ClaimedOperation.hpp>
#include <VoxieClient/DBusClient.hpp>
#include <VoxieClient/DBusProxies.hpp>
#include <VoxieClient/DBusTypeList.hpp>
#include <VoxieClient/DBusUtil.hpp>
#include <VoxieClient/Exception.hpp>
#include <VoxieClient/Exceptions.hpp>
#include <VoxieClient/Format.hpp>
#include <VoxieClient/MappedBuffer.hpp>
#include <VoxieClient/QtUtil.hpp>
#include <VoxieClient/RefCountHolder.hpp>

#include <QtCore/QCommandLineParser>
#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QString>

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>
#include <QtDBus/QDBusError>
#include <QtDBus/QDBusPendingReply>

#include <sstream>

// TODO: Move this somewhere else / integrate into Vector.hpp?
namespace fmt {
template <class CharT, typename T, std::size_t dim>
struct formatter<vx::Vector<T, dim>, CharT>
    : fmt::formatter<std::string, CharT> {
  template <class FormatContext>
  auto format(const vx::Vector<T, dim>& t, FormatContext& fc) const {
    std::stringstream s;
    s << t;
    return fmt::formatter<std::string, CharT>::format(s.str(), fc);
  }
};
}  // namespace fmt

int main(int argc, char* argv[]) {
  try {
    if (argc < 2)
      throw vx::Exception("de.uni_stuttgart.Voxie.ExtFilterVerifySurface.Error",
                          "argc is smaller than 2");
    // create parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Filter to check for holes in a surface");
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
      auto filterPath = op.op().filterNode();
      auto pars = op.op().parameters();
      // qDebug() << filterPath.path();
      QString name = op.opGen().name();

      // get properties
      auto properties = vx::dbusGetVariantValue<QMap<QString, QDBusVariant>>(
          pars[filterPath]["Properties"]);

      // get Surface
      auto inputPath = vx::dbusGetVariantValue<QDBusObjectPath>(
          properties["de.uni_stuttgart.Voxie.Filter.VerifySurface.Input"]);

      auto inputDataPath =
          vx::dbusGetVariantValue<QDBusObjectPath>(pars[inputPath]["Data"]);

      auto srfT = makeSharedQObject<
          de::uni_stuttgart::Voxie::SurfaceDataTriangleIndexed>(
          dbusClient.uniqueName(), inputDataPath.path(),
          dbusClient.connection());

      // get vertices of Surfaces
      vx::Array2<const uint32_t> triangles(HANDLEDBUSPENDINGREPLY(
          srfT->GetTrianglesReadonly(QMap<QString, QDBusVariant>())));

      vx::Array2<const float> vertices(HANDLEDBUSPENDINGREPLY(
          srfT->GetVerticesReadonly(QMap<QString, QDBusVariant>())));

      if (name == "de.uni_stuttgart.Voxie.Filter.VerifySurface") {
        VerifySurface filter(triangles, vertices, nullptr);

        filter.run(op);

        // Finish
        QMap<QDBusObjectPath, QMap<QString, QDBusVariant>> result;
        HANDLEDBUSPENDINGREPLY(
            op.op().Finish(result, QMap<QString, QDBusVariant>()));
      } else if (name == "de.uni_stuttgart.Voxie.Filter.VerifySurfaceReport") {
        auto reportPath = vx::dbusGetVariantValue<QDBusObjectPath>(
            properties["de.uni_stuttgart.Voxie.Filter.VerifySurfaceReport."
                       "OutputReport"]);
        auto brokenEdgesPath = vx::dbusGetVariantValue<QDBusObjectPath>(
            properties["de.uni_stuttgart.Voxie.Filter.VerifySurfaceReport."
                       "OutputBrokenEdges"]);

        QString report;

        VerifySurface filter(triangles, vertices, &report);

        filter.run(op);

        QByteArray reportUtf8 = report.toUtf8();
        vx::RefObjWrapper<de::uni_stuttgart::Voxie::FileDataByteStream>
            reportData(
                dbusClient,
                HANDLEDBUSPENDINGREPLY(dbusClient->CreateFileDataByteStream(
                    dbusClient.clientPath(),
                    "text/markdown; charset=UTF-8; variant=CommonMark",
                    reportUtf8.size(), vx::emptyOptions())));
        auto reportDataGen =
            reportData.castUnchecked<de::uni_stuttgart::Voxie::Data>();
        vx::RefObjWrapper<de::uni_stuttgart::Voxie::ExternalDataUpdate>
            reportUpdate(dbusClient,
                         HANDLEDBUSPENDINGREPLY(reportDataGen->CreateUpdate(
                             dbusClient.clientPath(), vx::emptyOptions())));
        vx::Array1<uint8_t> reportBuffer(
            HANDLEDBUSPENDINGREPLY(reportData->GetContentWritable(
                reportUpdate.path(), vx::emptyOptions())));
        for (int i = 0; i < reportUtf8.size(); i++)
          reportBuffer(i) = reportUtf8[i];
        vx::RefObjWrapper<de::uni_stuttgart::Voxie::DataVersion> reportVersion(
            dbusClient, HANDLEDBUSPENDINGREPLY(reportUpdate->Finish(
                            dbusClient.clientPath(), vx::emptyOptions())));

        vx::RefObjWrapper<de::uni_stuttgart::Voxie::GeometricPrimitiveData>
            brokenEdgesData(
                dbusClient,
                HANDLEDBUSPENDINGREPLY(dbusClient->CreateGeometricPrimitiveData(
                    dbusClient.clientPath(), vx::emptyOptions())));
        auto brokenEdgesDataGen =
            brokenEdgesData.castUnchecked<de::uni_stuttgart::Voxie::Data>();
        vx::RefObjWrapper<de::uni_stuttgart::Voxie::ExternalDataUpdate>
            brokenEdgesUpdate(
                dbusClient,
                HANDLEDBUSPENDINGREPLY(brokenEdgesDataGen->CreateUpdate(
                    dbusClient.clientPath(), vx::emptyOptions())));
        auto components =
            makeSharedQObject<de::uni_stuttgart::Voxie::ComponentContainer>(
                dbusClient.uniqueName(), dbusClient->components().path(),
                dbusClient.connection());
        auto pointType = HANDLEDBUSPENDINGREPLY(components->GetComponent(
            "de.uni_stuttgart.Voxie.ComponentType.GeometricPrimitiveType",
            "de.uni_stuttgart.Voxie.GeometricPrimitive.Point",
            vx::emptyOptions()));
        uint32_t nr = 0;
        for (const auto& brokenEdge : filter.failedEdges) {
          auto v1 = std::get<0>(brokenEdge);
          auto v2 = std::get<1>(brokenEdge);
          vx::Vector<double, 3> v1v = {vertices(v1, 0), vertices(v1, 1),
                                       vertices(v1, 2)};
          vx::Vector<double, 3> v2v = {vertices(v2, 0), vertices(v2, 1),
                                       vertices(v2, 2)};
          auto diff = std::get<2>(brokenEdge);
          QString geomName =
              vx::format("Edge {} from {} ({}) to {} ({}) (diff {})", nr, v1,
                         v1v, v2, v2v, diff);
          nr++;

          QMap<QString, QDBusVariant> values;

          values["Position"] = vx::dbusMakeVariant<vx::TupleVector<double, 3>>(
              vx::toTupleVector(v1v));
          HANDLEDBUSPENDINGREPLY(brokenEdgesData->AddPrimitive(
              brokenEdgesUpdate.path(), pointType, geomName + " A", values,
              vx::emptyOptions()));

          values["Position"] = vx::dbusMakeVariant<vx::TupleVector<double, 3>>(
              vx::toTupleVector(v2v));
          HANDLEDBUSPENDINGREPLY(brokenEdgesData->AddPrimitive(
              brokenEdgesUpdate.path(), pointType, geomName + " B", values,
              vx::emptyOptions()));
        }
        vx::RefObjWrapper<de::uni_stuttgart::Voxie::DataVersion>
            brokenEdgesVersion(
                dbusClient, HANDLEDBUSPENDINGREPLY(brokenEdgesUpdate->Finish(
                                dbusClient.clientPath(), vx::emptyOptions())));

        // Finish
        QMap<QString, QDBusVariant> reportResult;
        reportResult["Data"] =
            vx::dbusMakeVariant<QDBusObjectPath>(reportData.path());
        reportResult["DataVersion"] =
            vx::dbusMakeVariant<QDBusObjectPath>(reportVersion.path());

        QMap<QString, QDBusVariant> brokenEdgesResult;
        brokenEdgesResult["Data"] =
            vx::dbusMakeVariant<QDBusObjectPath>(brokenEdgesData.path());
        brokenEdgesResult["DataVersion"] =
            vx::dbusMakeVariant<QDBusObjectPath>(brokenEdgesVersion.path());

        QMap<QDBusObjectPath, QMap<QString, QDBusVariant>> result;
        result[reportPath] = reportResult;
        result[brokenEdgesPath] = brokenEdgesResult;
        HANDLEDBUSPENDINGREPLY(
            op.op().Finish(result, QMap<QString, QDBusVariant>()));
      } else {
        throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                            vx::format("Got invalid filter name: '{}'", name));
      }
    });
    return 0;
  } catch (vx::Exception& error) {
    QTextStream(stderr) << error.name() << ": " << error.message() << endl
                        << flush;
    return 1;
  }
}
