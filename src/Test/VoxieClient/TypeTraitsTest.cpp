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

// Note: This is a compile-time test

#include <VoxieClient/TypeTraits.hpp>

#include <cstdint>

static_assert(vx::IsConvertibleWithoutNarrowing<float, double>::value,
              "Failed");
static_assert(vx::IsConvertibleWithoutNarrowing<float, float>::value, "Failed");
static_assert(!vx::IsConvertibleWithoutNarrowing<double, float>::value,
              "Failed");

static_assert(vx::IsConvertibleWithoutNarrowing<int32_t, int32_t>::value,
              "Failed");
static_assert(!vx::IsConvertibleWithoutNarrowing<uint32_t, int32_t>::value,
              "Failed");
static_assert(vx::IsConvertibleWithoutNarrowing<uint32_t, int64_t>::value,
              "Failed");
static_assert(!vx::IsConvertibleWithoutNarrowing<int64_t, uint32_t>::value,
              "Failed");
static_assert(!vx::IsConvertibleWithoutNarrowing<int64_t, int32_t>::value,
              "Failed");

static_assert(!vx::IsConvertibleWithoutNarrowing<int32_t, double>::value,
              "Failed");
static_assert(!vx::IsConvertibleWithoutNarrowing<double, int32_t>::value,
              "Failed");
