/*
 * Copyright (c) 2014-2024 The Voxie Authors
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

#define FMT_SHARED 1

#include <fmt/format.h>

#include <VoxieClient/VoxieClient.hpp>

#include <QtCore/QString>

namespace fmt {
template <class CharT>
struct formatter<QString, CharT> : fmt::formatter<std::string, CharT> {
  template <class FormatContext>
  auto format(QString t, FormatContext& fc) const {
    return fmt::formatter<std::string, CharT>::format(t.toStdString(), fc);
  }
};
}  // namespace fmt

namespace vx {
namespace intern {
VOXIECLIENT_EXPORT Q_NORETURN void throwFormatError(const std::string& fmt_str,
                                                    fmt::format_error& error);
}

template <typename... Args>
QString format(fmt::format_string<Args...> fmt, Args&&... args) {
  try {
    return QString::fromStdString(
        fmt::format(fmt, std::forward<Args>(args)...));
  } catch (fmt::format_error& error) {
    const auto& fmt_str = fmt.get();
    intern::throwFormatError(std::string(fmt_str.data(), fmt_str.size()),
                             error);
  }
}
}  // namespace vx
