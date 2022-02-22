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

#include <VoxieClient/QDBusPendingReplyWrapper.hpp>

#include <functional>

#include <QtCore/QEventLoop>
#include <QtCore/QException>
#include <QtCore/QString>

#include <QtDBus/QDBusContext>
#include <QtDBus/QDBusPendingReply>
#include <QtDBus/QDBusReply>

namespace vx {
class VOXIECLIENT_EXPORT DBusDefaultReturnValueType {
 public:
  template <typename T>
  operator T() const {
    return T();
  }
};
inline DBusDefaultReturnValueType dbusDefaultReturnValue() {
  return DBusDefaultReturnValueType();
}

class VOXIECLIENT_EXPORT Exception : public QException {
  QString name_;
  QString message_;
  QString what_;
  QByteArray whatUtf8_;
  QString additional_;

 public:
  Exception(const QString& name, const QString& message,
            const QString& additional = "");
  ~Exception() override;
  void raise() const override { throw *this; }
  Exception* clone() const override { return new Exception(*this); }

  const QString& name() const { return name_; }
  const QString& message() const { return message_; }
  const QString& additional() const { return additional_; }
  const char* what() const throw() override;

  DBusDefaultReturnValueType handle(QDBusContext* context);

  static void executeWithHandler(const std::function<void()>& code,
                                 const std::function<void(Exception)>& handler);

  static bool verboseDBusExceptions();
};

// Needs DBusUtil.hpp
template <typename T>
inline T dbusGetVariantValue(const QDBusVariant& variant);

namespace intern {
// TODO: This probably could be done in a more efficient way
template <typename... T>
struct PendingReplyWrapperHelper;
template <typename T, typename... U>
struct PendingReplyWrapperHelper<T, U...> {
  template <typename... V>
  static std::tuple<T, U...> get(const QDBusPendingReplyWrapper<V...>& reply,
                                 size_t pos) {
    auto arg0 = vx::dbusGetVariantValue<T>(QDBusVariant(reply.argumentAt(pos)));
    auto tail = PendingReplyWrapperHelper<U...>::get(reply, pos + 1);
    return std::tuple_cat(std::make_tuple(arg0), tail);
  }
};
template <>
struct PendingReplyWrapperHelper<> {
  template <typename... V>
  static std::tuple<> get(const QDBusPendingReplyWrapper<V...>& reply,
                          size_t pos) {
    (void)reply;
    (void)pos;
    return std::make_tuple();
  }
};
template <typename... T>
struct PendingReplyHelper;
template <typename T, typename... U>
struct PendingReplyHelper<T, U...> {
  template <typename... V>
  static std::tuple<T, U...> get(const QDBusPendingReply<V...>& reply,
                                 size_t pos) {
    auto arg0 = vx::dbusGetVariantValue<T>(QDBusVariant(reply.argumentAt(pos)));
    auto tail = PendingReplyHelper<U...>::get(reply, pos + 1);
    return std::tuple_cat(std::make_tuple(arg0), tail);
  }
};
template <>
struct PendingReplyHelper<> {
  template <typename... V>
  static std::tuple<> get(const QDBusPendingReply<V...>& reply, size_t pos) {
    (void)reply;
    (void)pos;
    return std::make_tuple();
  }
};
}  // namespace intern

VOXIECLIENT_EXPORT void waitForDBusPendingCall(const QDBusPendingCall& call);
template <typename... T>
inline std::enable_if_t<sizeof...(T) >= 2, std::tuple<T...>>
handleDBusPendingReply(const QDBusPendingReplyWrapper<T...>& reply,
                       const QString& additional = "") {
  waitForDBusPendingCall(reply);
  if (reply.isError())
    throw Exception(reply.error().name(), reply.error().message(), additional);
  return vx::intern::PendingReplyWrapperHelper<T...>::get(reply, 0);
}
template <typename T>
inline T handleDBusPendingReply(const QDBusPendingReplyWrapper<T>& reply,
                                const QString& additional = "") {
  waitForDBusPendingCall(reply);
  if (reply.isError())
    throw Exception(reply.error().name(), reply.error().message(), additional);
  return reply.value();
}
template <>
inline void handleDBusPendingReply<void>(
    const QDBusPendingReplyWrapper<>& reply, const QString& additional) {
  waitForDBusPendingCall(reply);
  if (reply.isError())
    throw Exception(reply.error().name(), reply.error().message(), additional);
}
template <typename... T>
inline std::enable_if_t<sizeof...(T) >= 2, std::tuple<T...>>
handleDBusPendingReply(const QDBusPendingReply<T...>& reply,
                       const QString& additional = "") {
  waitForDBusPendingCall(reply);
  if (reply.isError())
    throw Exception(reply.error().name(), reply.error().message(), additional);
  return vx::intern::PendingReplyHelper<T...>::get(reply, 0);
}
template <typename T>
inline T handleDBusPendingReply(const QDBusPendingReply<T>& reply,
                                const QString& additional = "") {
  waitForDBusPendingCall(reply);
  if (reply.isError())
    throw Exception(reply.error().name(), reply.error().message(), additional);
  return reply.value();
}
template <>
inline void handleDBusPendingReply<void>(const QDBusPendingReply<>& reply,
                                         const QString& additional) {
  waitForDBusPendingCall(reply);
  if (reply.isError())
    throw Exception(reply.error().name(), reply.error().message(), additional);
}
template <typename T>
inline T handleDBusPendingReply(QDBusReply<T> reply,
                                const QString& additional = "") {
  if (!reply.isValid())
    throw Exception(reply.error().name(), reply.error().message(), additional);
  return reply.value();
}
template <>
inline void handleDBusPendingReply<void>(QDBusReply<void> reply,
                                         const QString& additional) {
  if (!reply.isValid())
    throw Exception(reply.error().name(), reply.error().message(), additional);
}
}  // namespace vx

#define HANDLEDBUSPENDINGREPLY(call) ::vx::handleDBusPendingReply(call, #call)
