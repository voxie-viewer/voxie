#pragma once

#include <Voxie/voxiecore_global.hpp>

#include <Voxie/spnav/spacenavevent.hpp>

#include <QtCore/QObject>

namespace voxie { namespace spnav {

class VOXIECORESHARED_EXPORT SpaceNavClient : public QObject {
    Q_OBJECT

    friend class SpaceNavWidget;

    class Private {
    public:
        virtual ~Private();
        virtual bool isOk() const = 0;
    };

    class PrivateX11;
    Private* createPrivateX11();

    QScopedPointer<Private> priv;

    SpaceNavClient();

public:
    ~SpaceNavClient();

    static QSharedPointer<SpaceNavClient> getClient();

signals:
    // This events will always be emitted, regardless of which visualizer is
    // active
    void motionEvent(SpaceNavMotionEvent* event);
    void buttonPressEvent(SpaceNavButtonPressEvent* event);
    void buttonReleaseEvent(SpaceNavButtonReleaseEvent* event);
};

} }

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
