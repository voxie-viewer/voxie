#pragma once

#include <Voxie/voxiecore_global.hpp>

#include <Voxie/spnav/spacenavclient.hpp>

#include <QtCore/QObject>
#include <QtCore/QSharedPointer>
#include <QtCore/QMap>

namespace voxie {
namespace visualization {
class Visualizer;
} }

namespace voxie { namespace spnav {

class SpaceNavVisualizer;

class VOXIECORESHARED_EXPORT SpaceNavVisualizerMaster : public QObject {
    Q_OBJECT

    friend class SpaceNavVisualizer;

    QSharedPointer<SpaceNavClient> client;

    QMap<voxie::visualization::Visualizer*, SpaceNavVisualizer*> visualizers;
    SpaceNavVisualizer* current = nullptr;

    SpaceNavVisualizerMaster();

public:
    ~SpaceNavVisualizerMaster();

    static QSharedPointer<SpaceNavVisualizerMaster> getMaster();
};

class VOXIECORESHARED_EXPORT SpaceNavVisualizer : public QObject {
    Q_OBJECT

    QSharedPointer<SpaceNavVisualizerMaster> master;

public:
    SpaceNavVisualizer(voxie::visualization::Visualizer* parent);
    ~SpaceNavVisualizer();

signals:
    // This events will only be emitted if the parent visualizer is active
    void motionEvent(SpaceNavMotionEvent* event);
    void buttonPressEvent(SpaceNavButtonPressEvent* event);
    void buttonReleaseEvent(SpaceNavButtonReleaseEvent* event);

    void looseFocus();
};

} }

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
