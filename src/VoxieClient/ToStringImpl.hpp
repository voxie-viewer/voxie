/*
 * Copyright (c) 2014-2025 The Voxie Authors
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

namespace vx {
class InvalidToStringImpl {};

template <typename T, typename Enable = void>
class ToStringImpl : InvalidToStringImpl {};

// Provides a way to turn a value into a string which is useful for debugging.
//
// The type is assumed to be known from the context and does not have to be
// included in the returned string. (In some cases there might be other
// information which is assumed to be known from the context, e.g. toString()
// for DBusValue assumes the signature is known, which toString() for
// DBusVariant will include the signature.)
//
// The format of the returned string should not be considered part of the API.

// template <>
// struct ToStringImpl<SomeType> {
//   // Can return a 'const char *', a 'std::string' or a 'QString'
//   static const char* toString(const SomeType& value);
// };
}  // namespace vx
