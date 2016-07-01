#include "spacenavevent.hpp"

using namespace voxie::spnav;

SpaceNavEvent::SpaceNavEvent() {}
SpaceNavEvent::~SpaceNavEvent() {}

SpaceNavMotionEvent::SpaceNavMotionEvent(const std::array<qint16, 6>& data) : data_(data) {}
SpaceNavMotionEvent::~SpaceNavMotionEvent() {}

SpaceNavButtonEvent::SpaceNavButtonEvent(quint16 button) : button_(button) {}
SpaceNavButtonEvent::~SpaceNavButtonEvent() {}

SpaceNavButtonPressEvent::SpaceNavButtonPressEvent(quint16 button) : SpaceNavButtonEvent(button) {}
SpaceNavButtonPressEvent::~SpaceNavButtonPressEvent() {}

SpaceNavButtonReleaseEvent::SpaceNavButtonReleaseEvent(quint16 button) : SpaceNavButtonEvent(button) {}
SpaceNavButtonReleaseEvent::~SpaceNavButtonReleaseEvent() {}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
