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

#include "FileData.hpp"

#include <VoxieClient/ArrayInfo.hpp>
#include <VoxieClient/DBusAdaptors.hpp>

#include <VoxieBackend/Data/SharedMemory.hpp>

namespace vx {
namespace internal {
class FileDataAdaptorImpl : public FileDataAdaptor {
  [[gnu::unused]]  // Currently not used
  FileData* object;

 public:
  FileDataAdaptorImpl(FileData* object)
      : FileDataAdaptor(object), object(object) {}
  virtual ~FileDataAdaptorImpl() {}
};

class FileDataByteStreamAdaptorImpl : public FileDataByteStreamAdaptor {
  FileDataByteStream* object;

 public:
  FileDataByteStreamAdaptorImpl(FileDataByteStream* object)
      : FileDataByteStreamAdaptor(object), object(object) {}
  virtual ~FileDataByteStreamAdaptorImpl() {}

  QString mediaType() const override {
    try {
      return object->mediaType();
    } catch (vx::Exception& e) {
      return e.handle(object);
    }
  }

  std::tuple<QMap<QString, QDBusVariant>, qint64,
             std::tuple<QString, quint32, QString>, std::tuple<quint64>,
             std::tuple<qint64>, QMap<QString, QDBusVariant>>
  GetContentReadonly(const QMap<QString, QDBusVariant>& options) override {
    try {
      vx::ExportedObject::checkOptions(options);

      return this->getContent(false).toDBus();
    } catch (vx::Exception& e) {
      return e.handle(object);
    }
  }
  std::tuple<QMap<QString, QDBusVariant>, qint64,
             std::tuple<QString, quint32, QString>, std::tuple<quint64>,
             std::tuple<qint64>, QMap<QString, QDBusVariant>>
  GetContentWritable(const QDBusObjectPath& update,
                     const QMap<QString, QDBusVariant>& options) override {
    try {
      vx::ExportedObject::checkOptions(options);

      auto updateObj = vx::DataUpdate::lookup(update);
      updateObj->validateCanUpdate(object);

      return this->getContent(true).toDBus();
    } catch (vx::Exception& e) {
      return e.handle(object);
    }
  }

  vx::Array1Info getContent(bool writable) {
    vx::Array1Info info;

    object->data()->getHandle(writable, info.handle);
    info.offset = 0;

    info.dataType = "uint";
    info.dataTypeSize = 8;
    info.byteorder = "none";

    info.size = object->data()->getSizeBytes();
    info.stride = 1;

    return info;
  }
};
}  // namespace internal
}  // namespace vx

vx::FileData::FileData() { new vx::internal::FileDataAdaptorImpl(this); }
vx::FileData::~FileData() {}

vx::FileDataByteStream::FileDataByteStream(const QString& mediaType,
                                           std::size_t lengthBytes)
    : mediaType_(mediaType),
      data_(createQSharedPointer<SharedMemory>(lengthBytes)) {
  new vx::internal::FileDataByteStreamAdaptorImpl(this);
}
vx::FileDataByteStream::~FileDataByteStream() {}

QList<QString> vx::FileDataByteStream::supportedDBusInterfaces() {
  return {
      "de.uni_stuttgart.Voxie.FileDataByteStream",
      "de.uni_stuttgart.Voxie.FileData",
  };
}

std::size_t vx::FileDataByteStream::size() {
  return this->data()->getSizeBytes();
}

QList<QSharedPointer<vx::SharedMemory>>
vx::FileDataByteStream::getSharedMemorySections() {
  return {this->data()};
}
