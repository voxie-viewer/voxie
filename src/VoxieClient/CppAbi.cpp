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

#include "CppAbi.hpp"

#include <VoxieClient/Format.hpp>

#include <memory>

#ifndef _MSC_VER
#include <cxxabi.h>
#endif

namespace vx {
namespace {
struct free_deleter {
  void operator()(void* p) const { std::free(p); }
};
}  // namespace

QString demangleTypeName(const char* mangled_name) {
#ifndef _MSC_VER
  size_t len;
  int status;
  std::unique_ptr<char, free_deleter> demangled(
      abi::__cxa_demangle(mangled_name, NULL, &len, &status));
  if (status == 0)
    return demangled.get();
  else if (status == -2)
    return vx::format("<unable to demangle: {}>", mangled_name);
  else if (status == -1)
    return vx::format("<unable to demangle (memory allocation failed): {}>",
                      mangled_name);
  else if (status == -3)
    return vx::format("<unable to demangle (invalid argument): {}>",
                      mangled_name);
  else
    return vx::format("<unable to demangle (status {}): {}>", status,
                      mangled_name);
#else
  return mangled_name;
#endif
}
}  // namespace vx
