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

#include "Any.hpp"

#include <VoxieClient/CppAbi.hpp>
#include <VoxieClient/Exception.hpp>
#include <VoxieClient/Format.hpp>
#include <VoxieClient/ToString.hpp>

Q_NORETURN void vx::throwBadAnyCast(const std::type_info& expected,
                                    const std::type_info& actual) {
  throw vx::Exception(
      "de.uni_stuttgart.Voxie.BadAnyCast",
      vx::format(
          "Bad vx::any_cast: Expected type {} (0x{:x}), got type {} (0x{:x})",
          vx::demangleTypeName(expected.name()), (uintptr_t)&expected,
          vx::demangleTypeName(actual.name()), (uintptr_t)&actual));
}

Q_NORETURN void vx::throwNoToStringMethod(const std::type_info& type) {
  throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                      vx::format("No toString() method for vx::Any of type {}",
                                 vx::demangleTypeName(type.name())));
}

QString vx::Intern::typeInfoToString(const std::type_info& info) {
  return vx::toString(info);
}

const QString vx::ToStringImpl<vx::Any>::toString(const vx::Any& value) {
  auto data = value.data.get();
  if (!data)
    return "no-value";
  else if (!data->hasToString())
    return vx::demangleTypeName(value.type().name());
  else
    return vx::demangleTypeName(value.type().name()) + "(" + data->toString() +
           ")";
}
