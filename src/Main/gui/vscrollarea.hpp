#pragma once

#include <QtWidgets/QScrollArea>

namespace voxie
{
namespace gui
{


class VScrollArea :
        public QScrollArea
{
    Q_OBJECT
public:
    explicit VScrollArea(QWidget *parent = 0);

    bool eventFilter(QObject *o, QEvent *e);

signals:

public slots:

};

}
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
