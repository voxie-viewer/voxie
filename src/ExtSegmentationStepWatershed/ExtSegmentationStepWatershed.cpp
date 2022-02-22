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

#include <ExtSegmentationStepWatershed/Watershed.hpp>

#include <QtCore/QCommandLineParser>
#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QString>

#include <QDebug>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>
#include <QtDBus/QDBusError>
#include <QtDBus/QDBusPendingReply>

#include <Voxie/Node/PropertyValueConvertRaw.hpp>
#include <Voxie/Node/Types.hpp>

int main(int argc, char* argv[]) {
  try {
    if (argc < 1)
      throw vx::Exception(
          "de.uni_stuttgart.Voxie.ExtSegmentationStepWatershed.Error",
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

    vx::ClaimedOperation<
        de::uni_stuttgart::Voxie::ExternalOperationRunSegmentationStep>
        op(dbusClient, vx::ClaimedOperationBase::getOperationPath(
                           parser, "RunSegmentationStep"));
    op.forwardExc([&]() {
      auto pars = op.op().parameters();

      auto properties = vx::dbusGetVariantValue<QMap<QString, QDBusVariant>>(
          pars[op.op().segmentationStepNode()]["Properties"]);

      // Load sigma
      int sigmaData = vx::dbusGetVariantValue<qint64>(
          properties["de.uni_stuttgart.Voxie.SegmentationStep."
                     "ExtSegmentationStepWatershed.Sigma"]);

      // Load execution kind
      QString executionKind = vx::dbusGetVariantValue<QString>(
          properties["de.uni_stuttgart.Voxie.SegmentationStep."
                     "ExtSegmentationStepWatershed.ExecutionKind"]);

      // Load existing label list
      QList<quint64> seedLabelIDs = vx::dbusGetVariantValue<QList<quint64>>(
          properties["de.uni_stuttgart.Voxie.SegmentationStep."
                     "ExtSegmentationStepWatershed.Seeds"]);

      // Load label table
      auto containerDataPath = op.op().containerData();
      auto containerData =
          makeSharedQObject<de::uni_stuttgart::Voxie::ContainerData>(
              dbusClient.uniqueName(), containerDataPath.path(),
              dbusClient.connection());

      auto labelTablePath =
          HANDLEDBUSPENDINGREPLY(
              containerData->GetElement("labelTable",
                                        QMap<QString, QDBusVariant>()))
              .path();
      auto labelTable = makeSharedQObject<de::uni_stuttgart::Voxie::TableData>(
          dbusClient.uniqueName(), labelTablePath, dbusClient.connection());

      // Create a map from column name to label table column index
      auto labelColumns = labelTable->columns();
      QHash<QString, int> columnsByName;
      for (int i = 0; i < labelColumns.size(); i++) {
        columnsByName.insert(std::get<0>(labelColumns[i]), i);
      }

      auto labelRows = HANDLEDBUSPENDINGREPLY(
          labelTable->GetRows(0, std::numeric_limits<quint64>::max(),
                              QMap<QString, QDBusVariant>()));

      QList<vx::SegmentationType> selectedLabels;
      for (vx::SegmentationType labelID : seedLabelIDs) {
        selectedLabels.push_back(labelID);
      }

      if (selectedLabels.size() < 2) {
        throw vx::Exception(
            "de.uni_stuttgart.Voxie.InvalidOperation",
            QString("Select at least two labels to be used as seed points for "
                    "the watershed algorithm."));
      }
      qDebug() << "Label count: " << selectedLabels.size();
      qDebug() << "Sigma: " << sigmaData;

      // Load volume
      auto volumePath = vx::dbusGetVariantValue<QDBusObjectPath>(
          properties["de.uni_stuttgart.Voxie.SegmentationStep."
                     "ExtSegmentationStepWatershed.Volume"]);
      if (volumePath == QDBusObjectPath("/")) {
        volumePath = op.op().inputNode();
      }

      auto volumeDataPath =
          vx::dbusGetVariantValue<QDBusObjectPath>(pars[volumePath]["Data"]);

      auto volumeData = makeSharedQObject<de::uni_stuttgart::Voxie::VolumeData>(
          dbusClient.uniqueName(), volumeDataPath.path(),
          dbusClient.connection());
      auto volumeDataVoxel =
          makeSharedQObject<de::uni_stuttgart::Voxie::VolumeDataVoxel>(
              dbusClient.uniqueName(), volumeDataPath.path(),
              dbusClient.connection());

      // Load label volume
      auto labelVolumePath = op.op().labelData();

      auto labelVolumeData =
          makeSharedQObject<de::uni_stuttgart::Voxie::VolumeData>(
              dbusClient.uniqueName(), labelVolumePath.path(),
              dbusClient.connection());
      auto labelVolumeDataVoxel =
          makeSharedQObject<de::uni_stuttgart::Voxie::VolumeDataVoxel>(
              dbusClient.uniqueName(), labelVolumePath.path(),
              dbusClient.connection());
      auto labelVolumeVoxieData =
          makeSharedQObject<de::uni_stuttgart::Voxie::Data>(
              dbusClient.uniqueName(), labelVolumeDataVoxel->path(),
              dbusClient.connection());

      // Load container data
      auto containerPath = op.op().containerData();
      auto containerVoxieData =
          makeSharedQObject<de::uni_stuttgart::Voxie::Data>(
              dbusClient.uniqueName(), containerPath.path(),
              dbusClient.connection());

      // Load labelTable data
      auto labelTableVoxieData =
          makeSharedQObject<de::uni_stuttgart::Voxie::Data>(
              dbusClient.uniqueName(), labelTablePath, dbusClient.connection());
      qDebug() << "Loaded required data for watershed";

      QSharedPointer<vx::RefObjWrapper<de::uni_stuttgart::Voxie::DataVersion>>
          labelVolumeVersion;
      QSharedPointer<vx::RefObjWrapper<de::uni_stuttgart::Voxie::DataVersion>>
          labelTableVersion;
      QSharedPointer<vx::RefObjWrapper<de::uni_stuttgart::Voxie::DataVersion>>
          containerVersion;
      {
        vx::RefObjWrapper<de::uni_stuttgart::Voxie::ExternalDataUpdate>
            containerUpdate(
                dbusClient,
                HANDLEDBUSPENDINGREPLY(containerVoxieData->CreateUpdate(
                    dbusClient.clientPath(), QMap<QString, QDBusVariant>())));

        vx::RefObjWrapper<de::uni_stuttgart::Voxie::ExternalDataUpdate>
            labelVolumeUpdate(
                dbusClient,
                HANDLEDBUSPENDINGREPLY(labelVolumeVoxieData->CreateUpdate(
                    dbusClient.clientPath(),
                    QMap<QString, QDBusVariant>{
                        {"ContainerUpdates",
                         QDBusVariant(QVariant::fromValue(
                             QMap<QDBusObjectPath, QDBusObjectPath>{
                                 {containerPath,
                                  containerUpdate.path()}}))}})));

        vx::RefObjWrapper<de::uni_stuttgart::Voxie::ExternalDataUpdate>
            labelTableUpdate(
                dbusClient,
                HANDLEDBUSPENDINGREPLY(labelTableVoxieData->CreateUpdate(
                    dbusClient.clientPath(),
                    QMap<QString, QDBusVariant>{
                        {"ContainerUpdates",
                         QDBusVariant(QVariant::fromValue(
                             QMap<QDBusObjectPath, QDBusObjectPath>{
                                 {containerPath,
                                  containerUpdate.path()}}))}})));

        vx::Array3<const float> volumeDataArray(HANDLEDBUSPENDINGREPLY(
            volumeDataVoxel->GetDataReadonly(QMap<QString, QDBusVariant>())));

        vx::Array3<vx::SegmentationType> labelVolumeDataArray(
            HANDLEDBUSPENDINGREPLY(labelVolumeDataVoxel->GetDataWritable(
                labelVolumeUpdate.path(), QMap<QString, QDBusVariant>())));

        Watershed watershed(volumeDataArray, labelVolumeDataArray,
                            selectedLabels, sigmaData);

        if (executionKind ==
            "de.uni_stuttgart.Voxie.SegmentationStep."
            "ExtSegmentationStepWatershed.ExecutionKind.Parallel") {
          watershed.runParallel(op);
          qDebug() << "Parallel watershed executed";
        } else {
          watershed.run(op);
          qDebug() << "Sequential watershed executed";
        }

        // Remove other labels and update voxelCount and percentage
        for (auto row : labelRows) {
          HANDLEDBUSPENDINGREPLY(
              labelTable->RemoveRow(labelTableUpdate.path(), std::get<0>(row),
                                    QMap<QString, QDBusVariant>()));

          QList<QVariant> rowData;
          for (auto v : std::get<1>(row)) {
            rowData.push_back(v.variant());
          }

          if (selectedLabels.contains(
                  rowData[columnsByName["LabelID"]].toUInt())) {
            size_t labelVoxelCount =
                watershed.labelVoxelCounts[rowData[columnsByName["LabelID"]]
                                               .toUInt()];
            rowData[columnsByName["Voxels"]] =
                QVariant::fromValue<qint64>(labelVoxelCount);
            rowData[columnsByName["Percent"]] = QVariant::fromValue<double>(
                (double)labelVoxelCount / watershed.getVoxelCount() * 100);
          } else {
            rowData[columnsByName["Voxels"]] = QVariant::fromValue<qint64>(0);
            rowData[columnsByName["Percent"]] = QVariant::fromValue<double>(0);
          }

          HANDLEDBUSPENDINGREPLY(labelTable->AddRow(
              labelTableUpdate.path(), rowData, QMap<QString, QDBusVariant>()));
        }

        labelVolumeVersion = createQSharedPointer<
            vx::RefObjWrapper<de::uni_stuttgart::Voxie::DataVersion>>(
            dbusClient,
            HANDLEDBUSPENDINGREPLY(labelVolumeUpdate->Finish(
                dbusClient.clientPath(), QMap<QString, QDBusVariant>())));

        labelTableVersion = createQSharedPointer<
            vx::RefObjWrapper<de::uni_stuttgart::Voxie::DataVersion>>(
            dbusClient,
            HANDLEDBUSPENDINGREPLY(labelTableUpdate->Finish(
                dbusClient.clientPath(), QMap<QString, QDBusVariant>())));

        containerVersion = createQSharedPointer<
            vx::RefObjWrapper<de::uni_stuttgart::Voxie::DataVersion>>(
            dbusClient,
            HANDLEDBUSPENDINGREPLY(containerUpdate->Finish(
                dbusClient.clientPath(), QMap<QString, QDBusVariant>())));
      }

      HANDLEDBUSPENDINGREPLY(op.op().Finish(labelVolumeVersion->path(),
                                            QMap<QString, QDBusVariant>()));
    });
    return 0;
  } catch (vx::Exception& error) {
    QTextStream(stderr) << error.name() << ": " << error.message() << endl
                        << flush;
    return 1;
  }
}
