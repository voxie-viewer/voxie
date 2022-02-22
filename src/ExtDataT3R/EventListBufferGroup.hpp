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

#include <VoxieClient/Array.hpp>
#include <VoxieClient/DBusProxies.hpp>
#include <VoxieClient/DBusTypeList.hpp>
#include <VoxieClient/Exception.hpp>
#include <VoxieClient/MappedBuffer.hpp>

#include <QSharedPointer>
#include <QString>

#include <map>

namespace vx {
namespace t3r {

class EventListBufferGroup {
 public:
  EventListBufferGroup() = default;

  EventListBufferGroup(
      QSharedPointer<de::uni_stuttgart::Voxie::EventListDataBuffer> proxy)
      : proxy(proxy) {
    for (auto& attribute : proxy->attributes()) {
      // TODO options?
      addBuffer(std::get<0>(attribute),
                HANDLEDBUSPENDINGREPLY(proxy->GetAttributeReadonly(
                    std::get<0>(attribute), QMap<QString, QDBusVariant>())));
    }
  }

  EventListBufferGroup(
      const QDBusObjectPath& update,
      QSharedPointer<de::uni_stuttgart::Voxie::EventListDataBuffer> proxy)
      : proxy(proxy) {
    for (auto& attribute : proxy->attributes()) {
      // TODO options?
      addBuffer(
          std::get<0>(attribute),
          HANDLEDBUSPENDINGREPLY(proxy->GetAttributeWritable(
              update, std::get<0>(attribute), QMap<QString, QDBusVariant>())));
    }
  }

  void addBuffer(QString name, vx::Array1Info arrayInfo) {
    if (buffers.empty()) {
      size = arrayInfo.size;
    } else if (size != arrayInfo.size) {
      throw vx::Exception("de.uni_stuttgart.Voxie.ExtDataT3R.Error",
                          QString("Event list buffer size mismatch"));
    }
    buffers.emplace(name, arrayInfo);
  }

  template <typename T>
  Array1<T> getBuffer(const QString& name) const {
    try {
      auto it = buffers.find(name);
      if (it != buffers.end()) {
        return Array1<T>(it->second);
      } else {
        throw vx::Exception("de.uni_stuttgart.Voxie.ExtDataT3R.Error",
                            QString("Missing attribute"));
      }
    } catch (vx::Exception& e) {
      throw vx::Exception(
          "de.uni_stuttgart.Voxie.ExtDataT3R.Error",
          QString("Failed to obtain buffer for event list attribute '%1': %2")
              .arg(name)
              .arg(e.message()));
    }
  }

  std::size_t getBufferSize() const { return size; }

  QSharedPointer<de::uni_stuttgart::Voxie::EventListDataBuffer> getProxy()
      const {
    return proxy;
  }

 private:
  QSharedPointer<de::uni_stuttgart::Voxie::EventListDataBuffer> proxy;
  std::map<QString, vx::Array1Info> buffers;
  std::size_t size = 0;
};

}  // namespace t3r
}  // namespace vx
