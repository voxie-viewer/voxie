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

#pragma once

#include <VoxieBackend/VoxieBackend.hpp>

#include <VoxieBackend/Component/Component.hpp>
#include <VoxieBackend/Component/ComponentTypeInfo.hpp>

#include <VoxieClient/Optional.hpp>

#include <VoxieClient/ObjectExport/ExportedObject.hpp>

namespace vx {
enum class DataType;

class VOXIEBACKEND_EXPORT BufferTypeBase {
 public:
  BufferTypeBase();
  virtual ~BufferTypeBase();

  static QSharedPointer<BufferTypeBase> fromJson(const QJsonValue& json);

  virtual bool equals(BufferTypeBase* other) = 0;

  virtual QJsonValue toJson() = 0;
};
class VOXIEBACKEND_EXPORT BufferTypePrimitive : public BufferTypeBase {
  DataType dataType_;

 public:
  BufferTypePrimitive(DataType dataType);
  ~BufferTypePrimitive() override;

  DataType dataType() { return dataType_; }

  bool equals(BufferTypeBase* other) override;

  QJsonValue toJson() override;
};
class VOXIEBACKEND_EXPORT BufferTypeArray : public BufferTypeBase {
  QList<quint64> shape_;
  QList<qint64> stridesBytes_;
  QSharedPointer<BufferTypeBase> elementType_;

 public:
  BufferTypeArray(const QList<quint64>& shape,
                  const QList<qint64>& stridesBytes,
                  const QSharedPointer<BufferTypeBase>& elementType);
  ~BufferTypeArray() override;

  const QList<quint64>& shape() { return shape_; }
  const QList<qint64>& stridesBytes() { return stridesBytes_; }
  const QSharedPointer<BufferTypeBase>& elementType() { return elementType_; }
  size_t dim() { return shape_.count(); }

  bool equals(BufferTypeBase* other) override;

  QJsonValue toJson() override;
};
class VOXIEBACKEND_EXPORT BufferTypeStruct : public BufferTypeBase {
 public:
  class Member {
    vx::Optional<QString> name_;
    qint64 offset_;
    QSharedPointer<BufferTypeBase> type_;

   public:
    Member(const vx::Optional<QString>& name, qint64 offset,
           QSharedPointer<BufferTypeBase> type);
    ~Member();

    const vx::Optional<QString>& name() const { return name_; }
    qint64 offset() const { return offset_; }
    QSharedPointer<BufferTypeBase> type() const { return type_; }

    bool equals(const Member& other) const;

    QJsonValue toJson() const;
  };

 private:
  QList<Member> members_;

 public:
  BufferTypeStruct(const QList<Member>& members);
  ~BufferTypeStruct() override;

  const QList<Member>& members() { return members_; }

  bool equals(BufferTypeBase* other) override;

  QJsonValue toJson() override;
};

class VOXIEBACKEND_EXPORT BufferType : public Component {
  Q_OBJECT

  QSharedPointer<BufferTypeBase> type_;
  quint64 sizeBytes_;

 public:
  BufferType(const QString& name, const QSharedPointer<BufferTypeBase>& type,
             quint64 sizeBytes);
  ~BufferType() override;

  const QSharedPointer<BufferTypeBase>& type() { return type_; }
  quint64 sizeBytes() { return sizeBytes_; }

  QList<QString> supportedComponentDBusInterfaces() override;
};

template <>
struct ComponentTypeInfo<BufferType> {
  static const char* name() {
    return "de.uni_stuttgart.Voxie.ComponentType.BufferType";
  }
};

#define VX_BUFFER_TYPE_DECLARE  \
  static const char* getName(); \
  static QSharedPointer<vx::BufferType> bufferType();

struct VOXIEBACKEND_EXPORT ByteBuffer {
  using Type = std::uint8_t;

  VX_BUFFER_TYPE_DECLARE
};
}  // namespace vx
