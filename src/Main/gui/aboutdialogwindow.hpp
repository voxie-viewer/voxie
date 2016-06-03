#pragma once

#include <QtWidgets/QDialog>

namespace voxie
{
namespace gui
{
/**
 * @brief The AboutDialogWindow class provides the about-dialog in the gui
 * @author Tim Borner
 */
class AboutDialogWindow :
        public QDialog
{
    Q_OBJECT
public:
    explicit AboutDialogWindow(QWidget *parent = 0);
    ~AboutDialogWindow();

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
