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

#include "VolumeStructureVoxel.hpp"

#include <VoxieClient/DBusAdaptors.hpp>

using namespace vx;

namespace vx {
namespace internal {
class VolumeStructureVoxelAdaptorImpl : public VolumeStructureVoxelAdaptor {
  Q_OBJECT

  VolumeStructureVoxel* object;

 public:
  VolumeStructureVoxelAdaptorImpl(VolumeStructureVoxel* object)
      : VolumeStructureVoxelAdaptor(object), object(object) {}
  virtual ~VolumeStructureVoxelAdaptorImpl() {}

  vx::TupleVector<quint64, 3> arrayShape() const override {
    return object->arrayShape().toTupleVector();
  }
};
}  // namespace internal
}  // namespace vx

using namespace vx::internal;

VolumeStructureVoxel::VolumeStructureVoxel(const VectorSizeT3& arrayShape)
    : VolumeStructure(), arrayShape_(arrayShape) {
  new VolumeStructureVoxelAdaptorImpl(this);
}

VolumeStructureVoxel::~VolumeStructureVoxel() {}

QList<QString> VolumeStructureVoxel::supportedDBusInterfaces() {
  return {
      "de.uni_stuttgart.Voxie.VolumeStructureVoxel",
      //"de.uni_stuttgart.Voxie.VolumeStructure",
  };
}

#include "VolumeStructureVoxel.moc"
