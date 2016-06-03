#pragma once

#include <Voxie/plugin/interfaces.hpp>

#include <QtCore/QVector>

#include <QtGui/QGenericPlugin>

/**
 * @brief The SliceView class specifies the plugin properties and initializes its classes.
 * @author Tim Borner
 */
class DiffView :
        public QGenericPlugin,
        public voxie::plugin::IUICommandPlugin,
        public voxie::plugin::IVisualizerPlugin
{
    Q_OBJECT
    Q_INTERFACES(voxie::plugin::IUICommandPlugin)
    Q_INTERFACES(voxie::plugin::IVisualizerPlugin)
#if QT_VERSION >= 0x050000
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QGenericPluginFactoryInterface" FILE "DiffView.json")
#endif // QT_VERSION >= 0x050000

public:
    DiffView(QObject *parent = 0);

    virtual QObject *create ( const QString & key, const QString & specification ) override;

    virtual QVector<QAction*> uiCommands() override;

    virtual QVector<voxie::plugin::MetaVisualizer*> visualizers() override;
};

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
