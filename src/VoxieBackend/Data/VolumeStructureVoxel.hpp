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

#include <VoxieBackend/Data/VolumeStructure.hpp>

#include <VoxieBackend/DBus/DBusTypes.hpp>

#include <VoxieClient/DBusTypeList.hpp>

namespace vx {
class VOXIEBACKEND_EXPORT VolumeStructureVoxel : public vx::VolumeStructure {
  Q_OBJECT
  REFCOUNTEDOBJ_DECL(VolumeStructureVoxel)

  VectorSizeT3 arrayShape_;

 public:
  // throws Exception
  VolumeStructureVoxel(const VectorSizeT3& arrayShape);

  ~VolumeStructureVoxel();

  const VectorSizeT3& arrayShape() const { return arrayShape_; }

  QList<QString> supportedDBusInterfaces() override;

  QString volumeStructureType() override;
};

}  // namespace vx