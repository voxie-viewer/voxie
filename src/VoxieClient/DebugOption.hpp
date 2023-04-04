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

#include <VoxieClient/QtUtil.hpp>
#include <VoxieClient/VoxieClient.hpp>

#include <QtCore/QDebug>
#include <QtCore/QPointer>
#include <QtCore/QSharedPointer>
#include <QtCore/QString>

#include <QtDBus/QDBusSignature>
#include <QtDBus/QDBusVariant>

namespace vx {
class VOXIECLIENT_EXPORT DebugOption {
  const char* name_;

 public:
  DebugOption(const char* name);
  virtual ~DebugOption();

  const char* name();

  virtual void setValueStringEmpty() = 0;
  virtual void setValueString(const QString& value) = 0;

  virtual QDBusSignature dbusSignature() = 0;
  virtual QDBusVariant getValueDBus() = 0;
  virtual void setValueDBus(const QDBusVariant& value) = 0;
};

class VOXIECLIENT_EXPORT DebugOptionBool : public DebugOption {
  QAtomicInt valueAtomic;
  QList<
      std::tuple<QPointer<QObject>, QSharedPointer<std::function<void(bool)>>>>
      changeHandlers;

 public:
  DebugOptionBool(const char* name) : DebugOption(name), valueAtomic(0) {}
  bool get() { return valueAtomic.load(); }
  void set(bool value);
  bool enabled() { return get(); }

  void registerAndCallChangeHandler(QObject* obj,
                                    std::function<void(bool)>&& fun);

  void setValueStringEmpty() override;
  void setValueString(const QString& value) override;

  QDBusSignature dbusSignature() override;
  QDBusVariant getValueDBus() override;
  void setValueDBus(const QDBusVariant& value) override;
};
}  // namespace vx

#define VX_DEFINE_DEBUG_OPTION_BOOL(OptionName, OptionSName) \
  namespace vx {                                             \
  namespace debug_option_impl {                              \
  vx::DebugOptionBool OptionName##_option(OptionSName);      \
  }                                                          \
  vx::DebugOptionBool* vx::debug_option::OptionName() {      \
    return &vx::debug_option_impl::OptionName##_option;      \
  }                                                          \
  }
