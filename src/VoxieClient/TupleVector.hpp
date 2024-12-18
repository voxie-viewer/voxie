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

#include <VoxieClient/Vector.hpp>

#include <tuple>

// vx::TupleVector<T, dim> is an alias for std::tuple<T, T, ...>
namespace vx {
namespace DBusUtilIntern {
template <typename T, size_t c>
struct TupleVectorImpl {
  template <typename... Tail>
  static std::tuple<T, Tail...> addT(std::tuple<Tail...>);

  typedef typename TupleVectorImpl<T, c - 1>::type prev;
  typedef decltype(addT(*(prev*)nullptr)) type;
};
template <typename T>
struct TupleVectorImpl<T, 0> {
  typedef std::tuple<> type;
};
}  // namespace DBusUtilIntern
template <typename T, size_t c>
using TupleVector = typename DBusUtilIntern::TupleVectorImpl<T, c>::type;

// Method "vx::Vector<T, dim> toVector(vx::TupleVector<T, dim>)" where template
// argument deduction works
namespace DBusUtilIntern {
// Note: TupleVectorTraits<T, U, ...> will be incomplete if T != U
// TupleVectorTraits<> will also be incomplete if the list of template
// parameters is empty (because the type for the vector cannot be deduced).
template <typename... Args>
struct TupleVectorTraits;
template <typename T>
struct TupleVectorTraits<T> {
  using Type = T;
};
template <typename T, typename... Args>
struct TupleVectorTraits<T, T, Args...> : TupleVectorTraits<T, Args...> {};

template <typename T, size_t dim, size_t start_pos = 0>
struct TupleVectorCopy {
  static void copyValueToVector(vx::Vector<T, dim>& out,
                                const vx::TupleVector<T, dim>& tuple) {
    static_assert(start_pos < dim, "start_pos < dim");
    out.template access<start_pos>() = std::get<start_pos>(tuple);
    TupleVectorCopy<T, dim, start_pos + 1>::copyValueToVector(out, tuple);
  }
  static void copyValueToTuple(vx::TupleVector<T, dim>& out,
                               const vx::Vector<T, dim>& vector) {
    static_assert(start_pos < dim, "start_pos < dim");
    std::get<start_pos>(out) = vector.template access<start_pos>();
    TupleVectorCopy<T, dim, start_pos + 1>::copyValueToTuple(out, vector);
  }
};
template <typename T, size_t dim>
struct TupleVectorCopy<T, dim, dim> {
  static void copyValueToVector(vx::Vector<T, dim>& out,
                                const vx::TupleVector<T, dim>& tuple) {
    (void)out;
    (void)tuple;
  }
  static void copyValueToTuple(vx::TupleVector<T, dim>& out,
                               const vx::Vector<T, dim>& vector) {
    (void)out;
    (void)vector;
  }
};
}  // namespace DBusUtilIntern
template <typename... Args>
vx::Vector<typename DBusUtilIntern::TupleVectorTraits<Args...>::Type,
           sizeof...(Args)>
toVector(const std::tuple<Args...>& tuple) {
  using T = typename DBusUtilIntern::TupleVectorTraits<Args...>::Type;
  const size_t dim = sizeof...(Args);
  vx::Vector<T, dim> out;
  DBusUtilIntern::TupleVectorCopy<T, dim>::copyValueToVector(out, tuple);
  return out;
}
// Opposite direction, template deduction is easier
template <typename T, size_t dim>
vx::TupleVector<T, dim> toTupleVector(const vx::Vector<T, dim>& vector) {
  vx::TupleVector<T, dim> out;
  DBusUtilIntern::TupleVectorCopy<T, dim>::copyValueToTuple(out, vector);
  return out;
}
}  // namespace vx
