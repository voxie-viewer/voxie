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

#include <VoxieClient/DBusValue.hpp>
#include <VoxieClient/ToStringImpl.hpp>
#include <VoxieClient/VoxieClient.hpp>

namespace vx {
// An alternative to QDBusVariant which allows reading a created value.
// (QDBusVariant does not allow this if the value is a QDBusArgument, which is
// required for more complex values.)
//
// Values are immutable.
class VOXIECLIENT_EXPORT DBusVariant {
  DBusValue value_;

 public:
  explicit DBusVariant(const DBusValue& value) : value_(value) {}

  static DBusVariant fromQDBusVariant(const QDBusVariant& variant);
  QDBusVariant asQDBusVariant() const;

  QDBusSignature valueSignature() const;

  const DBusValue& value() const { return value_; }

  friend VOXIECLIENT_EXPORT bool operator==(const DBusVariant& v1,
                                            const DBusVariant& v2);
  friend VOXIECLIENT_EXPORT bool operator!=(const DBusVariant& v1,
                                            const DBusVariant& v2);
};
VOXIECLIENT_EXPORT bool operator==(const DBusVariant& v1,
                                   const DBusVariant& v2);
VOXIECLIENT_EXPORT bool operator!=(const DBusVariant& v1,
                                   const DBusVariant& v2);

template <>
struct VOXIECLIENT_EXPORT ToStringImpl<DBusVariant> {
  static const QString toString(const DBusVariant& variant);
};

}  // namespace vx
