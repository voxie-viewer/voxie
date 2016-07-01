#pragma once

#include <Voxie/voxiecore_global.hpp>

#include <QtCore/QtGlobal>

#include <QtGui/QVector3D>

#include <array>

namespace voxie { namespace spnav {

class VOXIECORESHARED_EXPORT SpaceNavEvent {
public:
    SpaceNavEvent();
    virtual ~SpaceNavEvent();
};

class VOXIECORESHARED_EXPORT SpaceNavMotionEvent : public SpaceNavEvent {
    std::array<qint16, 6> data_;

public:
    SpaceNavMotionEvent(const std::array<qint16, 6>& data);
    ~SpaceNavMotionEvent() override;

    const std::array<qint16, 6>& data() const { return data_; }

    QVector3D translation() const { return QVector3D(data()[0], data()[1], -data()[2]); }
    QVector3D rotation() const { return QVector3D(data()[3], data()[4], -data()[5]); }
};

class VOXIECORESHARED_EXPORT SpaceNavButtonEvent : public SpaceNavEvent {
    quint16 button_;

public:
    SpaceNavButtonEvent(quint16 button);
    ~SpaceNavButtonEvent() override;

    quint16 button() const { return button_; }

    virtual bool pressed() const = 0;
};

class VOXIECORESHARED_EXPORT SpaceNavButtonPressEvent : public SpaceNavButtonEvent {
public:
    SpaceNavButtonPressEvent(quint16 button);
    ~SpaceNavButtonPressEvent() override;

    bool pressed() const override { return true; }
};

class VOXIECORESHARED_EXPORT SpaceNavButtonReleaseEvent : public SpaceNavButtonEvent {
public:
    SpaceNavButtonReleaseEvent(quint16 button);
    ~SpaceNavButtonReleaseEvent() override;

    bool pressed() const override { return false; }
};

} }

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
