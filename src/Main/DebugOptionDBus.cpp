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

#include "DebugOptionDBus.hpp"

#include <VoxieClient/DBusAdaptors.hpp>

using namespace vx;

class DebugOptionAdaptorImpl : public DebugOptionAdaptor,
                               public ObjectPropertyAdaptor {
  DebugOptionDBus* object;

 public:
  DebugOptionAdaptorImpl(DebugOptionDBus* object)
      : DebugOptionAdaptor(object),
        ObjectPropertyAdaptor(object),
        object(object) {}
  ~DebugOptionAdaptorImpl() override {}

  QDBusSignature dBusSignature() const override {
    try {
      return object->option()->dbusSignature();
    } catch (vx::Exception& e) {
      return e.handle(object);
    }
  }

  QString name() const override {
    try {
      return object->option()->name();
    } catch (vx::Exception& e) {
      return e.handle(object);
    }
  }

  QDBusVariant GetValue(const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options);

      return object->option()->getValueDBus();
    } catch (Exception& e) {
      return e.handle(object);
    }
  }

  void SetValue(const QDBusVariant& value,
                const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options);

      object->option()->setValueDBus(value);
    } catch (Exception& e) {
      e.handle(object);
    }
  }
};

DebugOptionDBus::DebugOptionDBus(DebugOption* option)
    : ExportedObject("DebugOption"), option_(option) {
  new DebugOptionAdaptorImpl(this);
}
