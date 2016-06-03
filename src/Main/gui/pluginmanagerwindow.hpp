#pragma once

#include <QtWidgets/QDialog>

namespace voxie
{
namespace gui
{
/**
 * @brief The PluginManagerWindow class provides information about the currently loaded addons
 * @author Tim Borner
 */
class PluginManagerWindow :
        public QDialog
{
    Q_OBJECT
public:
    explicit PluginManagerWindow(QWidget *parent = 0);
    ~PluginManagerWindow();

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
