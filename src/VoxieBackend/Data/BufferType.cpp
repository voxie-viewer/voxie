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

#include "BufferType.hpp"

#include <VoxieBackend/Data/BufferTypeInst.hpp>
#include <VoxieBackend/Data/DataType.hpp>

#include <VoxieClient/DBusAdaptors.hpp>
#include <VoxieClient/JsonDBus.hpp>
#include <VoxieClient/JsonUtil.hpp>

#include <QtCore/QJsonArray>

vx::BufferTypeBase::BufferTypeBase() {}
vx::BufferTypeBase::~BufferTypeBase() {}

QSharedPointer<vx::BufferTypeBase> vx::BufferTypeBase::fromJson(
    const QJsonValue& json) {
  auto ar = expectArray(json);
  if (ar.size() == 0)
    throw vx::Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                        "Got invalid buffer type: Got zero-length array");
  QString kind = expectString(ar[0]);
  if (kind == "primitive") {
    if (ar.size() != 4)
      throw vx::Exception(
          "de.uni_stuttgart.Voxie.InvalidArgument",
          "Got invalid buffer type: Got unexpected array size for primitive");
    auto bits = expectUnsignedInt(ar[2]);
    if (bits > std::numeric_limits<quint32>::max())
      throw vx::Exception(
          "de.uni_stuttgart.Voxie.InvalidArgument",
          "Got invalid buffer type: Got out-of-range bit count");
    auto dt = parseDataTypeStruct(
        std::make_tuple(expectString(ar[1]), bits, expectString(ar[3])));
    return createQSharedPointer<BufferTypePrimitive>(dt);
  } else if (kind == "array") {
    if (ar.size() != 4)
      throw vx::Exception(
          "de.uni_stuttgart.Voxie.InvalidArgument",
          "Got invalid buffer type: Got unexpected array size for array");

    auto shapeAr = expectArray(ar[1]);
    size_t dim = shapeAr.size();
    QList<quint64> shape;
    for (const auto& val : shapeAr) shape << expectUnsignedInt(val);

    auto stridesAr = expectArray(ar[2]);
    if ((size_t)stridesAr.size() != dim)
      throw vx::Exception(
          "de.uni_stuttgart.Voxie.InvalidArgument",
          "Got invalid buffer type: Got dimension mismatch for array");
    QList<qint64> strides;
    for (const auto& val : stridesAr) strides << expectSignedInt(val);

    return createQSharedPointer<BufferTypeArray>(shape, strides,
                                                 fromJson(ar[3]));
  } else if (kind == "struct") {
    if (ar.size() != 2)
      throw vx::Exception(
          "de.uni_stuttgart.Voxie.InvalidArgument",
          "Got invalid buffer type: Got unexpected array size for struct");

    QList<BufferTypeStruct::Member> members;
    for (const auto& memberJson : expectArray(ar[1])) {
      auto memberArray = expectArray(memberJson);
      if (memberArray.size() != 3)
        throw vx::Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                            "Got invalid buffer type: Got unexpected array "
                            "size for struct member");

      vx::Optional<QString> name;
      if (memberArray[0].type() != QJsonValue::Null)
        name = expectString(memberArray[0]);

      members << BufferTypeStruct::Member(name, expectSignedInt(memberArray[1]),
                                          fromJson(memberArray[2]));
    }

    return createQSharedPointer<BufferTypeStruct>(members);
  } else {
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.InvalidArgument",
        "Got invalid buffer type: Got unknown type kind: " + kind);
  }
}

vx::BufferTypePrimitive::BufferTypePrimitive(DataType dataType)
    : dataType_(dataType) {}
vx::BufferTypePrimitive::~BufferTypePrimitive() {}

bool vx::BufferTypePrimitive::equals(BufferTypeBase* other) {
  auto otherCast = dynamic_cast<BufferTypePrimitive*>(other);
  if (!otherCast) return false;
  return this->dataType() == otherCast->dataType();
}

QJsonValue vx::BufferTypePrimitive::toJson() {
  auto strct = getDataTypeStruct(this->dataType());
  return QJsonArray{
      "primitive",
      std::get<0>(strct),
      (qint64)std::get<1>(strct),
      std::get<2>(strct),
  };
}

