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

#include <queue>

// typedef std::vector<std::tuple<size_t, size_t, size_t>> TodoStruct;
// typedef std::priority_queue<std::tuple<size_t, size_t, size_t>> TodoStruct;
typedef std::queue<std::tuple<size_t, size_t, size_t>> TodoStruct;

static inline void add(TodoStruct& todo, vx::Array3<uint8_t>& flood, size_t x,
                       size_t y, size_t z) {
  if (flood(x, y, z)) return;
  flood(x, y, z) = 1;
  todo.push(std::make_tuple(x, y, z));
}

static inline void maybeAdd(TodoStruct& todo, vx::Array3<uint8_t>& flood,
                            vx::Array3<const float>& inputVolume, size_t x,
                            size_t y, size_t z) {
  if (inputVolume(x, y, z)) return;
  add(todo, flood, x, y, z);
}

static void floodFill(
    vx::Array3<const float>& inputVolume, vx::Array3<float>& outputVolume,
    double fillValue,
    vx::ClaimedOperation<de::uni_stuttgart::Voxie::ExternalOperationRunFilter>&
        op) {
  if (inputVolume.size<0>() != outputVolume.size<0>() ||
      inputVolume.size<1>() != outputVolume.size<1>() ||
      inputVolume.size<2>() != outputVolume.size<2>())
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.ExtFilterFloodFill.InternalError",
        "Volume size mismatch");

  vx::Array3<uint8_t> flood(
      {outputVolume.size<0>(), outputVolume.size<1>(), outputVolume.size<2>()});
  TodoStruct todo;

  for (size_t z = 0; z < outputVolume.size<2>(); z++) {
    HANDLEDBUSPENDINGREPLY(
        op.opGen().SetProgress(0.3 * z / outputVolume.size<2>(), {}));
    for (size_t y = 0; y < outputVolume.size<1>(); y++) {
      for (size_t x = 0; x < outputVolume.size<0>(); x++) {
        outputVolume(x, y, z) = inputVolume(x, y, z);
        flood(x, y, z) = 0;
        if (x == 0 || y == 0 || z == 0 || x == outputVolume.size<0>() ||
            y == outputVolume.size<1>() || z == outputVolume.size<2>()) {
          maybeAdd(todo, flood, inputVolume, x, y, z);
        }
      }
    }
  }

  while (!todo.empty()) {
    auto value = todo.front();
    todo.pop();

    auto x = std::get<0>(value);
    auto y = std::get<1>(value);
    auto z = std::get<2>(value);

    if (x > 0) maybeAdd(todo, flood, inputVolume, x - 1, y, z);
    if (y > 0) maybeAdd(todo, flood, inputVolume, x, y - 1, z);
    if (z > 0) maybeAdd(todo, flood, inputVolume, x, y, z - 1);
    if (x < outputVolume.size<0>() - 1)
      maybeAdd(todo, flood, inputVolume, x + 1, y, z);
    if (y < outputVolume.size<1>() - 1)
      maybeAdd(todo, flood, inputVolume, x, y + 1, z);
    if (z < outputVolume.size<2>() - 1)
      maybeAdd(todo, flood, inputVolume, x, y, z + 1);
  }

  for (size_t z = 0; z < outputVolume.size<2>(); z++) {
    HANDLEDBUSPENDINGREPLY(
        op.opGen().SetProgress(0.7 + 0.3 * z / outputVolume.size<2>(), {}));
    for (size_t y = 0; y < outputVolume.size<1>(); y++) {
      for (size_t x = 0; x < outputVolume.size<0>(); x++) {
        if (!inputVolume(x, y, z) && !flood(x, y, z))
          outputVolume(x, y, z) = fillValue;
      }
    }
  }
}

int main(int argc, char* argv[]) {
  try {
    if (argc < 1)
      throw vx::Exception("de.uni_stuttgart.Voxie.ExtFilterFloodFill.Error",
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

      auto properties = vx::dbusGetVariantValue<QMap<QString, QDBusVariant>>(
          pars[filterPath]["Properties"]);

      // for (QString key : properties.keys())
      // qDebug() << "QQ" << key;
      auto fillValue = vx::dbusGetVariantValue<double>(
          properties["de.uni_stuttgart.Voxie.Filter.FloodFill.FillValue"]);

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
          dbusClient, HANDLEDBUSPENDINGREPLY(dbusClient->CreateVolumeDataVoxel(
                          dbusClient.clientPath(), size, inputData->dataType(),
                          inputData->volumeOrigin(),
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

        vx::Array3<float> volumeData(
            HANDLEDBUSPENDINGREPLY(volume->GetDataWritable(
                update.path(), QMap<QString, QDBusVariant>())));

        floodFill(inputVolume, volumeData, fillValue, op);

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
