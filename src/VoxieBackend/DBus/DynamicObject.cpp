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

#include "DynamicObject.hpp"

#include <VoxieClient/DBusAdaptors.hpp>

namespace vx {
namespace {
class DynamicObjectAdaptorImpl : public DynamicObjectAdaptor {
  DynamicObject* object;

 public:
  DynamicObjectAdaptorImpl(DynamicObject* object)
      : DynamicObjectAdaptor(object), object(object) {}
  ~DynamicObjectAdaptorImpl() override {}

  QStringList supportedInterfaces() const override {
    try {
      return object->supportedDBusInterfaces();
    } catch (Exception& e) {
      e.handle(object);
      return {};
    }
  }
};
}  // namespace
}  // namespace vx

vx::DynamicObject::DynamicObject(const QString& name)
    : vx::RefCountedObject(name) {
  new vx::DynamicObjectAdaptorImpl(this);
}
