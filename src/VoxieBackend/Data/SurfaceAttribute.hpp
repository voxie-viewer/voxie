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

#include <VoxieBackend/Data/DataType.hpp>

#include <QtCore/QSharedPointer>

#include <QtDBus/QDBusVariant>

#include <cstdint>
#include <vector>

// TODO: Split into SurfaceAttribute and SurfaceAttributeTriangleIndexed

namespace vx {
class SurfaceDataTriangleIndexed;
class SharedMemory;

class VOXIEBACKEND_EXPORT SurfaceAttribute {
 public:
  SurfaceAttribute(
      SurfaceDataTriangleIndexed* surface,
      const std::tuple<QString, QString, quint64,
                       std::tuple<QString, quint32, QString>, QString,
                       QMap<QString, QDBusVariant>,
                       QMap<QString, QDBusVariant>>& attributeData);

  SurfaceAttribute(const SurfaceAttribute& other) = delete;
  SurfaceAttribute(SurfaceAttribute&& other) = delete;
  SurfaceAttribute& operator=(SurfaceAttribute&& other) = delete;

  uint64_t componentCount() const { return componentCount_; }
  uint64_t count() const { return count_; }

  uint64_t getSize() const;

  uint64_t getByteSize() const;

  void* getBytes() const;

  int getOpenGLType() const;

  int32_t getIntComponent(uint64_t index, uint64_t component) const;

  void setIntComponent(uint64_t index, uint64_t component, int32_t value);

  int32_t getInt(uint64_t index) const;

  void setInt(uint64_t index, int32_t value);

  float getFloatComponent(uint64_t index, uint64_t component) const;

  void setFloatComponent(uint64_t index, uint64_t component, float value);

  float getFloat(uint64_t index) const;

  void setFloat(uint64_t index, float value);

  std::tuple<QString, QString, quint64, std::tuple<QString, quint32, QString>,
             QString, QMap<QString, QDBusVariant>, QMap<QString, QDBusVariant>>
  dbusData();

  const QString& name() const { return name_; }
  const QString& kind() const { return kind_; }
  const QString& displayName() const { return displayName_; }

  const QMap<QString, QDBusVariant>& metadata() const { return metadata_; }
  const QMap<QString, QDBusVariant>& options() const { return options_; }

  QSharedPointer<SharedMemory> getSharedMemory();

  DataType dataType() const { return dataType_; }

  // Copy one entry from src[indexSrc] to this[indexDst]
  void copyEntryFrom(uint64_t indexDst,
                     const QSharedPointer<SurfaceAttribute>& src,
                     uint64_t indexSrc);

 private:
  QString name_;
  QString kind_;
  QString displayName_;

  QMap<QString, QDBusVariant> metadata_;
  QMap<QString, QDBusVariant> options_;

  uint64_t componentCount_;

  uint64_t count_;

  DataType dataType_;
  uint64_t elementSize;
  QSharedPointer<SharedMemory> dataSH;

  void checkType(DataType expectedType) const;

  uint64_t checkBounds(uint64_t index, uint64_t component) const;
};
}  // namespace vx
