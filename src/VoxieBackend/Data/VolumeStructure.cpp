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

#include "VolumeStructure.hpp"

#include <VoxieClient/DBusAdaptors.hpp>

using namespace vx;

namespace vx {
namespace internal {
class VolumeStructureAdaptorImpl : public VolumeStructureAdaptor {
  Q_OBJECT

  VolumeStructure* object;

 public:
  VolumeStructureAdaptorImpl(VolumeStructure* object)
      : VolumeStructureAdaptor(object), object(object) {}
  virtual ~VolumeStructureAdaptorImpl() {}

  QString volumeStructureType() const override {
    return object->volumeStructureType();
  }
};
}  // namespace internal
}  // namespace vx

using namespace vx::internal;

VolumeStructure::VolumeStructure() : DynamicObject("VolumeStructure") {
  new VolumeStructureAdaptorImpl(this);
}

VolumeStructure::~VolumeStructure() {}

#include "VolumeStructure.moc"
