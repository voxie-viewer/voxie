#pragma once

#include <QtWidgets/QLabel>

namespace voxie
{
namespace gui
{

class ButtonLabel :
        public QLabel
{
    Q_OBJECT
public:
    explicit ButtonLabel(QWidget *parent = 0);

protected:
    virtual void mousePressEvent(QMouseEvent *) override;

    virtual void enterEvent(QEvent*) override;

    virtual void leaveEvent(QEvent*) override;

signals:

    void clicked();

public slots:

};

}
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
