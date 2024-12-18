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

#include <VoxieBackend/Data/Data.hpp>
#include <VoxieBackend/Data/DataType.hpp>
#include <VoxieBackend/Data/SharedMemory.hpp>

#include <VoxieClient/Array.hpp>

#include <VoxieClient/Exception.hpp>

#include <VoxieClient/ObjectExport/ExportedObject.hpp>

namespace vx {

class VOXIEBACKEND_EXPORT EventListDataBuffer : public Data {
  Q_OBJECT
  VX_REFCOUNTEDOBJECT

 public:
  using AttributeInfo =
      std::tuple<QString, std::tuple<QString, quint32, QString>, QString,
                 QMap<QString, QDBusVariant>, QMap<QString, QDBusVariant>>;

  EventListDataBuffer();
  EventListDataBuffer(size_t capacity, QList<AttributeInfo> attributeInfoList);
  virtual ~EventListDataBuffer();

  QList<QString> supportedDBusInterfaces() override;

  size_t getCapacity() const;
  const QList<AttributeInfo>& getAttributes() const;

  vx::Array1Info getAttributeDataReadOnly(QString attributeName) const;
  vx::Array1Info getAttributeDataWritable(QString attributeName);

  QList<QSharedPointer<SharedMemory>> getSharedMemorySections() override;

 private:
  struct AttributeData {
    DataType type;
    QSharedPointer<SharedMemory> data;
  };

  vx::Array1Info getAttributeDataImpl(QString attributeName,
                                      bool writable) const;

  size_t capacity;
  QList<AttributeInfo> attributeInfoList;
  QMap<QString, AttributeData> attributes;
};

}  // namespace vx
