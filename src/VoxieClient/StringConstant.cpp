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

#include "StringConstant.hpp"

VX_DECLARE_STRING_CONSTANT_GLOBAL(foo0, "foo");

using nulval0 = vx::StringConstant<'n', 'u', 'l', '\0', 'v', 'a', 'l'>;
VX_DECLARE_STRING_CONSTANT_GLOBAL(nulval1, "nul\0val");
static_assert(std::is_same<nulval0, nulval1>::value, "");

class SomeClass {
 public:
  VX_DECLARE_STRING_CONSTANT_CLASS(foo4, "foo");
  static_assert(std::is_same<foo0, foo4>::value, "");

  VX_DECLARE_STRING_CONSTANT_CLASS(nulval2, "nul\0val");
  static_assert(std::is_same<nulval0, nulval2>::value, "");
};

[[gnu::unused]] static inline void test() {
  auto fooValue1 = VX_GET_STRING_CONSTANT_VALUE("foo");
  using foo1 = decltype(fooValue1);
  auto fooValue2 = VX_GET_STRING_CONSTANT_VALUE("foo");
  using foo2 = decltype(fooValue2);
  using foo3 = vx::StringConstant<'f', 'o', 'o'>;

  static_assert(std::is_same<foo1, foo2>::value, "");
  static_assert(std::is_same<foo2, foo3>::value, "");

  static_assert(std::is_same<foo0, foo1>::value, "");
}
