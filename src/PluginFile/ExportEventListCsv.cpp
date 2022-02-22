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

#include "ExportEventListCsv.hpp"

#include <PluginFile/TypeErasedArray.hpp>

#include <VoxieBackend/Data/EventListDataAccessor.hpp>
#include <VoxieBackend/Data/EventListDataBuffer.hpp>

#include <VoxieBackend/IO/Operation.hpp>

#include <VoxieBackend/Property/PropertyType.hpp>

#include <VoxieClient/VoxieDBus.hpp>

#include <QtConcurrent/QtConcurrent>
#include <QtCore/QByteArray>
#include <QtCore/QFile>
#include <QtCore/QFuture>

using namespace vx;
using namespace vx::file;

ExportEventListCsv::ExportEventListCsv()
    : vx::io::Exporter(
          "de.uni_stuttgart.Voxie.FileFormat.EventList.Csv.Export",
          vx::io::FilenameFilter("Comma-separated values", {"*.csv"}),
          {"de.uni_stuttgart.Voxie.Data.EventListData"}) {}

QSharedPointer<vx::OperationResult> ExportEventListCsv::exportData(
    const QSharedPointer<vx::Data>& data, const QString& fileName) {
  return runThreaded(
      "Export " + fileName,
      [data, fileName](const QSharedPointer<vx::io::Operation>& op) {
        auto eventListData =
            qSharedPointerDynamicCast<vx::EventListDataAccessor>(data);
        if (!eventListData) {
          throw vx::Exception(
              "de.uni_stuttgart.Voxie.InternalError",
              "ExportEventListCsv::exportData(): data is not an event list");
        }

        ExporterImpl(*eventListData, *op, fileName).save();
      });
}

constexpr ExportEventListCsv::ExporterImpl::Timestamp
    ExportEventListCsv::ExporterImpl::TimestampBlockSize;

constexpr qulonglong ExportEventListCsv::ExporterImpl::BlockSize;

QThreadStorage<QSharedPointer<EventListDataBuffer>>
    ExportEventListCsv::ExporterImpl::bufferStorage;

ExportEventListCsv::ExporterImpl::ExporterImpl(
    vx::EventListDataAccessor& accessor, vx::io::Operation& op,
    QString fileName)
    : accessor(accessor),
      op(op),
      fileName(fileName),
      streamCount(accessor.getStreamCount()) {}

void ExportEventListCsv::ExporterImpl::save() {
  for (quint64 streamID = 0; streamID < streamCount; ++streamID) {
    QString effectiveFileName = fileName;

    // Append stream ID if there is more than 1 stream
    if (streamCount > 1) {
      QFileInfo fileInfo(fileName);
      effectiveFileName = fileInfo.dir().filePath(
          fileInfo.baseName() + "_" + QString::number(streamID) + "." +
          fileInfo.completeSuffix());
    }

    QFile file(effectiveFileName);
    if (file.open(QIODevice::WriteOnly)) {
      appendStream(streamID, file);
    } else {
      qWarning() << "Unable to open file" << file.errorString();
    }
  }
}

void ExportEventListCsv::ExporterImpl::appendStream(qulonglong streamID,
                                                    QIODevice& outputDevice) {
  QTextStream headerOutputStream(&outputDevice);

  Timestamp minimumTimestamp = 0;
  Timestamp maximumTimestamp = 0;
  QMap<QString, QDBusVariant> metadata;
  accessor.getStreamInfo(streamID, vx::emptyOptions(), minimumTimestamp,
                         maximumTimestamp, attributes, metadata);

  for (int attribID = 0; attribID < attributes.size(); ++attribID) {
    if (attribID != 0) {
      headerOutputStream << ",";
    }
    headerOutputStream << std::get<0>(attributes[attribID]);
  }
  headerOutputStream << "\n";

  headerOutputStream.flush();

  // Generate list of timestamp ranges for exporting
  QVector<Block> blocks;
  for (Timestamp ts = minimumTimestamp; ts <= maximumTimestamp;
       ts += TimestampBlockSize) {
    Block block;
    block.start = ts;
    block.end =
        std::min<Timestamp>(ts + TimestampBlockSize - 1, maximumTimestamp);
    block.exporter = this;
    block.streamID = streamID;
    blocks.append(block);
  }

  if (blocks.empty()) {
    return;
  }

  size_t blockCount = blocks.size();

  QFuture<QSharedPointer<QByteArray>> mappedBlocks = QtConcurrent::mapped(
      blocks, &ExportEventListCsv::ExporterImpl::handleBlockInThread);

  for (quint64 i = 0; i < blockCount; ++i) {
    auto mappedBlock = mappedBlocks.resultAt(i);
    outputDevice.write(*mappedBlock.data());
    mappedBlock->clear();

    op.updateProgress((streamID + float(i) / blockCount) / streamCount);
  }
}

QSharedPointer<QByteArray>
ExportEventListCsv::ExporterImpl::handleBlockInThread(Block block) {
  auto result = QSharedPointer<QByteArray>::create();
  QTextStream output(result.data());
  output.setCodec("UTF-8");

  const auto& attributes = block.exporter->attributes;

  if (!bufferStorage.hasLocalData() ||
      bufferStorage.localData()->getCapacity() < BlockSize ||
      bufferStorage.localData()->getAttributes() != attributes) {
    bufferStorage.setLocalData(
        EventListDataBuffer::create(BlockSize, attributes));
  }

  // TODO is this the correct baseService to use here?
  std::tuple<QString, QDBusObjectPath> bufferPath(
      QDBusConnection::sessionBus().baseService(),
      bufferStorage.localData()->getPath());

  std::vector<std::function<void(std::size_t)>> appenders(attributes.size());
  for (size_t attribID = 0; attribID < appenders.size(); ++attribID) {
    appenders[attribID] =
        TypeErasedArray1(bufferStorage.localData()->getAttributeDataReadOnly(
                             std::get<0>(attributes[attribID])))
            .specialize<void>([&output](auto& value) { output << value; });
  }

  qlonglong lastReadTimestamp = 0;
  QString versionString;

  // Process events in blocks
  while (true) {
    qulonglong readEventCount = block.exporter->accessor.readEvents(
        block.streamID, block.start, block.end, 0, BlockSize, bufferPath,
        vx::emptyOptions(), lastReadTimestamp, versionString);

    if (readEventCount == 0) {
      break;
    }

    // Increase minimum timestamp for next read
    block.start = std::max(block.start, lastReadTimestamp) + 1;

    int attributeCount = attributes.size();

    for (qulonglong i = 0; i < readEventCount; ++i) {
      for (int attribID = 0; attribID < attributeCount; ++attribID) {
        if (attribID != 0) {
          output << ",";
        }
        appenders[attribID](i);
      }
      output << "\n";
    }
  }

  return result;
}
