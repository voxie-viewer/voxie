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

#include "DebugOption.hpp"

#include <VoxieClient/DBusUtil.hpp>

using namespace vx;

DebugOption::DebugOption(const char* name) : name_(name) {}
DebugOption::~DebugOption() {}

const char* DebugOption::name() { return name_; }

void DebugOptionBool::set(bool value) {
  vx::checkOnMainThread("DebugOptionBool::set()");
  bool old = valueAtomic.load();
  if (old == value) return;
  valueAtomic.store(value ? 1 : 0);

  for (const auto& handler : changeHandlers) {
    // TODO: Clean up deleted objects?
    if (std::get<0>(handler)) (*std::get<1>(handler))(value);
  }
}

void DebugOptionBool::registerAndCallChangeHandler(
    QObject* obj, std::function<void(bool)>&& fun) {
  vx::checkOnMainThread("DebugOptionBool::registerAndCallChangeHandler()");

  auto data = std::make_tuple(
      QPointer<QObject>(obj),
      createQSharedPointer<std::function<void(bool)>>(std::move(fun)));

  changeHandlers << data;

  (*std::get<1>(data))(get());
}

void DebugOptionBool::setValueStringEmpty() { set(true); }
void DebugOptionBool::setValueString(const QString& value) {
  auto lower = value.toLower();
  if (lower == "0" || lower == "false")
    set(false);
  else if (lower == "1" || lower == "true")
    set(true);
  else
    qWarning() << "Invalid value for debug option" << name() << ":" << value;
}

QDBusSignature DebugOptionBool::dbusSignature() { return QDBusSignature("b"); }

QDBusVariant DebugOptionBool::getValueDBus() {
  return vx::dbusMakeVariant<bool>(get());
}
void DebugOptionBool::setValueDBus(const QDBusVariant& value) {
  set(vx::dbusGetVariantValue<bool>(value));
}
