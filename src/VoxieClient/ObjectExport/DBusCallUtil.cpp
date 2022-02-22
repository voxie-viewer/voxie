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

#include "DBusCallUtil.hpp"

#include <VoxieClient/ObjectExport/ExportedObject.hpp>

#include <QtDBus/QDBusConnection>

void vx::handleDBusCallOnBackgroundThread(
    vx::ExportedObject* obj, std::function<QList<QDBusVariant>()> fun) {
  auto conn = obj->connection();
  auto msg = obj->message();
  obj->setDelayedReply(true);

  vx::enqueueOnBackgroundThread([conn, msg, fun]() {
    QDBusMessage result;

    try {
      auto values = fun();
      QList<QVariant> data;
      for (const auto& value : values) data << value.variant();
      result = msg.createReply(data);
    } catch (vx::Exception& e) {
      if (vx::Exception::verboseDBusExceptions()) {
        qWarning()
            << "Returning error over DBus (handleDBusCallOnBackgroundThread):"
            << msg.service() << msg.path() << msg.interface() << msg.member()
            << e.name() << e.message();
      }

      result = msg.createErrorReply(e.name(), e.message());
    }
    conn.send(result);
  });
}
