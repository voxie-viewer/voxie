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

#include "SpaceNavClient.hpp"

#include <QtCore/QtGlobal>
#include <QtNetwork/QLocalSocket>

#ifdef Q_OS_WIN

vx::spnav::SpaceNavClient::Private*
vx::spnav::SpaceNavClient::createPrivateUnix() {
  return nullptr;
}

#else

#include <QtCore/QDebug>

using namespace vx::spnav;

class SpaceNavClient::PrivateUnix : public QObject,
                                    public SpaceNavClient::Private {
  SpaceNavClient* snclient;

  bool ok = false;

  QLocalSocket socket;
  int buffer[8];
  int pos = 0;

 public:
  PrivateUnix(SpaceNavClient* snclient) {
    this->snclient = snclient;

    socket.connectToServer("/var/run/spnav.sock");
    if (!socket.waitForConnected()) {
      qWarning() << "Could not connect to spacenavd socket";
      return;
    }

    connect(&socket,
            static_cast<void (QLocalSocket::*)(QLocalSocket::LocalSocketError)>(
                &QLocalSocket::error),
            this, [](QLocalSocket::LocalSocketError socketError) {
              qWarning() << "Got error from spacenavd socket:" << socketError;
            });

    connect(&socket, &QIODevice::readyRead, this, [this] {
      for (;;) {
        int bytes = socket.read(((char*)buffer) + pos, sizeof(buffer) - pos);
        if (bytes == -1) {
          qWarning() << "Read from spacenavd socket failed";
          return;
        }
        if (bytes == 0) return;
        pos += bytes;
        if (pos == sizeof(buffer)) {
          pos = 0;
          switch (buffer[0]) {
            case 0: {
              std::array<qint16, 6> data;
              for (int i = 0; i < 6; i++) data[i] = buffer[i + 1];
              SpaceNavMotionEvent event(data);
              Q_EMIT this->snclient->motionEvent(&event);
              break;
            }
            case 1: {
              SpaceNavButtonPressEvent event(buffer[1]);
              Q_EMIT this->snclient->buttonPressEvent(&event);
              break;
            }
            case 2: {
              SpaceNavButtonReleaseEvent event(buffer[1]);
              Q_EMIT this->snclient->buttonReleaseEvent(&event);
              break;
            }
            default: {
              qWarning() << "Unkown spacenavd event" << buffer[0];
              break;
            }
          }
        }
      }
    });

    ok = true;
  }

  ~PrivateUnix() {}

  bool isOk() const override { return ok; }
};

SpaceNavClient::Private* SpaceNavClient::createPrivateUnix() {
  return new PrivateUnix(this);
}

#endif
