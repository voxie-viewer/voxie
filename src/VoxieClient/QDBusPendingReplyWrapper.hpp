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

#include <QtDBus/QDBusPendingReply>

namespace vx {
/**
 * A wrapper around QDBusPendingReply which will show a warning when the
 * implicit conversion is used.
 */
template <typename T1 = void, typename... T>
class QDBusPendingReplyWrapper {
  QDBusPendingReply<T1, T...> val;

 public:
  QDBusPendingReplyWrapper(const QDBusMessage& message) : val(message) {}
  QDBusPendingReplyWrapper(const QDBusPendingCall& call) : val(call) {}
  QDBusPendingReplyWrapper(const QDBusPendingReply<T1, T...>& other)
      : val(other) {}
  QDBusPendingReplyWrapper() : val() {}
  QVariant argumentAt(int index) const { return val.argumentAt(index); }
  int count() const { return val.count(); }
  QDBusError error() const { return val.error(); }
  bool isError() const { return val.isError(); }
  bool isFinished() const { return val.isFinished(); }
  bool isValid() const { return val.isValid(); }
  QDBusMessage reply() const { return val.reply(); }
  T1 value() const { return val.value(); }
  void waitForFinished() { return val.waitForFinished(); }

  Q_DECL_DEPRECATED operator T1() const { return val; }

  // operator QDBusPendingReply<T1, T...>() const { return val; }
  operator QDBusPendingCall() const { return val; }
};
}  // namespace vx
