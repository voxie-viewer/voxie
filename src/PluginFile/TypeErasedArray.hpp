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

#include <VoxieClient/Array.hpp>
#include <VoxieClient/Exception.hpp>

#include <QVariant>

#include <functional>

// Register arrays for use in QVariant
Q_DECLARE_METATYPE(QSharedPointer<vx::Array1<const int8_t>>)
Q_DECLARE_METATYPE(QSharedPointer<vx::Array1<const int16_t>>)
Q_DECLARE_METATYPE(QSharedPointer<vx::Array1<const int32_t>>)
Q_DECLARE_METATYPE(QSharedPointer<vx::Array1<const int64_t>>)
Q_DECLARE_METATYPE(QSharedPointer<vx::Array1<const uint8_t>>)
Q_DECLARE_METATYPE(QSharedPointer<vx::Array1<const uint16_t>>)
Q_DECLARE_METATYPE(QSharedPointer<vx::Array1<const uint32_t>>)
Q_DECLARE_METATYPE(QSharedPointer<vx::Array1<const uint64_t>>)
Q_DECLARE_METATYPE(QSharedPointer<vx::Array1<const float>>)
Q_DECLARE_METATYPE(QSharedPointer<vx::Array1<const double>>)

namespace vx {
namespace file {

class TypeErasedArray1 {
 public:
  TypeErasedArray1(const vx::Array1Info& info) {
    if (info.dataType == "uint") {
      switch (info.dataTypeSize) {
        case 8:
          init<uint8_t>(info);
          break;
        case 16:
          init<uint16_t>(info);
          break;
        case 32:
          init<uint32_t>(info);
          break;
        case 64:
          init<uint64_t>(info);
          break;
      }
    } else if (info.dataType == "int") {
      switch (info.dataTypeSize) {
        case 8:
          init<int8_t>(info);
          break;
        case 16:
          init<int16_t>(info);
          break;
        case 32:
          init<int32_t>(info);
          break;
        case 64:
          init<int64_t>(info);
          break;
      }
    } else if (info.dataType == "float") {
      init<float>(info);
    } else if (info.dataType == "double") {
      init<double>(info);
    }

    if (variant.isNull()) {
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "TypeErasedArray1: Array is of unknown type");
    }
  }

  template <typename Ret, typename Func>
  std::function<Ret(size_t)> specialize(Func func) {
    std::function<Ret(size_t)> result;
    // Use short-circuiting behavior of || to stop on matching type
    specializeImpl<int8_t>(result, func) ||
        specializeImpl<int16_t>(result, func) ||
        specializeImpl<int32_t>(result, func) ||
        specializeImpl<int64_t>(result, func) ||
        specializeImpl<uint8_t>(result, func) ||
        specializeImpl<uint16_t>(result, func) ||
        specializeImpl<uint32_t>(result, func) ||
        specializeImpl<uint64_t>(result, func) ||
        specializeImpl<float>(result, func) ||
        specializeImpl<double>(result, func);
    return result;
  }

 private:
  template <typename T>
  void init(const vx::Array1Info& info) {
    variant.setValue(QSharedPointer<vx::Array1<const T>>::create(info));
  }

  template <typename T, typename Ret, typename Func>
  bool specializeImpl(std::function<Ret(size_t)>& result, Func func) {
    using ArrayPtr = QSharedPointer<vx::Array1<const T>>;
    if (variant.canConvert<ArrayPtr>()) {
      auto array = variant.value<ArrayPtr>();
      result = std::function<Ret(size_t)>(
          [func, array](size_t index) { func((*array)(index)); });
      return true;
    } else {
      return false;
    }
  }

  QVariant variant;
};

}  // namespace file
}  // namespace vx
