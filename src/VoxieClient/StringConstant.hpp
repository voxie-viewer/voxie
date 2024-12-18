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

#include <cstddef>
#include <string>
#include <tuple>
#include <utility>

// String constant type to allow passing strings as template parameters in
// C++14.
// Note: Starting with C++20 this will be much easier:
// https://ctrpeach.io/posts/cpp20-string-literal-template-parameters/

namespace vx {
template <char... Chars>
struct StringConstant {
  // Note: Before C++17 these data_const and size_const should not be ODR-used
  // (Otherwise they would need a definition.)
  static constexpr const char data_const[] = {Chars..., '\0'};
  static constexpr const size_t size_const = sizeof...(Chars);

  static constexpr size_t size() { return sizeof...(Chars); }
  static const char* data() {
    static constexpr const char value[size_const + 1] = {Chars..., '\0'};
    return value;
  }

  static std::string as_string() { return std::string(data(), size()); }
};

struct StringConstantFromArrayHelper {
  template <size_t size_with_nul, const char data[size_with_nul], size_t... Idx>
  static constexpr decltype(auto) buildStringConstant(
      std::index_sequence<Idx...>) noexcept {
    static_assert(size_with_nul == sizeof...(Idx) + 1,
                  "size_with_nul == sizeof...(Idx) + 1");
    std::tuple<std::integral_constant<size_t, Idx>...> tuple{};
    return StringConstant<data[std::get<Idx>(tuple)]...>{};
  }
};
template <size_t size_with_nul, const char data[size_with_nul]>
using StringConstantFromArray =
    decltype(StringConstantFromArrayHelper::buildStringConstant<size_with_nul,
                                                                data>(
        std::make_index_sequence<size_with_nul - 1>()));

struct StringConstantHelper {
  template <typename F, size_t... Idx>
  static constexpr decltype(auto) applyIndexValues(std::index_sequence<Idx...>,
                                                   F f) noexcept {
    std::tuple<std::integral_constant<size_t, Idx>...> tuple{};
    return f(std::get<Idx>(tuple)...);
  }
  static constexpr size_t constexprStrLen(const char* c) {
    size_t t = 0;
    while (*c++) ++t;
    return t;
  }
};
#define VX_GET_STRING_CONSTANT_VALUE(s)                                  \
  ::vx::StringConstantHelper::applyIndexValues(                          \
      std::make_index_sequence<sizeof(s) - 1>(), [](auto... indices) {   \
        return ::vx::StringConstant<(s)[decltype(indices)::value]...>(); \
      })

#define VX_DECLARE_STRING_CONSTANT_GLOBAL(name, s)                  \
  [[gnu::unused]] static inline auto vx_string_const_fun_##name() { \
    return VX_GET_STRING_CONSTANT_VALUE(s);                         \
  }                                                                 \
  using name = decltype(vx_string_const_fun_##name())

#define VX_DECLARE_STRING_CONSTANT_CLASS(name, s)                         \
  static constexpr const char vx_string_const_array_##name[] = s;         \
  using name =                                                            \
      ::vx::StringConstantFromArray<sizeof(vx_string_const_array_##name), \
                                    vx_string_const_array_##name>

}  // namespace vx
