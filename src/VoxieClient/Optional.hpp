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

#include <VoxieClient/VoxieClient.hpp>

#include <QtCore/QtGlobal>

#include <utility>

// This class implements a subset of std::optional<T>

namespace vx {
VOXIECLIENT_EXPORT Q_NORETURN void throwBadOptionalAccess();

struct NullOptT {
  constexpr explicit NullOptT(int) {}
};
static constexpr const NullOptT nullopt{0};

template <typename T>
class Optional {
  bool valid;
  T value_;

 public:
  Optional() : valid(false) {}
  Optional(NullOptT) : valid(false) {}
  Optional(const T& value) : valid(true), value_(value) {}
  Optional(T&& value) : valid(true), value_(std::move(value)) {}

  explicit operator bool() const noexcept { return valid; }
  bool has_value() const noexcept { return valid; }
  T value() const {
    if (!valid) throwBadOptionalAccess();
    return value_;
  }
  T value_or(T defaultValue) {
    if (!valid)
      return defaultValue;
    else
      return value_;
  }
};
}  // namespace vx
