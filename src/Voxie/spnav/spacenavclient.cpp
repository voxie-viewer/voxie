#include "spacenavclient.hpp"

#include <QtCore/QDebug>
#include <QtCore/QMutex>
#include <QtCore/QSharedPointer>

using namespace voxie::spnav;

SpaceNavClient::SpaceNavClient() : QObject() {
    //qDebug() << "SpaceNavClient()";

    if (!priv)
        priv.reset(createPrivateX11());
    // TODO: add implementation for windows

    if (priv && !priv->isOk())
        priv.reset();
}
SpaceNavClient::~SpaceNavClient() {
    //qDebug() << "~SpaceNavClient()";
}

QSharedPointer<SpaceNavClient> SpaceNavClient::getClient() {
    static QWeakPointer<SpaceNavClient> weak;
    static QMutex weakMutex;
    
    QMutexLocker locker(&weakMutex);

    QSharedPointer<SpaceNavClient> strong = weak;
    if (strong)
        return strong;

    strong.reset(new SpaceNavClient(), [] (SpaceNavClient* obj) { obj->deleteLater(); });
    weak = strong;
    return strong;
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
