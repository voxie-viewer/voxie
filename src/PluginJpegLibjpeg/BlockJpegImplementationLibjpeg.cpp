/*
 * Copyright (c) 2014-2024 The Voxie Authors
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

#include "BlockJpegImplementationLibjpeg.hpp"

#include <PluginJpegLibjpeg/BlockDecoderImpl.hpp>
#include <PluginJpegLibjpeg/BlockEncoderImpl.hpp>

namespace vx {
namespace libjpeg {
// TODO: Put libjpeg implementations into plugins instead of including this
// statically here

BlockJpegImplementationLibjpeg::BlockJpegImplementationLibjpeg()
    : BlockJpegImplementation(
          "de.uni_stuttgart.Voxie.BlockJpegImplementation.Libjpeg") {}
BlockJpegImplementationLibjpeg::~BlockJpegImplementationLibjpeg() {}

bool BlockJpegImplementationLibjpeg::hasDecoder() { return true; }
bool BlockJpegImplementationLibjpeg::supportsParametersDecoder(
    const ParametersRef& parRef) {
#if VX_LIBJPEG_HAVE_8BIT
  if (parRef.samplePrecision == 8) return true;
#endif
#if VX_LIBJPEG_HAVE_12BIT
  if (parRef.samplePrecision == 12) return true;
#endif
  return false;
}
std::shared_ptr<BlockDecoder> BlockJpegImplementationLibjpeg::createDecoder(
    const ParametersRef& parRef) {
  return IMPL_NAMESPACE::createBlockDecoder(parRef);
}

bool BlockJpegImplementationLibjpeg::hasEncoder() { return true; }
bool BlockJpegImplementationLibjpeg::supportsParametersEncoder(
    const ParametersRef& parRef) {
#if VX_LIBJPEG_HAVE_8BIT
  if (parRef.samplePrecision == 8) return true;
#endif
#if VX_LIBJPEG_HAVE_12BIT
  if (parRef.samplePrecision == 12) return true;
#endif
  return false;
}
std::shared_ptr<BlockEncoder> BlockJpegImplementationLibjpeg::createEncoder(
    const ParametersRef& parRef) {
  return IMPL_NAMESPACE::createBlockEncoder(parRef);
}

int BlockJpegImplementationLibjpeg::priority() {
#if VX_LIBJPEG_BITS == 12
  return 8;
#else
  return 10;
#endif
}
}  // namespace libjpeg
}  // namespace vx
