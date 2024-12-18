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
#include <VoxieClient/Vector.hpp>

#include <QtCore/QCommandLineParser>
#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QString>

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>
#include <QtDBus/QDBusError>
#include <QtDBus/QDBusPendingReply>

#include <fstream>

Q_NORETURN static void error(const QString& str) {
  throw vx::Exception("de.uni_stuttgart.Voxie.ExtPly.Error", str);
}

QCommandLineOption voxieImportFilename("voxie-import-filename", "File to load.",
                                       "filename");

/*
static void import(const QCommandLineParser& parser) {
  if (!parser.isSet(vx::ClaimedOperationBase::voxieOperationOption()))
    error("--voxie-operation is not set");
  QString operationPath =
      parser.value(vx::ClaimedOperationBase::voxieOperationOption());
  if (!parser.isSet(voxieImportFilename))
    error("--voxie-import-filename is not set");
  QString filename = parser.value(voxieImportFilename);

  vx::initDBusTypes();

  vx::DBusClient dbusClient(parser);

  vx::ClaimedOperation<de::uni_stuttgart::Voxie::ExternalOperationImport> op(
      dbusClient, QDBusObjectPath(operationPath));

  op.forwardExc([&]() {

    HANDLEDBUSPENDINGREPLY(op.opGen().SetProgress(1.0, vx::emptyOptions()));

    HANDLEDBUSPENDINGREPLY(op.op().Finish(QDBusObjectPath(srf2.path()),
                                          QDBusObjectPath(srf2_version->path()),
                                          vx::emptyOptions()));
  });
}
*/

// TODO: Should this use the rotation / position of the Volume node?

static void exportFile(const QCommandLineParser& parser) {
  if (!parser.isSet(vx::ClaimedOperationBase::voxieOperationOption()))
    error("--voxie-operation is not set");
  QString operationPath =
      parser.value(vx::ClaimedOperationBase::voxieOperationOption());
  // if (!parser.isSet(voxieExportFilename))
  //   error("--voxie-export-filename is not set");
  // QString filename = parser.value(voxieExportFilename);

  vx::initDBusTypes();

  vx::DBusClient dbusClient(parser);

  vx::ClaimedOperation<de::uni_stuttgart::Voxie::ExternalOperationExport> op(
      dbusClient, QDBusObjectPath(operationPath));

  op.forwardExc([&]() {
    QString filename = op.op().filename();
    auto inputDataPath = op.op().data();

    auto volumeData = makeSharedQObject<de::uni_stuttgart::Voxie::VolumeData>(
        dbusClient.uniqueName(), inputDataPath.path(), dbusClient.connection());
    auto voxelData =
        makeSharedQObject<de::uni_stuttgart::Voxie::VolumeDataVoxel>(
            dbusClient.uniqueName(), inputDataPath.path(),
            dbusClient.connection());

    // TODO: Determine this automatically
    // using DataType = float;
    using DataType = quint8;
    vx::Array3<const DataType> data(HANDLEDBUSPENDINGREPLY(
        voxelData->GetDataReadonly(QMap<QString, QDBusVariant>())));

    auto volumeOriginTuple = volumeData->volumeOrigin();
    vx::Vector<double, 3> volumeOrigin{
        std::get<0>(volumeOriginTuple),
        std::get<1>(volumeOriginTuple),
        std::get<2>(volumeOriginTuple),
    };

    auto gridSpacingTuple = voxelData->gridSpacing();
    vx::Vector<double, 3> gridSpacing{
        std::get<0>(gridSpacingTuple),
        std::get<1>(gridSpacingTuple),
        std::get<2>(gridSpacingTuple),
    };
    double volume = 1;
    for (int i = 0; i < 3; i++) volume *= gridSpacing[i];

    std::filebuf fb;
    if (!fb.open(filename.toStdString(), std::ios::out))
      error("Failed to open '" + filename + "'");
    std::ostream stream(&fb);
    if (!stream.good()) error("Opening stream failed");

    // TODO: precision?
    stream << std::scientific;

    HANDLEDBUSPENDINGREPLY(op.opGen().SetProgress(0.03, vx::emptyOptions()));

    for (size_t z = 0; z < data.template size<2>(); z++) {
      QCoreApplication::instance()
          ->processEvents();  // check for Cancelled signal
      op.throwIfCancelled();

      for (size_t y = 0; y < data.template size<1>(); y++) {
        for (size_t x = 0; x < data.template size<0>(); x++) {
          vx::Vector<size_t, 3> posInt{x, y, z};
          // TODO: Should this be the center of the voxel?
          // TODO: Unit?
          vx::Vector<double, 3> halfVoxel{0.5, 0.5, 0.5};
          auto posCenter = vx::vectorCastNarrow<double>(posInt) + halfVoxel;
          auto pos = volumeOrigin + elementwiseProduct(gridSpacing, posCenter);
          stream << pos[0] << " " << pos[1] << " " << pos[2] << " "
                 << +data(x, y, z) << " " << volume << "\n";
        }
      }
      if (!stream.good()) error("Writing to stream failed");
      HANDLEDBUSPENDINGREPLY(op.opGen().SetProgress(
          0.03 + 0.94 * (z + 1) / data.template size<2>(), vx::emptyOptions()));
    }

    HANDLEDBUSPENDINGREPLY(op.opGen().SetProgress(0.97, vx::emptyOptions()));

    if (!stream.good()) error("Writing to stream failed");

    HANDLEDBUSPENDINGREPLY(op.op().Finish(vx::emptyOptions()));
  });
}

int main(int argc, char* argv[]) {
  try {
    if (argc < 1) error("argc is smaller than 1");

    QCommandLineParser parser;
    parser.setApplicationDescription("Voxie script for loading PLY files");
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addOptions(vx::DBusClient::options());
    parser.addOptions(vx::ClaimedOperationBase::options());
    parser.addOption(voxieImportFilename);

    QStringList args;
    for (char** arg = argv; *arg; arg++) args.push_back(*arg);
    int argc0 = 1;
    char* args0[2] = {argv[0], NULL};
    QCoreApplication app(argc0, args0);
    parser.process(args);

    QString action = "";
    if (parser.isSet(vx::ClaimedOperationBase::voxieActionOption()))
      action = parser.value(vx::ClaimedOperationBase::voxieActionOption());
    if (action == "Import")
      error("TODO: Importer?");
    else if (action == "Export")
      exportFile(parser);
    else
      error("--voxie-action is not 'Import' or 'Export'");

    return 0;
  } catch (vx::Exception& error) {
    QTextStream(stderr) << error.name() << ": " << error.message() << endl
                        << flush;
    return 1;
  }
}