vx::BufferTypeArray::BufferTypeArray(
    const QList<quint64>& shape, const QList<qint64>& stridesBytes,
    const QSharedPointer<BufferTypeBase>& elementType)
    : shape_(shape), stridesBytes_(stridesBytes), elementType_(elementType) {
  if (shape.count() != stridesBytes.count())
    throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                        "BufferTypeArray: shape and stride dimension mismatch");
}
vx::BufferTypeArray::~BufferTypeArray() {}

bool vx::BufferTypeArray::equals(BufferTypeBase* other) {
  auto otherCast = dynamic_cast<BufferTypeArray*>(other);
  if (!otherCast) return false;
  if (this->dim() != otherCast->dim()) return false;
  for (size_t i = 0; i < this->dim(); i++) {
    if (this->shape()[i] != otherCast->shape()[i]) return false;
    if (this->stridesBytes()[i] != otherCast->stridesBytes()[i]) return false;
  }
  return this->elementType()->equals(otherCast->elementType().data());
}

QJsonValue vx::BufferTypeArray::toJson() {
  QJsonArray shapeJson;
  for (const auto& entry : this->shape()) shapeJson << (qint64)entry;

  QJsonArray stridesBytesJson;
  for (const auto& entry : this->stridesBytes()) stridesBytesJson << entry;

  return QJsonArray{
      "array",
      shapeJson,
      stridesBytesJson,
      this->elementType()->toJson(),
  };
}

vx::BufferTypeStruct::Member::Member(const vx::Optional<QString>& name,
                                     qint64 offset,
                                     QSharedPointer<BufferTypeBase> type)
    : name_(name), offset_(offset), type_(type) {}
vx::BufferTypeStruct::Member::~Member() {}

bool vx::BufferTypeStruct::Member::equals(const Member& other) const {
  if (this->offset() != other.offset()) return false;
  return this->type()->equals(other.type().data());
}

QJsonValue vx::BufferTypeStruct::Member::toJson() const {
  return QJsonArray{
      name().has_value() ? QJsonValue(name().value()) : QJsonValue(),
      offset(),
      type()->toJson(),
  };
}

vx::BufferTypeStruct::BufferTypeStruct(const QList<Member>& members)
    : members_(members) {}
vx::BufferTypeStruct::~BufferTypeStruct() {}

bool vx::BufferTypeStruct::equals(BufferTypeBase* other) {
  auto otherCast = dynamic_cast<BufferTypeStruct*>(other);
  if (!otherCast) return false;
  if (this->members().count() != otherCast->members().count()) return false;
  for (int i = 0; i < this->members().count(); i++)
    if (!this->members()[i].equals(otherCast->members()[i])) return false;
  return true;
}

QJsonValue vx::BufferTypeStruct::toJson() {
  QJsonArray membersJson;
  for (const auto& entry : this->members()) membersJson << entry.toJson();

  return QJsonArray{
      "struct",
      membersJson,
  };
}

namespace vx {
namespace internal {
class BufferTypeAdaptorImpl : public BufferTypeAdaptor {
  BufferType* object;

 public:
  BufferTypeAdaptorImpl(BufferType* object)
      : BufferTypeAdaptor(object), object(object) {}
  virtual ~BufferTypeAdaptorImpl() {}

  quint64 sizeBytes() const override {
    try {
      return object->sizeBytes();
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
};
}  // namespace internal
}  // namespace vx

vx::BufferType::BufferType(const QString& name,
                           const QSharedPointer<BufferTypeBase>& type,
                           quint64 sizeBytes)
    : Component(ComponentTypeInfo<BufferType>::name(), name, {}),
      type_(type),
      sizeBytes_(sizeBytes) {
  new vx::internal::BufferTypeAdaptorImpl(this);
}
vx::BufferType::~BufferType() {}

QList<QString> vx::BufferType::supportedComponentDBusInterfaces() {
  return {
      "de.uni_stuttgart.Voxie.BufferType",
  };
}

// Note: The name of the buffer type should not be used anywhere, instead the
// actual type should be hardcoded
VX_BUFFER_TYPE_DEFINE(vx::ByteBuffer,
                      "de.uni_stuttgart.Voxie.Internal.ByteBuffer")
