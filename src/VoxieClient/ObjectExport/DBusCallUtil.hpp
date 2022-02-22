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

#include <QtCore/QObject>

#include <QtDBus/QDBusVariant>

#include <functional>

namespace vx {
class ExportedObject;

// Needs VoxieClient/DBusUtil.hpp
template <typename T>
inline QDBusVariant dbusMakeVariant(const T& value);

VOXIECLIENT_EXPORT
void handleDBusCallOnBackgroundThread(vx::ExportedObject* obj,
                                      std::function<QList<QDBusVariant>()> fun);

// Version with more type checking for returning one result
// Use with 'return vx::handleDBusCallOnBackgroundThreadOne<...>(...);'
template <typename T>
Q_REQUIRED_RESULT inline T handleDBusCallOnBackgroundThreadOne(
    vx::ExportedObject* obj, std::function<T()> fun) {
  handleDBusCallOnBackgroundThread(obj, [fun]() {
    return QList<QDBusVariant>{
        vx::dbusMakeVariant<T>(fun()),
    };
  });
  return T();
}

inline void handleDBusCallOnBackgroundThreadVoid(vx::ExportedObject* obj,
                                                 std::function<void()> fun) {
  handleDBusCallOnBackgroundThread(obj, [fun]() {
    fun();
    return QList<QDBusVariant>{};
  });
  return;
}
}  // namespace vx
