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
#include <VoxieClient/JsonDBus.hpp>
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

#include <fstream>

Q_NORETURN static void error(const QString& str) {
  throw vx::Exception("de.uni_stuttgart.Voxie.ExtPly.Error", str);
}

QCommandLineOption voxieImportFilename("voxie-import-filename", "File to load.",
                                       "filename");

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
    error("TODO");
    /*
    HANDLEDBUSPENDINGREPLY(op.opGen().SetProgress(1.0, vx::emptyOptions()));

    HANDLEDBUSPENDINGREPLY(op.op().Finish(QDBusObjectPath(srf2.path()),
                            QDBusObjectPath(srf2_version->path()),
                            vx::emptyOptions()));
    */
  });
}

struct AttributeData {
  static const size_t bufferSize = 1024 * 1024;

  size_t offset;
  size_t size;
  vx::Array1Info info;
  QSharedPointer<de::uni_stuttgart::Voxie::EventListDataBuffer> buffer;
  QSharedPointer<vx::MappedBuffer> mappedBuffer;
  size_t count;
  ptrdiff_t stride;
  uint8_t* ptr;

  AttributeData(
      size_t offset, size_t size,
      const QSharedPointer<de::uni_stuttgart::Voxie::EventListDataBuffer>&
          buffer,
      const QString& name)
      : offset(offset), size(size) {
    this->buffer = buffer;

    info = HANDLEDBUSPENDINGREPLY(
        buffer->GetAttributeReadonly(name, vx::emptyOptions()));

    count = vx::checked_cast<size_t>(info.size);
    stride = vx::checked_cast<ptrdiff_t>(info.stride);

    // qDebug() << name << size << info.dataTypeSize;
    if (size * 8 != info.dataTypeSize) error("size * 8 != info.dataTypeSize");

    auto bytes = vx::checked_cast<int64_t>(info.offset);
    auto pos0 = vx::checked_cast<int64_t>(info.offset);
    bytes = vx::checked_add<int64_t>(bytes, vx::checked_cast<int64_t>(size));

    if (count == 0) {
      bytes = 0;
      pos0 = 0;
    } else {
      auto sizeDim = vx::checked_mul<int64_t>(
          (vx::checked_cast<int64_t>(count - 1)), stride);
      if (stride > 0)
        bytes = vx::checked_add<int64_t>(bytes, sizeDim);
      else
        pos0 = vx::checked_add<int64_t>(pos0, sizeDim);
    }

    ptrdiff_t ioffset = vx::checked_cast<ptrdiff_t>(info.offset);
    if (ioffset < 0)
      throw vx::Exception("de.uni_stuttgart.Voxie.Error", "ioffset < 0");
    if (pos0 < 0)
      throw vx::Exception("de.uni_stuttgart.Voxie.Error", "pos0 < 0");

    mappedBuffer =
        createQSharedPointer<vx::MappedBuffer>(info.handle, bytes, false);

    ptr = (uint8_t*)mappedBuffer->data() + ioffset;
  }

  void copyTo(uint8_t* output, size_t pos) const {
    if (pos >= count) error("pos >= info.size");
    uint8_t* src = ptr + (stride * (ptrdiff_t)pos);
    uint8_t* dst = output + offset;
    memcpy(dst, src, size);
  }
};

