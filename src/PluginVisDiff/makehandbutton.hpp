#pragma once

#include <QtWidgets/QPushButton>

class MakeHandButton : public QPushButton {
public:
    MakeHandButton(QWidget *parent = 0)
        : QPushButton(parent)
    {
    }

protected:
    void enterEvent(QEvent* event) {
        Q_UNUSED(event);
        setCursor(Qt::PointingHandCursor);
    }
};

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
