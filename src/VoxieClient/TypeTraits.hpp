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

#include <utility>

namespace vx {
// Based on
// https://stackoverflow.com/questions/36270158/avoiding-narrowing-conversions-with-c-type-traits/67603594#67603594
namespace intern {
template <class...>
using Void_t = void;
template <typename From, typename To, typename = void>
struct IsConvertibleWithoutNarrowing_impl : std::false_type {};
template <typename From, typename To>
struct IsConvertibleWithoutNarrowing_impl<
    From, To, Void_t<decltype(To{std::declval<From>()})>> : std::true_type {};
}  // namespace intern
template <typename From, typename To>
struct IsConvertibleWithoutNarrowing
    : intern::IsConvertibleWithoutNarrowing_impl<From, To> {};
}  // namespace vx
