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

#include "SurfaceAttribute.hpp"

#include <QDebug>

#include <VoxieClient/Array.hpp>
#include <VoxieClient/Exception.hpp>

#include <VoxieBackend/Data/SharedMemory.hpp>
#include <VoxieBackend/Data/SurfaceData.hpp>

#include <QtGui/QOpenGLFunctions>

using namespace vx;

SurfaceAttribute::SurfaceAttribute(
    SurfaceDataTriangleIndexed* surface,
    const std::tuple<QString, QString, quint64,
                     std::tuple<QString, quint32, QString>, QString,
                     QMap<QString, QDBusVariant>, QMap<QString, QDBusVariant>>&
        attributeData)
    : name_(std::get<0>(attributeData)),
      kind_(std::get<1>(attributeData)),
      displayName_(std::get<4>(attributeData)),
      metadata_(std::get<5>(attributeData)),
      options_(),
      componentCount_(std::get<2>(attributeData)) {
  ExportedObject::checkOptions(std::get<6>(attributeData));

  if (this->kind_ == "de.uni_stuttgart.Voxie.SurfaceAttributeKind.Triangle")
    count_ = surface->triangles().size();
  else if (this->kind_ == "de.uni_stuttgart.Voxie.SurfaceAttributeKind.Vertex")
    count_ = surface->vertices().size();
  else
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Invalid surface attribute kind");

  this->dataType_ = parseDataTypeStruct(std::get<3>(attributeData));

  this->elementSize = getElementSizeBytes(this->dataType_);
  quint64 bytes = vx::checked_mul<uint64_t>(
      vx::checked_mul<uint64_t>(count_, componentCount_), elementSize);
  if (bytes != (size_t)bytes)
    throw vx::Exception("de.uni_stuttgart.Voxie.OutOfMemory",
                        "Overflow while calculating size");
  this->dataSH = createQSharedPointer<vx::SharedMemory>(bytes);
}

uint64_t SurfaceAttribute::getSize() const {
  return vx::checked_mul(this->count_, this->componentCount_);
}

uint64_t SurfaceAttribute::getByteSize() const {
  return vx::checked_mul(getSize(), this->elementSize);
}

void* SurfaceAttribute::getBytes() const { return dataSH->getData(); }

int SurfaceAttribute::getOpenGLType() const {
  switch (this->dataType()) {
    case DataType::Int8:
      return GL_BYTE;
    case DataType::UInt8:
      return GL_UNSIGNED_BYTE;
    case DataType::Int16:
      return GL_SHORT;
    case DataType::UInt16:
      return GL_UNSIGNED_SHORT;
    case DataType::Int32:
      return GL_INT;
    case DataType::UInt32:
      return GL_UNSIGNED_INT;
    case DataType::Float16:
      return GL_HALF_FLOAT;
    case DataType::Float32:
      return GL_FLOAT;
    case DataType::Float64:
      return GL_DOUBLE;
    default:
      throw vx::Exception(
          "de.uni_stuttgart.Voxie.Error",
          "SurfaceAttribute::getOpenGLType(): Unknown data type");
  }
}

int32_t SurfaceAttribute::getIntComponent(uint64_t index,
                                          uint64_t component) const {
  checkType(DataType::Int32);
  auto dataIndex = checkBounds(index, component);
  return ((const qint32*)getBytes())[dataIndex];
}

void SurfaceAttribute::setIntComponent(uint64_t index, uint64_t component,
                                       int32_t value) {
  checkType(DataType::Int32);
  auto dataIndex = checkBounds(index, component);
  ((qint32*)getBytes())[dataIndex] = value;
}

int32_t SurfaceAttribute::getInt(uint64_t index) const {
  return getIntComponent(index, 0);
}

void SurfaceAttribute::setInt(uint64_t index, int32_t value) {
  setIntComponent(index, 0, value);
}

float SurfaceAttribute::getFloatComponent(uint64_t index,
                                          uint64_t component) const {
  checkType(DataType::Float32);
  auto dataIndex = checkBounds(index, component);
  return ((const float*)getBytes())[dataIndex];
}

void SurfaceAttribute::setFloatComponent(uint64_t index, uint64_t component,
                                         float value) {
  checkType(DataType::Float32);
  auto dataIndex = checkBounds(index, component);
  ((float*)getBytes())[dataIndex] = value;
}

float SurfaceAttribute::getFloat(uint64_t index) const {
  return getFloatComponent(index, 0);
}

void SurfaceAttribute::setFloat(uint64_t index, float value) {
  setFloatComponent(index, 0, value);
}

void SurfaceAttribute::checkType(DataType expectedType) const {
  if (this->dataType() != expectedType)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "SurfaceAttribute type mismatch");
}

uint64_t SurfaceAttribute::checkBounds(uint64_t index,
                                       uint64_t component) const {
  if (component >= this->componentCount())
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "SurfaceAttribute componentCount out of bounds");

  auto arrayIndex = index * this->componentCount() + component;
  if (arrayIndex >= this->getSize())
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "SurfaceAttribute index out of bounds");

  return arrayIndex;
}

std::tuple<QString, QString, quint64, std::tuple<QString, quint32, QString>,
           QString, QMap<QString, QDBusVariant>, QMap<QString, QDBusVariant>>
SurfaceAttribute::dbusData() {
  auto type = getDataTypeStruct(this->dataType());
  return std::make_tuple(name(), kind(), componentCount(), type, displayName(),
                         metadata(), options());
}

QSharedPointer<SharedMemory> SurfaceAttribute::getSharedMemory() {
  if (!dataSH)
    throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                        "SurfaceAttribute::getSharedMemory(): !dataSH");
  return dataSH;
}

void SurfaceAttribute::copyEntryFrom(
    uint64_t indexDst, const QSharedPointer<SurfaceAttribute>& src,
    uint64_t indexSrc) {
  // TODO: Make this faster?
  for (uint64_t component = 0; component < this->componentCount();
       component++) {
    auto dataIndexDst = checkBounds(indexDst, component);
    auto dataIndexSrc = src->checkBounds(indexSrc, component);
    memcpy((char*)getBytes() + (dataIndexDst * this->elementSize),
           (const char*)src->getBytes() + (dataIndexSrc * this->elementSize),
           this->elementSize);
  }
}
