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

#pragma once

#include <stdio.h>
// stdio.h has to be included before jpeglib.h
#include <jpeglib.h>

#if BITS_IN_JSAMPLE != VX_LIBJPEG_BITS
#error BITS_IN_JSAMPLE != VX_LIBJPEG_BITS
#endif

namespace vx {
namespace libjpeg {

template <size_t bits>
struct LibJpegImpl {};

template <>
struct LibJpegImpl<BITS_IN_JSAMPLE> {
  using jsample = JSAMPLE;

  static JDIMENSION write_scanlines(j_compress_ptr cinfo, JSAMPARRAY scanlines,
                                    JDIMENSION num_lines) {
    return jpeg_write_scanlines(cinfo, scanlines, num_lines);
  }

  static JDIMENSION read_scanlines(j_decompress_ptr cinfo, JSAMPARRAY scanlines,
                                   JDIMENSION max_lines) {
    return jpeg_read_scanlines(cinfo, scanlines, max_lines);
  }
};

#if BITS_IN_JSAMPLE == 8
#define VX_LIBJPEG_HAVE_8BIT 1
#elif BITS_IN_JSAMPLE == 12
#define VX_LIBJPEG_HAVE_12BIT 1
#else
#error Unknown BITS_IN_JSAMPLE value
#endif

#ifdef MAXJ12SAMPLE
// Library has both 8-bit and 12-bit support
#define VX_LIBJPEG_HAVE_RUNTIME 1

#if BITS_IN_JSAMPLE != 8
#error MAXJ12SAMPLE defined but BITS_IN_JSAMPLE != 8
#endif

#define VX_LIBJPEG_HAVE_12BIT 1

template <>
struct LibJpegImpl<12> {
  using jsample = J12SAMPLE;

  static JDIMENSION write_scanlines(j_compress_ptr cinfo,
                                    J12SAMPARRAY scanlines,
                                    JDIMENSION num_lines) {
    return jpeg12_write_scanlines(cinfo, scanlines, num_lines);
  }

  static JDIMENSION read_scanlines(j_decompress_ptr cinfo,
                                   J12SAMPARRAY scanlines,
                                   JDIMENSION max_lines) {
    return jpeg12_read_scanlines(cinfo, scanlines, max_lines);
  }
};

#else
// libjpeg-turbo < 3
#define VX_LIBJPEG_HAVE_RUNTIME 0
#endif

#ifndef VX_LIBJPEG_HAVE_8BIT
#define VX_LIBJPEG_HAVE_8BIT 0
#endif

#ifndef VX_LIBJPEG_HAVE_12BIT
#define VX_LIBJPEG_HAVE_12BIT 0
#endif

#if !VX_LIBJPEG_HAVE_8BIT && !VX_LIBJPEG_HAVE_12BIT
#error !VX_LIBJPEG_HAVE_8BIT && !VX_LIBJPEG_HAVE_12BIT
#endif

#define CONCAT(a, b) a##b
#define CONCAT2(a, b) CONCAT(a, b)
#define IMPL_NAMESPACE CONCAT2(impl_, VX_LIBJPEG_BITS)
}  // namespace libjpeg
}  // namespace vx
