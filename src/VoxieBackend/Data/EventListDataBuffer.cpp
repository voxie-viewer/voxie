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

// QDBusConnection should be included as early as possible:
// https://bugreports.qt.io/browse/QTBUG-48351 /
// https://bugreports.qt.io/browse/QTBUG-48377
#include <QtDBus/QDBusConnection>

#include "EventListDataBuffer.hpp"

#include <VoxieClient/DBusAdaptors.hpp>

#include <QtCore/QCoreApplication>

using namespace vx;

class EventListDataBufferAdaptorImpl : public EventListDataBufferAdaptor {
 public:
  EventListDataBufferAdaptorImpl(EventListDataBuffer* buffer)
      : EventListDataBufferAdaptor(buffer), buffer(buffer) {}

  virtual QList<
      std::tuple<QString, std::tuple<QString, quint32, QString>, QString,
                 QMap<QString, QDBusVariant>, QMap<QString, QDBusVariant>>>
  attributes() const override {
    return buffer->getAttributes();
  }

  virtual qulonglong capacity() const override { return buffer->getCapacity(); }

  virtual std::tuple<QMap<QString, QDBusVariant>, qint64,
                     std::tuple<QString, quint32, QString>, std::tuple<quint64>,
                     std::tuple<qint64>, QMap<QString, QDBusVariant>>
  GetAttributeReadonly(const QString& name,
                       const QMap<QString, QDBusVariant>& options) override {
    Q_UNUSED(options);
    return buffer->getAttributeDataReadOnly(name).toDBus();
  }

  virtual std::tuple<QMap<QString, QDBusVariant>, qint64,
                     std::tuple<QString, quint32, QString>, std::tuple<quint64>,
                     std::tuple<qint64>, QMap<QString, QDBusVariant>>
  GetAttributeWritable(const QDBusObjectPath& update, const QString& name,
                       const QMap<QString, QDBusVariant>& options) override {
    Q_UNUSED(update);
    Q_UNUSED(options);
    return buffer->getAttributeDataWritable(name).toDBus();
  }

 private:
  EventListDataBuffer* buffer;
};

EventListDataBuffer::EventListDataBuffer() {
  new EventListDataBufferAdaptorImpl(this);
}

EventListDataBuffer::EventListDataBuffer(size_t capacity,
                                         QList<AttributeInfo> attributeInfoList)
    : capacity(capacity), attributeInfoList(attributeInfoList) {
  new EventListDataBufferAdaptorImpl(this);
  for (auto& attributeInfo : attributeInfoList) {
    AttributeData attribute;
    attribute.type = parseDataTypeStruct(std::get<1>(attributeInfo));

    QString dataType;
    quint32 dataTypeSize = 0;
    QString byteOrder;

    getDataTypeInfo(attribute.type, dataType, dataTypeSize, byteOrder);

    attribute.data =
        createQSharedPointer<SharedMemory>(dataTypeSize * capacity);

    QString attributeName = std::get<0>(attributeInfo);
    attributes.insert(attributeName, attribute);
  }
}

EventListDataBuffer::~EventListDataBuffer() {}

QList<QString> EventListDataBuffer::supportedDBusInterfaces() {
  return {
      "de.uni_stuttgart.Voxie.EventListData",
  };
}

size_t EventListDataBuffer::getCapacity() const { return capacity; }

const QList<EventListDataBuffer::AttributeInfo>&
EventListDataBuffer::getAttributes() const {
  return attributeInfoList;
}

vx::Array1Info EventListDataBuffer::getAttributeDataReadOnly(
    QString attributeName) const {
  return getAttributeDataImpl(attributeName, false);
}

vx::Array1Info EventListDataBuffer::getAttributeDataWritable(
    QString attributeName) {
  return getAttributeDataImpl(attributeName, true);
}

vx::Array1Info EventListDataBuffer::getAttributeDataImpl(QString attributeName,
                                                         bool writable) const {
  if (!attributes.contains(attributeName)) {
    throw Exception(
        "de.uni_stuttgart.Voxie.InvalidOperation",
        QStringLiteral("EventListDataBuffer does not contain attribute '%1'")
            .arg(attributeName));
  }

  auto& attribute = attributes.value(attributeName);

  Array1Info info;

  attribute.data->getHandle(writable, info.handle);
  info.offset = 0;

  getDataTypeInfo(attribute.type, info.dataType, info.dataTypeSize,
                  info.byteorder);

  info.size = capacity;
  info.stride = info.dataTypeSize;

  return info;
}

QList<QSharedPointer<SharedMemory>>
EventListDataBuffer::getSharedMemorySections() {
  QList<QSharedPointer<SharedMemory>> sections;

  for (const auto& entry : attributes) {
    sections << entry.data;
  }

  return sections;
}
