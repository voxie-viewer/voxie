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

#include <ExtFilterMarchingCubes33_by_DVega/marchingcubes33.h>

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

      grid3d grid;
      grid.setDataPointer(std::get<0>(dimensions), std::get<1>(dimensions),
                          std::get<2>(dimensions),
                          // TODO: Why is a const_cast needed here?
                          const_cast<GRD_data_type*>(inputVolume.data()));
      grid.setOrigin(std::get<0>(inputVolumeData->volumeOrigin()),
                     std::get<1>(inputVolumeData->volumeOrigin()),
                     std::get<2>(inputVolumeData->volumeOrigin()));
      grid.setRatioAspect(std::get<0>(inputDataVoxel->gridSpacing()),
                          std::get<1>(inputDataVoxel->gridSpacing()),
                          std::get<2>(inputDataVoxel->gridSpacing()));

      // Run Marching Cubes
      MC33 mc33;
      mc33.set_grid3d(&grid);

      Surface* extractedSurface = mc33.calculate_isosurface(isovalue);

      // Creating the output
      auto triangleCount = extractedSurface->getNumberOfTriangles();

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
            srf2t_triangles(i, j) = extractedSurface->getTriangle(i)[j];
      }
      QSharedPointer<vx::RefObjWrapper<de::uni_stuttgart::Voxie::DataVersion>>
          srf2_version;
      auto vertexCount = extractedSurface->getNumberOfVertices();

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
          srf2_vertices(i, 0) = extractedSurface->getVertex(i)[0];
          srf2_vertices(i, 1) = extractedSurface->getVertex(i)[1];
          srf2_vertices(i, 2) = extractedSurface->getVertex(i)[2];
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
