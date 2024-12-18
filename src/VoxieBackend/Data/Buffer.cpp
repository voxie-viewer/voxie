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

#include "Buffer.hpp"

#include <VoxieBackend/DebugOptions.hpp>

#include <VoxieBackend/Data/BufferType.hpp>
#include <VoxieBackend/Data/SharedMemory.hpp>

#include <VoxieClient/DBusAdaptors.hpp>
#include <VoxieClient/JsonDBus.hpp>

// For checked_mul
#include <VoxieClient/Array.hpp>

namespace vx {
namespace internal {
class BufferAdaptorImpl : public BufferAdaptor {
  Buffer* object;

 public:
  BufferAdaptorImpl(Buffer* object) : BufferAdaptor(object), object(object) {}
  virtual ~BufferAdaptorImpl() {}

  qint64 offsetBytes() const override {
    try {
      // Currently always 0
      return 0;
    } catch (vx::Exception& e) {
      qWarning() << "Got error during DBus property:" << e.what();
      return e.handle(object);
    }
  }

  quint64 sizeBytes() const override {
    try {
      return object->data()->getSizeBytes();
    } catch (vx::Exception& e) {
      qWarning() << "Got error during DBus property:" << e.what();
      return e.handle(object);
    }
  }

  QDBusVariant type() const override {
    try {
      return jsonToDBus(object->type()->toJson());
    } catch (vx::Exception& e) {
      qWarning() << "Got error during DBus property:" << e.what();
      return e.handle(object);
    }
  }

  std::tuple<QMap<QString, QDBusVariant>, qint64, QDBusVariant,
             QMap<QString, QDBusVariant>>
  GetDataReadonly(const QMap<QString, QDBusVariant>& options) override {
    try {
      vx::ExportedObject::checkOptions(options);

      QMap<QString, QDBusVariant> handle;
      object->data()->getHandle(false, handle);

      return std::make_tuple(handle, 0, jsonToDBus(object->type()->toJson()),
                             QMap<QString, QDBusVariant>());
    } catch (vx::Exception& e) {
      return e.handle(object);
    }
  }

  std::tuple<QMap<QString, QDBusVariant>, qint64, QDBusVariant,
             QMap<QString, QDBusVariant>>
  GetDataWritable(const QDBusObjectPath& update,
                  const QMap<QString, QDBusVariant>& options) override {
    try {
      vx::ExportedObject::checkOptions(options);

      if (update != QDBusObjectPath("/"))
        throw vx::Exception(
            "de.uni_stuttgart.Voxie.InvalidOperation",
            "Buffer.GetDataWritable() called with update object");

      QMap<QString, QDBusVariant> handle;
      object->data()->getHandle(true, handle);

      return std::make_tuple(handle, 0, jsonToDBus(object->type()->toJson()),
                             QMap<QString, QDBusVariant>());
    } catch (vx::Exception& e) {
      return e.handle(object);
    }
  }
};
}  // namespace internal
}  // namespace vx

vx::Buffer::Buffer(const QSharedPointer<BufferTypeBase>& elementType,
                   quint64 count, qint64 strideBytes)
    : RefCountedObject("Buffer"),
      elementType_(elementType),
      count_(count),
      strideBytes_(strideBytes) {
  new vx::internal::BufferAdaptorImpl(this);

  if (strideBytes < 0)
    throw vx::Exception("de.uni_stuttgart.Voxie.NotImplemented",
                        "Got negative array stride");

  // TODO: Should this really use the stride or should it calculate the size
  // from elementType()? What will happen to padding in the latter case?
  data_ = createQSharedPointer<SharedMemory>(
      checked_mul((quint64)strideBytes, count));
}
vx::Buffer::~Buffer() {}

QSharedPointer<vx::BufferTypeBase> vx::Buffer::type() {
  return createQSharedPointer<BufferTypeArray>(
      QList<quint64>{count()}, QList<qint64>{strideBytes()}, elementType());
}

void vx::Buffer::checkTypeAndStrides(
    const QSharedPointer<BufferType>& expectedType) {
  if (!expectedType->type()->equals(this->elementType().data())) {
    if (vx::debug_option::Log_BufferType()->get()) {
      qDebug() << "Expected type" << expectedType->type()->toJson();
      qDebug() << "Actual type" << this->elementType()->toJson();
    }

    throw vx::Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                        "Got non-matching buffer type");
  }

  if (expectedType->sizeBytes() != (quint64)this->strideBytes())
    throw vx::Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                        "Got unexpected stride");
}
