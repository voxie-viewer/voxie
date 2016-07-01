#include "spacenavclient.hpp"

#include <QtCore/QtGlobal>
#include <QtNetwork/QLocalSocket>

#ifdef Q_OS_WIN

voxie::spnav::SpaceNavClient::Private* voxie::spnav::SpaceNavClient::createPrivateUnix() {
    return nullptr;
}

#else

#include <QtCore/QDebug>

using namespace voxie::spnav;

class SpaceNavClient::PrivateUnix : public QObject, public SpaceNavClient::Private {
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

        connect(&socket, static_cast<void(QLocalSocket::*)(QLocalSocket::LocalSocketError)>(&QLocalSocket::error), this, [this] (QLocalSocket::LocalSocketError socketError) {
                qWarning() << "Got error from spacenavd socket:" << socketError;
            });

        connect(&socket, &QIODevice::readyRead, this, [this] {
                for (;;) {
                    int bytes = socket.read(((char*) buffer) + pos, sizeof(buffer) - pos);
                    if (bytes == -1) {
                        qWarning() << "Read from spacenavd socket failed";
                        return;
                    }
                    if (bytes == 0)
                        return;
                    pos += bytes;
                    if (pos == sizeof(buffer)) {
                        pos = 0;
                        switch (buffer[0]) {
                        case 0: {
                            std::array<qint16, 6> data;
                            for (int i = 0; i < 6; i++)
                                data[i] = buffer[i + 1];
                            SpaceNavMotionEvent event(data);
                            emit this->snclient->motionEvent(&event);
                            break;
                        }
                        case 1: {
                            SpaceNavButtonPressEvent event(buffer[1]);
                            emit this->snclient->buttonPressEvent(&event);
                            break;
                        }
                        case 2: {
                            SpaceNavButtonReleaseEvent event(buffer[1]);
                            emit this->snclient->buttonReleaseEvent(&event);
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

    ~PrivateUnix() {
    }

    bool isOk() const { return ok; }
};

SpaceNavClient::Private* SpaceNavClient::createPrivateUnix() {
    return new PrivateUnix(this);
}

#endif

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
