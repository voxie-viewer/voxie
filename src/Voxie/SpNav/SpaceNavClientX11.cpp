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

#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)

vx::spnav::SpaceNavClient::Private*
vx::spnav::SpaceNavClient::createPrivateX11() {
  return nullptr;
}

#else

#include <QtCore/QAbstractNativeEventFilter>
#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QMutex>

#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>

#include <QtX11Extras/QX11Info>

#include <xcb/xcb.h>

using namespace vx::spnav;

const quint32 CMD_APP_WINDOW = 27695;
const quint32 CMD_APP_SENS = 27696;

static xcb_screen_t* screen_of_display(xcb_connection_t* connection,
                                       int screen) {
  for (auto iter = xcb_setup_roots_iterator(xcb_get_setup(connection));
       iter.rem; --screen, xcb_screen_next(&iter))
    if (screen == 0) return iter.data;
  return nullptr;
}

static xcb_atom_t internAtom(xcb_connection_t* connection, const char* name) {
  auto cookie = xcb_intern_atom(connection, 0, strlen(name), name);
  auto reply = xcb_intern_atom_reply(connection, cookie, nullptr);
  if (!reply) return 0;
  auto atom = reply->atom;
  free(reply);
  return atom;
}

class SpaceNavClient::PrivateX11 : public QWidget,
                                   QAbstractNativeEventFilter,
                                   public SpaceNavClient::Private {
  SpaceNavClient* snclient;

  xcb_connection_t* connection;
  Display* dpy;
  int appScreen;

  xcb_atom_t motionEvent, buttonPressEvent, buttonReleaseEvent, commandEvent;

  xcb_window_t daemonWin;

  bool ok = false;

  xcb_window_t getDaemonWindow() {
    if (!connection) return 0;
    if (!dpy) return 0;

    auto screen = screen_of_display(connection, appScreen);
    if (!screen) return 0;
    auto root = screen->root;
    if (!root) return 0;

    auto cookie = xcb_get_property(connection, false, root, commandEvent,
                                   XCB_GET_PROPERTY_TYPE_ANY, 0, 1);
    auto reply = xcb_get_property_reply(connection, cookie, nullptr);
    if (!reply) return 0;
    if (xcb_get_property_value_length(reply) < (int)sizeof(xcb_window_t)) {
      free(reply);
      return 0;
    }
    auto win = *(xcb_window_t*)xcb_get_property_value(reply);
    free(reply);

    cookie = xcb_get_property(connection, false, win, XCB_ATOM_WM_NAME,
                              XCB_GET_PROPERTY_TYPE_ANY, 0, 32);
    reply = xcb_get_property_reply(connection, cookie, nullptr);
    if (!reply) return 0;
    QString name = QByteArray((char*)xcb_get_property_value(reply),
                              xcb_get_property_value_length(reply));
    free(reply);

    if (name != "Magellan Window") return 0;

    return win;
  }

 public:
  PrivateX11(SpaceNavClient* snclient) {
    Q_UNUSED(CMD_APP_WINDOW);
    Q_UNUSED(CMD_APP_SENS);

    this->snclient = snclient;

    connection = QX11Info::connection();
    dpy = QX11Info::display();
    appScreen = QX11Info::appScreen();

    motionEvent = internAtom(connection, "MotionEvent");
    buttonPressEvent = internAtom(connection, "ButtonPressEvent");
    buttonReleaseEvent = internAtom(connection, "ButtonReleaseEvent");
    commandEvent = internAtom(connection, "CommandEvent");
    // qDebug() << motionEvent << buttonPressEvent << buttonReleaseEvent
    // <<commandEvent;
    if (!motionEvent || !buttonPressEvent || !buttonReleaseEvent ||
        !commandEvent) {
      qWarning() << "SpaceNavClient: Failed to look up X atom";
      return;
    }

    xcb_window_t win = this->winId();

    daemonWin = getDaemonWindow();
    if (!daemonWin) {
      qWarning() << "SpaceNavClient: Failed to find spacenav daemon window";
      return;
    }

    xcb_client_message_event_t event;
    memset(&event, 0, sizeof(event));
    event.response_type = XCB_CLIENT_MESSAGE;
    event.format = 16;
    event.window = win;
    event.type = commandEvent;
    event.data.data16[0] = ((unsigned int)win & 0xffff0000) >> 16;
    event.data.data16[1] = (unsigned int)win & 0xffff;
    event.data.data16[2] = CMD_APP_WINDOW;
    auto cookie = xcb_send_event_checked(connection, false, daemonWin, 0,
                                         (const char*)&event);
    auto error = xcb_request_check(connection, cookie);
    if (error) {
      qWarning() << "SpaceNavClient: xcb_send_event() failed";
      free(error);
      return;
    }

    QCoreApplication::instance()->installNativeEventFilter(this);

    ok = true;
  }

  ~PrivateX11() {
    // qDebug() << "dtor";
    QCoreApplication::instance()->removeNativeEventFilter(this);
  }

  bool isOk() const override { return ok; }

  virtual bool nativeEventFilter(const QByteArray& eventType, void* message,
                                 long* result) override {
    Q_UNUSED(result);

    // qDebug() << eventType;
    if (eventType != "xcb_generic_event_t") return false;
    auto ev = static_cast<xcb_generic_event_t*>(message);

    auto type = ev->response_type & ~0x80;
    if (type != XCB_CLIENT_MESSAGE) return false;
    auto client = (xcb_client_message_event_t*)ev;

    if (client->type == motionEvent) {
      std::array<qint16, 6> data;
      memcpy(data.data(), client->data.data16 + 2, 6 * sizeof(qint16));
      SpaceNavMotionEvent event(data);
      Q_EMIT snclient->motionEvent(&event);
    } else if (client->type == buttonPressEvent) {
      SpaceNavButtonPressEvent event(client->data.data16[2]);
      Q_EMIT snclient->buttonPressEvent(&event);
    } else if (client->type == buttonReleaseEvent) {
      SpaceNavButtonReleaseEvent event(client->data.data16[2]);
      Q_EMIT snclient->buttonReleaseEvent(&event);
    } else {
      return false;
    }

    return true;
  }
};

SpaceNavClient::Private* SpaceNavClient::createPrivateX11() {
  if (!QX11Info::isPlatformX11()) {
    qWarning() << "SpaceNavClient: Platform is not X11";
    return nullptr;
  } else {
    // qDebug() << QX11Info::connection();
    // qDebug() << QX11Info::display();
    return new PrivateX11(this);
  }
}

#endif
