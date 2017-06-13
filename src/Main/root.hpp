#pragma once

#include <Main/scriptwrapper.hpp>

#include <Main/gui/corewindow.hpp>

#include <Voxie/ivoxie.hpp>

#include <Voxie/scripting/scriptingcontainer.hpp>

#include <QtCore/QCommandLineParser>
#include <QtCore/QSettings>

#include <QtDBus/QDBusAbstractInterface>
#include <QtDBus/QDBusPendingReply>

#include <QtScript/QScriptEngine>

#include <QtWidgets/QWidget>

int main(int argc, char *argv[]);

namespace voxie
{
namespace visualization
{
    class Visualizer;
}

class VoxieInstance;
class DirectoryManager;

/**
 * @brief The voxie root object.
 */
class Root :
        public QObject,
        public IVoxie
{
    Q_OBJECT
    friend int ::main(int argc, char *argv[]);
private:
    gui::CoreWindow *coreWindow;
    QScriptEngine jsEngine;
    ScriptWrapper scriptWrapper;

    /**
     * @brief Container for all plugin objects
     */
    QObject *pluginContainer;

    /**
     * @brief Container for all data sets.
     */
    QObject *dataSetContainer;

    VoxieInstance* voxieInstance;

    DirectoryManager* directoryManager_;

    QSettings* settings_;

    bool disableOpenGL_;
    bool disableOpenCL_;

    QList<QSharedPointer<voxie::io::Loader>> pluginLoaders;

    QList<voxie::data::DataObject*> dataObjects_;

    explicit Root(QObject *parent = 0);

    ~Root();

    /**
     * @brief Starts voxie within the given QApplication
     * @param app
     * @return App exit code.
     */
    static int startVoxie(QApplication &app, QCommandLineParser& parser);

    void loadPlugins(QString pluginDirectory);

    bool initOpenCL();

public:
    /**
     * @brief Gets the current voxie instance.
     * @return
     */
    static Root *instance();

    /**
     * @brief Return the DirectoryManager instance.
     */
    DirectoryManager* directoryManager() const { return directoryManager_; }

    /**
     * @brief Return the QSettings instance.
     */
    QSettings* settings() const { return settings_; }

    /**
     * @brief Registers a visualizer.
     * @param visualizer The visualizer to be registered.
     *
     * Registers and shows a visualizer. Voxie takes ownership of the visualizer.
     */
    virtual void registerVisualizer(visualization::Visualizer *visualizer) override;

    /**
     * @brief Registers a side panel section.
     * @param section The section to be registered.
     * @param closeable If true, the section can be closed by the user.
     *
     * Registers and shows a section. Voxie takes ownership of the section.
     */
    virtual void registerSection(QWidget *section, bool closeable = false) override;

    /**
     * @brief Registers a data set.
     * @param dataSet
     *
     * Registers a data set, creates a data set section. Voxie takes ownership of the voxel data.
     */
    void registerDataSet(voxie::data::DataSet *dataSet);

    /**
     * @brief Registers a slice.
     * @param slice
     *
     * Registers a slice, creates a slice section. Voxie does not take ownership of the slice as
     * slices are children of DataSet.
     */
    void registerSlice(voxie::data::Slice *slice);

    void registerDataObject(voxie::data::DataObject* obj) override;

    /**
     * @brief Gets the global script engine.
     * @return
     */
    QScriptEngine& scriptEngine();

    /**
     * @brief Gets the main window.
     * @return
     */
    gui::CoreWindow *mainWindow() const override { return this->coreWindow; }

    /**
     * @brief Gets a vector with all open data sets.
     * @return
     */
    virtual QVector<voxie::data::DataSet*> dataSets() const override;

    /**
     * @brief Gets a vector with all loaded plugins.
     * @return
     */
    virtual QVector<voxie::plugin::VoxiePlugin*> plugins() const override;

    virtual bool disableOpenGL() const { return disableOpenGL_; }
    virtual bool disableOpenCL() const { return disableOpenCL_; }

    static QVector<QString> getBufferedMessages();

    ActiveVisualizerProvider* activeVisualizerProvider() const override { return &mainWindow()->activeVisualizerProvider; }

    const QList<voxie::data::DataObject*>& dataObjects() const { return dataObjects_; }

    void addProgressBar(voxie::io::Operation* operation) override;

    QObject* createLoaderAdaptor(voxie::io::Loader* loader) override;

    const QList<QSharedPointer<voxie::io::Loader>>& getPluginLoaders() { return pluginLoaders; }

public:
    /**
     * @brief Quits voxie.
     */
    void quit();

    /**
     * @brief Logs a value.
     * @param value Any script value that can be logged.
     */
    void log(QScriptValue value);

    /**
     * @brief Executes a JavaScript file.
     * @param fileName The file to be executed.
     * @return True on success.
     */
    bool execFile(const QString &fileName);

    /**
     * @brief Executes a JavaScript snippet.
     * @param code The snippet to be executed.
     * @return True on success.
     */
    bool exec(const QString& code, const QString& codeToPrint, const std::function<void(const QString&)>& print);

    // throws ScriptingException
    voxie::plugin::VoxiePlugin* getPluginByName (const QString& name);

signals:
    void logEmitted(const QString &text);

    void dataObjectAdded(voxie::data::DataObject* obj);
    void dataObjectRemoved(voxie::data::DataObject* obj);
};

class VoxieInstance : public scripting::ScriptableObject, public QDBusContext {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "de.uni_stuttgart.Voxie.Voxie")

    Root* root;

public:
    VoxieInstance (Root* root);
    ~VoxieInstance ();

    Q_PROPERTY (QDBusObjectPath Gui READ gui)
    QDBusObjectPath gui ();

    Q_SCRIPTABLE QList<QDBusObjectPath> ListPlugins ();
    Q_SCRIPTABLE QDBusObjectPath GetPluginByName (const QString& name);
    Q_SCRIPTABLE QStringList ListPluginMemberTypes ();

    Q_SCRIPTABLE QList<QDBusObjectPath> ListDataSets ();

    Q_SCRIPTABLE QDBusObjectPath CreateClient (const QMap<QString, QVariant>& options);
    Q_SCRIPTABLE QDBusObjectPath CreateIndependentClient (const QMap<QString, QVariant>& options);
    Q_SCRIPTABLE bool DestroyClient (const QDBusObjectPath& client, const QMap<QString, QVariant>& options);

    Q_SCRIPTABLE QDBusObjectPath CreateImage (const QDBusObjectPath& client, const voxie::scripting::IntVector2& size, const QMap<QString, QVariant>& options);

    Q_SCRIPTABLE QDBusObjectPath CreateVoxelData (const QDBusObjectPath& client, const voxie::scripting::IntVector3& size, const QMap<QString, QVariant>& options);

    Q_SCRIPTABLE QDBusObjectPath CreateDataSet (const QString& name, const QDBusObjectPath& data, const QMap<QString, QVariant>& options);

    Q_SCRIPTABLE void Quit (const QMap<QString, QVariant>& options);

    Q_SCRIPTABLE QDBusVariant ExecuteQScriptCode (const QString& code, const QMap<QString, QVariant>& options);

    static QDBusObjectPath OpenFileImpl (Root* root, QDBusContext* context, const QString& interface_, const QString& member, const QString& file, const QSharedPointer<voxie::io::Loader>& loader, const QMap<QString, QVariant>& options);
    Q_SCRIPTABLE QDBusObjectPath OpenFile (const QString& file, const QMap<QString, QVariant>& options);
};

}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
