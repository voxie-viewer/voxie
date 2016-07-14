#pragma once

#include <Main/gui/visualizercontainer.hpp>

#include <Voxie/plugin/metavisualizer.hpp>
#include <Voxie/ivoxie.hpp>

#include <QtCore/QMap>
#include <QtCore/QProcess>

#include <QtDBus/QDBusAbstractAdaptor>
#include <QtDBus/QDBusAbstractInterface>
#include <QtDBus/QDBusObjectPath>
#include <QtDBus/QDBusPendingReply>

#include <QtWidgets/QDockWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMdiArea>
#include <QtWidgets/QVBoxLayout>

namespace voxie
{
class Root;
namespace visualization
{
class Visualizer;
}
namespace plugin
{
class VoxiePlugin;
}
namespace io
{
    class Loader;
};
namespace gui
{

class GuiDBusObject;

class CoreWindow;

class ActiveVisualizerProviderImpl : public ActiveVisualizerProvider {
    Q_OBJECT

    CoreWindow* win;

public:
    ActiveVisualizerProviderImpl(CoreWindow* win) : win(win) {}
    ~ActiveVisualizerProviderImpl();

    voxie::visualization::Visualizer* activeVisualizer() const override;
};

/**
 * @brief The main window of voxie.
 */
class CoreWindow :
        public QMainWindow
{
    Q_OBJECT

    friend class ::voxie::Root;
private:
    QMdiArea *mdiArea;
    QMenu *importerMenu;
    QMenu *sectionMenu;
    QMenu *windowMenu;
    QMenu *scriptsMenu;
    QMenu *pluginsMenu;

    QVBoxLayout *sidePanel;

    QMap<plugin::VisualizerType, QMenu*> visualizerMenus;

    QList<VisualizerContainer*> visualizers;

    visualization::Visualizer* activeVisualizer;

    GuiDBusObject* guiDBusObject;

    bool isBeginDestroyed;

    quint64 currentScriptExecId = 0;

    ActiveVisualizerProviderImpl activeVisualizerProvider;

signals:
    void activeVisualizerChanged(visualization::Visualizer* visualizer);

public:
    Q_PROPERTY (voxie::visualization::Visualizer* activeVisualizer READ getActiveVisualizer)

    voxie::visualization::Visualizer* getActiveVisualizer();

    GuiDBusObject* getGuiDBusObject() { return guiDBusObject; }

private:

    /**
     * @brief Initializes the main menu.
     */
    void initMenu();

    /**
     * @brief Initializes the status bar.
     */
    void initStatusBar();

    /**
     * @brief Initializes the central widget.
     */
    void initWidgets();

    /**
     * @brief Shows the file dialog
     */
    void loadFile();

    /**
     * @brief Shows a message box with the error message and a warning icon.
     * @param msg The message to be shown.
     */
    void errorMessage(QString msg);

    /**
     * @brief Opens the given data set.
     * @param dataSet
     */
    void openDataSet(voxie::data::DataSet *dataSet);

    /**
     * @brief Opens the given slice.
     * @param slice
     */
    void openSlice(voxie::data::Slice *slice);

    /**
     * @brief Populates the script menu with script files.
     */
    void populateScriptsMenu();

public:
    explicit CoreWindow(QWidget *parent = 0);
    ~CoreWindow();

    QProcess* startScript(const QString& scriptFile, const QString* executable = nullptr, const QStringList& arguments = QStringList());

    void insertPlugin(plugin::VoxiePlugin *plugin);

    /**
     * @brief Adds a visualizer to the window.
     * @param widget
     */
    void addVisualizer(visualization::Visualizer *visualizer);

    /**
     * @brief Adds a section to the window.
     * @param widget
     * @param closeable If true, the user can close the section.
     */
    QWidget *addSection(QWidget *widget, bool closeable = false);
};

class GuiDBusObject : public voxie::scripting::ScriptableObject, public QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "de.uni_stuttgart.Voxie.Gui")

    CoreWindow* window;

public:
    GuiDBusObject (CoreWindow* window);
    virtual ~GuiDBusObject ();

    Q_PROPERTY (QDBusObjectPath ActiveVisualizer READ getActiveVisualizer)
    QDBusObjectPath getActiveVisualizer();

    Q_SCRIPTABLE void RaiseWindow(const QMap<QString, QVariant>& options);

    // The Window ID of the main window, useful e.g. for using it as a transient
    // parent
    // Returns 0 if not supported.
    Q_SCRIPTABLE qulonglong GetMainWindowID();
};

}
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