static void exportFile(const QCommandLineParser& parser) {
  if (!parser.isSet(vx::ClaimedOperationBase::voxieOperationOption()))
    error("--voxie-operation is not set");
  QString operationPath =
      parser.value(vx::ClaimedOperationBase::voxieOperationOption());

  vx::initDBusTypes();

  vx::DBusClient dbusClient(parser);

  vx::ClaimedOperation<de::uni_stuttgart::Voxie::ExternalOperationExport> op(
      dbusClient, QDBusObjectPath(operationPath));

  op.forwardExc([&]() {
    QString filename = op.op().filename();
    auto inputDataPath = op.op().data();

    auto eventList = makeSharedQObject<
        de::uni_stuttgart::Voxie::EventListDataAccessorOperations>(
        dbusClient.uniqueName(), inputDataPath.path(), dbusClient.connection());

    auto streamCount = eventList->streamCount();
    auto metadata =
        HANDLEDBUSPENDINGREPLY(eventList->GetMetadata(vx::emptyOptions()));
    QJsonArray streams;
    for (quint64 streamID = 0; streamID < streamCount; streamID++) {
      quint64 eventCount;
      qint64 minimumTimestamp;
      qint64 maximumTimestamp;
      QList<
          std::tuple<QString, std::tuple<QString, quint32, QString>, QString,
                     QMap<QString, QDBusVariant>, QMap<QString, QDBusVariant>>>
          attributes;
      QMap<QString, QDBusVariant> perStreamMetadata;
      std::tie(eventCount, minimumTimestamp, maximumTimestamp, attributes,
               perStreamMetadata) =
          HANDLEDBUSPENDINGREPLY(
              eventList->GetStreamInfo(streamID, vx::emptyOptions()));
      // qDebug() << i << eventCount << minimumTimestamp << maximumTimestamp;

      // TODO: Use a pool for buffers? (See e.g.
      // src/ExtFilterEventListClustering/EventListBufferPool.hpp)
      // TODO: Use multithreading for reading events
      auto path = HANDLEDBUSPENDINGREPLY(dbusClient->CreateEventListDataBuffer(
          dbusClient.clientPath(), AttributeData::bufferSize, attributes,
          vx::emptyOptions()));
      auto buffer =
          makeSharedQObject<de::uni_stuttgart::Voxie::EventListDataBuffer>(
              dbusClient.uniqueName(), path.path(), dbusClient.connection());

      quint64 offset = 0;
      QJsonArray attributesJson;
      QList<AttributeData> datas;
      for (const auto& attribute : attributes) {
        // Throw an error if there is an unknown option (currently no options
        // are known)
        for (const auto& optionName : std::get<4>(attribute).keys())
          error("Unknown event list stream option: '" + optionName + "'");

        QString name = std::get<0>(attribute);

        auto typeSizeBits = std::get<1>(std::get<1>(attribute));
        if (typeSizeBits % 8 != 0) error("typeSizeBits % 8 != 0");
        auto typeSizeBytes = typeSizeBits / 8;

        attributesJson.append(QJsonObject{
            {"Name", name},
            {"Type", QJsonArray{std::get<0>(std::get<1>(attribute)),
                                (qint64)typeSizeBits,
                                std::get<2>(std::get<1>(attribute))}},
            {"DisplayName", std::get<2>(attribute)},
            {"Metadata", vx::dbusToJson(std::get<3>(attribute))},
            {"Offset", (qint64)offset},
        });
        datas.append(AttributeData(offset, typeSizeBytes, buffer, name));

        offset += typeSizeBytes;
      }
      auto recordSize = offset;

      // qDebug() << attributesJson;

      std::vector<uint8_t> outputBuffer(recordSize * AttributeData::bufferSize);
      // memset(outputBuffer.data(), 255, recordSize *
      // AttributeData::bufferSize);

      QFileInfo fileInfo(filename);
      QString streamFileName =
          fileInfo.dir().filePath(fileInfo.completeBaseName() + ".stream-" +
                                  QString::number(streamID) + ".dat");

      QFile file(streamFileName);
      if (!file.open(QIODevice::WriteOnly)) {
        error("Unable to open file " + streamFileName + ": " +
              file.errorString());
      }

      qint64 pos = minimumTimestamp;
      quint64 overallCount = 0;
      for (;;) {
        quint64 readEventCount;
        qint64 lastReadTimestamp;
        QString versionString;

        // TODO: Do this in a better way. Use a separate accessor for
        // long-running operations?
        eventList->setTimeout(INT_MAX);

        std::tie(readEventCount, lastReadTimestamp, versionString) =
            HANDLEDBUSPENDINGREPLY(eventList->ReadEvents(
                streamID, pos, maximumTimestamp, 0, AttributeData::bufferSize,
                std::make_tuple(dbusClient.uniqueName(),
                                QDBusObjectPath(buffer->path())),
                vx::emptyOptions()));
        if (readEventCount == 0 ||
            lastReadTimestamp == std::numeric_limits<qint64>::max())
          break;
        pos = lastReadTimestamp + 1;
        overallCount += readEventCount;

        // TODO: Sort values by timestamp, probably put timestamp always at
        // offset 0, ...
        for (size_t j = 0; j < readEventCount; j++) {
          auto outputPos = outputBuffer.data() + j * recordSize;
          for (const auto& data : datas) {
            data.copyTo(outputPos, j);
          }
        }

        file.write((char*)outputBuffer.data(), readEventCount * recordSize);

        HANDLEDBUSPENDINGREPLY(op.opGen().SetProgress(
            (streamID + (pos - minimumTimestamp) /
                            (1.0 * maximumTimestamp + 1 - minimumTimestamp)) /
                streamCount,
            vx::emptyOptions()));
      }

      HANDLEDBUSPENDINGREPLY(op.opGen().SetProgress(
          1.0 * (streamID + 1) / streamCount, vx::emptyOptions()));

      // TODO: Add sha512sum?

      streams.append(QJsonObject{
          {"MinimumTimestamp", minimumTimestamp},
          {"MaximumTimestamp", maximumTimestamp},
          {"Attributes", attributesJson},
          {"EventCount", (qint64)overallCount},
          {"RecordSize", (qint64)recordSize},
      });
    }

    QJsonObject json{
        {"Type", "de.uni_stuttgart.Voxie.FileFormat.EventList.Json"},
        {"Metadata", vx::dbusToJson(metadata)},
        {"Streams", streams},
    };

    QFile file(filename);
    if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
      error("Error opening file " + file.fileName() + ": " +
            file.errorString());
    }
    file.write(QJsonDocument(json).toJson());

    HANDLEDBUSPENDINGREPLY(op.op().Finish(vx::emptyOptions()));
  });
}

int main(int argc, char* argv[]) {
  try {
    if (argc < 1) error("argc is smaller than 1");

    QCommandLineParser parser;
    parser.setApplicationDescription(
        "Voxie extension for loading .vxevl files");
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
      import(parser);
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
