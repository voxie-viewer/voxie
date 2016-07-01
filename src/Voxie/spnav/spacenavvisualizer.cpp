#include "spacenavvisualizer.hpp"

#include <Voxie/ivoxie.hpp>
#include <Voxie/visualization/visualizer.hpp>

#include <QtCore/QDebug>
#include <QtCore/QAbstractNativeEventFilter>
#include <QtCore/QCoreApplication>
#include <QtCore/QMutex>

#include <QtWidgets/QWidget>
#include <QtWidgets/QApplication>

using namespace voxie::spnav;

SpaceNavVisualizerMaster::SpaceNavVisualizerMaster() : QObject() {
    client = SpaceNavClient::getClient();

    connect(voxie::voxieRoot().activeVisualizerProvider(), &voxie::ActiveVisualizerProvider::activeVisualizerChanged, this, [this] (voxie::visualization::Visualizer* now) {
            if (visualizers.contains(now)) {
                auto old = current;
                current = visualizers[now];
                if (old && old != current)
                    old->looseFocus();
            }
        });

    connect(client.data(), &SpaceNavClient::motionEvent, this, [this] (SpaceNavMotionEvent* ev) {
            if (current)
                emit current->motionEvent(ev);
        });
    connect(client.data(), &SpaceNavClient::buttonPressEvent, this, [this] (SpaceNavButtonPressEvent* ev) {
            if (current)
                emit current->buttonPressEvent(ev);
        });
    connect(client.data(), &SpaceNavClient::buttonReleaseEvent, this, [this] (SpaceNavButtonReleaseEvent* ev) {
            if (current)
                emit current->buttonReleaseEvent(ev);
        });
}
SpaceNavVisualizerMaster::~SpaceNavVisualizerMaster() {
    //qDebug() << "~SpaceNavVisualizerMaster()";
}

QSharedPointer<SpaceNavVisualizerMaster> SpaceNavVisualizerMaster::getMaster() {
    static QWeakPointer<SpaceNavVisualizerMaster> weak;
    static QMutex weakMutex;
    
    QMutexLocker locker(&weakMutex);

    QSharedPointer<SpaceNavVisualizerMaster> strong = weak;
    if (strong)
        return strong;

    strong.reset(new SpaceNavVisualizerMaster(), [] (SpaceNavVisualizerMaster* obj) { obj->deleteLater(); });
    weak = strong;
    return strong;
}

SpaceNavVisualizer::SpaceNavVisualizer(voxie::visualization::Visualizer* parent) : QObject(parent) {
    if (!parent) {
        qCritical() << "parent is null";
        return;
    }

    master = SpaceNavVisualizerMaster::getMaster();

    if (master->visualizers.contains(parent)) {
        qCritical() << "Visualizer" << parent << "already has a SpaceNavVisualizer";
        return;
    }

    connect(this, &QObject::destroyed, master.data(), [this, parent] {
            if (master->current == this) {
                master->current = nullptr;
            }
            master->visualizers.remove(parent);
        });

    if (voxie::voxieRoot().activeVisualizerProvider()->activeVisualizer() == parent)
        master->current = this;
    master->visualizers[parent] = this;
}
SpaceNavVisualizer::~SpaceNavVisualizer() {
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
