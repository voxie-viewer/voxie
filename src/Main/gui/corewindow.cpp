#include "corewindow.hpp"

#include <Main/dbusproxies.hpp>
#include <Main/directorymanager.hpp>
#include <Main/root.hpp>

#include <Main/gui/aboutdialogwindow.hpp>
#include <Main/gui/buttonlabel.hpp>
#include <Main/gui/dataselectiondialog.hpp>
#include <Main/gui/datasetview.hpp>
#include <Main/gui/objecttree.hpp>
#include <Main/gui/pluginmanagerwindow.hpp>
#include <Main/gui/preferenceswindow.hpp>
#include <Main/gui/scriptconsole.hpp>
#include <Main/gui/sliceview.hpp>
#include <Main/gui/vscrollarea.hpp>
#include <Main/gui/sidepanel.hpp>

#include <Main/io/load.hpp>

#include <Voxie/plugin/voxieplugin.hpp>

#include <Voxie/scripting/scriptingexception.hpp>

#include <Voxie/visualization/visualizer.hpp>

#include <functional>

#include <assert.h>

#include <QtCore/QDebug>
#include <QtCore/QProcess>
#include <QtCore/QSettings>
#include <QtCore/QUrl>

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>

#include <QtGui/QDesktopServices>
#include <QtGui/QWindow>
#include <QtGui/QScreen>

#include <QtWidgets/QDockWidget>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMdiSubWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QSplitter>

using namespace voxie::io;
using namespace voxie::gui;
using namespace voxie::data;
using namespace voxie::plugin;
using namespace voxie::visualization;

ActiveVisualizerProviderImpl::~ActiveVisualizerProviderImpl() {}

voxie::visualization::Visualizer* ActiveVisualizerProviderImpl::activeVisualizer() const {
    return win->getActiveVisualizer(); 
}

GuiDBusObject::GuiDBusObject (CoreWindow* window) : ScriptableObject("Gui", window, true, true), window (window) {
  // https://bugreports.qt.io/browse/QTBUG-48008
  // https://randomguy3.wordpress.com/2010/09/07/the-magic-of-qtdbus-and-the-propertychanged-signal/
  connect (window, &CoreWindow::activeVisualizerChanged, [=](visualization::Visualizer* visualizer) {
      // http://dbus.freedesktop.org/doc/dbus-specification.html#standard-interfaces-properties
      // org.freedesktop.DBus.Properties.PropertiesChanged (STRING interface_name,
      //                                                    DICT<STRING,VARIANT> changed_properties,
      //                                                    ARRAY<STRING> invalidated_properties);
      QString propertyName = "ActiveVisualizer";
      QDBusMessage signal = QDBusMessage::createSignal(window->getGuiDBusObject()->getPath().path(),
                                                       "org.freedesktop.DBus.Properties",
                                                       "PropertiesChanged");
      signal << "de.uni_stuttgart.Voxie.Gui";
      QVariantMap changedProps;
      changedProps.insert("ActiveVisualizer", qVariantFromValue(voxie::scripting::ScriptableObject::getPath(visualizer)));
      signal << changedProps;
      signal << QStringList();
      QDBusConnection::sessionBus().send(signal);
    });
}
GuiDBusObject::~GuiDBusObject () {
}

QDBusObjectPath GuiDBusObject::getActiveVisualizer()
{
  return voxie::scripting::ScriptableObject::getPath(window->getActiveVisualizer());
}

void GuiDBusObject::RaiseWindow(const QMap<QString, QVariant>& options) {
    try {
        voxie::scripting::ScriptableObject::checkOptions(options);
        window->activateWindow();
        window->raise();
    } catch (voxie::scripting::ScriptingException& e) {
        e.handle(this);
    }
}

qulonglong GuiDBusObject::GetMainWindowID() {
    return window->winId();
}

CoreWindow::CoreWindow(voxie::Root* root, QWidget *parent) :
    QMainWindow(parent),
    visualizers(),
    activeVisualizer(nullptr),
    isBeginDestroyed(false),
    activeVisualizerProvider(this)
{
    connect(this, &CoreWindow::activeVisualizerChanged, this, [this] (voxie::visualization::Visualizer* current) { emit activeVisualizerProvider.activeVisualizerChanged(current); });
    this->guiDBusObject = new GuiDBusObject (this);
    this->initMenu();
    this->initStatusBar();
    this->initWidgets(root);

    this->setWindowIcon(QIcon(":/icons-voxie/voxel-data-32.png"));
}
CoreWindow::~CoreWindow() {
    isBeginDestroyed = true;
}

void CoreWindow::insertPlugin(VoxiePlugin *plugin)
{
    if(plugin->uiCommands().size() > 0)
    {
        QMenu *pluginEntry = this->pluginsMenu->addMenu(plugin->name());
        // Insert UI Actions
        QList<QAction*> pluginActions;
        for(QAction *action : plugin->uiCommands())
        {
            pluginActions.append(action);
        }
        pluginEntry->addActions(pluginActions);
    }

    // Insert visualizers
    for(MetaVisualizer *metaVis : plugin->visualizers())
    {
        QMenu *menu = this->visualizerMenus[metaVis->type()];
        if(menu == nullptr)
        {
            menu = this->visualizerMenus[voxie::plugin::vtMiscellaneous];
            assert(menu);
        }

        QAction *action = menu->addAction(metaVis->name());
        connect(action, &QAction::triggered, [=]() -> void
        {
            Range sliceCount = metaVis->requiredSliceCount();
            Range voxelCount = metaVis->requiredDataSetCount();

            QVector<Slice*> slices;
            QVector<DataSet*> dataSets;

            if((sliceCount.isNull() == false) || (voxelCount.isNull() == false))
            {
                DataSelectionDialog dialog(sliceCount, voxelCount, this);
                if(dialog.exec() != QDialog::Accepted)
                {
                    return;
                }

                slices = dialog.slices();
                dataSets = dialog.dataSets();
            }

            try {
                metaVis->create(dataSets, slices);
            } catch (voxie::scripting::ScriptingException& e) {
                QMessageBox(QMessageBox::Critical, this->windowTitle(), QString("Failed to create visualizer: %1").arg(e.message()), QMessageBox::Ok, this).exec();
                return;
            }
        });
    }

    for(Importer *importer : plugin->importers())
    {
        QAction *action = importerMenu->addAction(importer->name());
        connect(action, &QAction::triggered, [=]() -> void
        {
            try {
                importer->import();
            } catch (voxie::scripting::ScriptingException& e) {
                errorMessage(e.message());
            }
        });
    }
}

void CoreWindow::addVisualizer(Visualizer *visualizer)
{
    if(visualizer == nullptr)
    {
        return;
    }

    VisualizerContainer *container = new VisualizerContainer(this->mdiArea, visualizer);

    this->visualizers.append(container);
    connect(container, &QObject::destroyed, this, [=]()
    {
        if (isBeginDestroyed)
            return;
        this->visualizers.removeAll(container);
    });

    QAction *windowEntry = this->windowMenu->addAction(visualizer->icon(), container->windowTitle());
    connect(windowEntry, &QAction::triggered, container, &VisualizerContainer::activate);
    connect(container, &QWidget::destroyed, windowEntry, &QAction::deleteLater);
    connect(visualizer, &QWidget::destroyed, container, &VisualizerContainer::closeWindow);

    QMetaObject::Connection connection = connect(container, &VisualizerContainer::sidePanelVisiblityChanged, [=](bool visible)
    {
        if(visible == false) return;
        if (this->activeVisualizer != visualizer) {
          this->activeVisualizer = visualizer;
          emit activeVisualizerChanged (visualizer);
        }
        this->mdiArea->setActiveSubWindow(container->window);
    });
    connect(container, &QObject::destroyed, this, [=]() {
        if (isBeginDestroyed)
            return;
        if (this->activeVisualizer == visualizer) {
            this->activeVisualizer = nullptr;
            emit activeVisualizerChanged (nullptr);
        }
        disconnect(connection);
      });

    QVector<QWidget*> sections = visualizer->dynamicSections();
    for(QWidget *section : sections)
        visualizer->addPropertySection(section);
    visualizer->mainView()->setFocus();

    this->activeVisualizer = visualizer;
    emit activeVisualizerChanged (visualizer);
}

Visualizer* CoreWindow::getActiveVisualizer() {
  return activeVisualizer;
}

void CoreWindow::initMenu()
{
    QMenuBar *menu = new QMenuBar(this);

    QMenu *fileMenu = menu->addMenu("&File");
    QMenu *visualizerMenu = menu->addMenu("&Visualizer");
    this->pluginsMenu = menu->addMenu("Plu&gins");
    this->scriptsMenu = menu->addMenu("&Scripts");
    {
        connect(scriptsMenu, &QMenu::aboutToShow, this, &CoreWindow::populateScriptsMenu);
    }
    //this->sectionMenu = menu->addMenu("Se&ctions");
    this->windowMenu = menu->addMenu("&Window");
    QMenu *helpMenu = menu->addMenu("&Help");

    QAction *openAction = fileMenu->addAction(QIcon(":/icons/blue-folder-horizontal-open.png"), "&Open…");
    openAction->setShortcut(QKeySequence("Ctrl+O"));
    connect(openAction, &QAction::triggered, this, &CoreWindow::loadFile);
    this->importerMenu = fileMenu->addMenu("&Import");
    fileMenu->addSeparator();
    QAction *preferencesAction = fileMenu->addAction(QIcon(":/icons/gear.png"), "&Preferences…");
    connect(preferencesAction, &QAction::triggered, [this]() -> void
    {
        PreferencesWindow *preferences = new PreferencesWindow(this);
        preferences->setAttribute(Qt::WA_DeleteOnClose);
        preferences->exec();
    });
    QAction *quitAction = fileMenu->addAction(QIcon(":/icons/cross.png"), "E&xit");
    quitAction->setShortcut(QKeySequence("Ctrl+Q"));
    connect(quitAction, &QAction::triggered, this, &QMainWindow::close);

    this->visualizerMenus.insert(voxie::plugin::vt2D, visualizerMenu->addMenu(QIcon(":/icons/layers.png"), "&2D"));
    this->visualizerMenus.insert(voxie::plugin::vt3D, visualizerMenu->addMenu(QIcon(":/icons/spectacle-3d.png"), "&3D"));
    this->visualizerMenus.insert(voxie::plugin::vtAnalytic, visualizerMenu->addMenu(QIcon(":/icons/flask.png"), "&Analytic"));
    this->visualizerMenus.insert(voxie::plugin::vtMiscellaneous, visualizerMenu->addMenu(QIcon(":/icons/equalizer.png"), "&Miscellaneous"));

    QAction *pluginManagerAction = this->pluginsMenu->addAction(QIcon(":/icons/plug.png"), "&Plugin Manager…");
    connect(pluginManagerAction, &QAction::triggered, [this]() -> void
    {
        PluginManagerWindow* pluginmanager = new PluginManagerWindow(this);
        pluginmanager->setAttribute(Qt::WA_DeleteOnClose);
        pluginmanager->exec();
    });
    this->pluginsMenu->addSeparator();

    QAction *scriptConsoleAction = this->scriptsMenu->addAction(QIcon(":/icons/application-terminal.png"), "&Script Console…");
    scriptConsoleAction->setShortcut(QKeySequence("Ctrl+Shift+S"));
    connect(scriptConsoleAction, &QAction::triggered, [this]() -> void
    {
        ScriptConsole *console = new ScriptConsole(this);
        console->setAttribute(Qt::WA_DeleteOnClose);
        console->show();
    });
    this->scriptsMenu->addSeparator();

    // this->sectionMenu

    QAction *cascadeAction = this->windowMenu->addAction(QIcon(":/icons/applications-blue.png"), "&Cascade");
    connect(cascadeAction, &QAction::triggered, [=]() -> void
    {
        this->mdiArea->cascadeSubWindows();
    });
    QAction *tileAction = this->windowMenu->addAction(QIcon(":/icons/application-tile.png"), "&Tile");
    connect(tileAction, &QAction::triggered, [=]() -> void
    {
        this->mdiArea->tileSubWindows();
    });
    QAction *fillAction = this->windowMenu->addAction(QIcon(":/icons/application-blue.png"), "&Fill");
    connect(fillAction, &QAction::triggered, [=]() -> void
    {
        QMdiSubWindow *subwindow = this->mdiArea->currentSubWindow();
        if(subwindow == nullptr)
        {
            return;
        }
        subwindow->setWindowState (subwindow->windowState() | Qt::WindowMaximized);
    });
    QAction *toggleTabbedAction = this->windowMenu->addAction(QIcon(":/icons/application-dock-tab.png"), "Ta&bbed");
    toggleTabbedAction->setCheckable(true);
    connect(toggleTabbedAction, &QAction::toggled, [=]() -> void
    {
        bool checked = toggleTabbedAction->isChecked();
        cascadeAction->setEnabled(checked == false);
        tileAction->setEnabled(checked == false);
        fillAction->setEnabled(checked == false);
        if(checked)
        {
            this->mdiArea->setViewMode(QMdiArea::TabbedView);
        }
        else
        {
            this->mdiArea->setViewMode(QMdiArea::SubWindowView);
        }
    });
    this->windowMenu->addSeparator();

    QAction *manualAction = helpMenu->addAction(QIcon(":/icons/book-question.png"), "&Manual…");
    connect(manualAction, &QAction::triggered, [this]() -> void
    {
        QString manualFile = QProcessEnvironment::systemEnvironment().value("VOXIE_MANUAL_FILE", "");
        if (manualFile == "") {
            manualFile = QCoreApplication::applicationDirPath() + "/manual.pdf";
            if (!QFile::exists (manualFile))
                manualFile = "";
        }
        if (manualFile == "")
            QMessageBox(QMessageBox::Warning, this->windowTitle(), "Manual file not found.", QMessageBox::Ok, this).exec();
        else
            QDesktopServices::openUrl(QUrl::fromLocalFile(manualFile));
    });
    QAction *homepageAction = helpMenu->addAction("&Homepage…");
    connect(homepageAction, &QAction::triggered, []() -> void
    {
        QDesktopServices::openUrl(QUrl("https://github.com/voxie-viewer/voxie", QUrl::TolerantMode));
    });
    QAction *aboutAction = helpMenu->addAction(QIcon(":/icons/information.png"), "&About…");
    connect(aboutAction, &QAction::triggered, [this]() -> void
    {
        AboutDialogWindow* aboutwindow = new AboutDialogWindow(this);
        aboutwindow->setAttribute(Qt::WA_DeleteOnClose);
        aboutwindow->exec();
    });

    this->setMenuBar(menu);
}

void CoreWindow::initStatusBar()
{
    QStatusBar *statusBar = new QStatusBar(this);

    /*QLabel *label = new QLabel(statusBar);
    label->setText("Voxie is alive!");
    label->setAlignment(Qt::AlignVCenter);
    statusBar->addWidget(label);*/

    this->setStatusBar(statusBar);
}

void CoreWindow::initWidgets(voxie::Root* root)
{
    bool sidePanelLeft = true;

    //auto container = new QWidget();
    auto container = new QSplitter(Qt::Horizontal);
    {
        /*
        QHBoxLayout *layout = new QHBoxLayout();
        layout->setMargin(0);
        layout->setSpacing(0);
        */
        {
            this->mdiArea = new QMdiArea(this);
            this->mdiArea->setTabsClosable(true);
            //layout->addWidget(this->mdiArea);

            sidePanel = new SidePanel(root, this);
            connect(sidePanel, &SidePanel::openFile, this, &CoreWindow::loadFile);

            if (sidePanelLeft) {
                container->addWidget(sidePanel);
                container->addWidget(this->mdiArea);
                container->setCollapsible(1, false);
            } else {
                container->addWidget(this->mdiArea);
                container->setCollapsible(0, false);
                container->addWidget(sidePanel);
            }
        }
        //container->setLayout(layout);
    }
    this->setCentralWidget(container);
}

void CoreWindow::errorMessage(QString msg)
{
    QMessageBox(
        QMessageBox::Warning,
        this->windowTitle(),
        msg,
        QMessageBox::Ok,
        this).exec();
}

void CoreWindow::openDataSet(DataSet *dataSet)
{
    dataSet->addPropertySection(new DataSetView(dataSet));
}

void CoreWindow::openSlice(voxie::data::Slice *slice)
{
    slice->addPropertySection(new SliceView(slice));
}

void CoreWindow::populateScriptsMenu()
{
    QList<QAction*> actions = this->scriptsMenu->actions();
    for(int i = 2; i < actions.size(); i++)
    {
        this->scriptsMenu->removeAction(actions.at(i));
    }

    for (auto scriptDirectory : Root::instance()->directoryManager()->scriptPath()) {
        //qDebug() << "Searching for scripts in" << scriptDirectory;
        QDir scriptDir = QDir(scriptDirectory);
        QStringList scripts = scriptDir.entryList(QStringList("*.js"), QDir::Files | QDir::Readable);
        for(QString script : scripts) {
            if (script.endsWith("~"))
                continue;
            QString scriptFile = scriptDirectory + "/" + script;

            if (QFileInfo (scriptFile).isExecutable ())
                continue;

            QString baseName = scriptFile;
            if (baseName.endsWith(".exe"))
                baseName = scriptFile.left(scriptFile.length() - 4);
            if (QFile::exists (baseName + ".conf"))
                continue;
            
            QAction *action = this->scriptsMenu->addAction(QIcon(":/icons/script-attribute-j.png"), script);
            connect(action, &QAction::triggered, [scriptFile]() -> void
                    {
                        Root::instance()->execFile(scriptFile);
                    });
        }
    }
    this->scriptsMenu->addSeparator();

    for (auto scriptDirectory : Root::instance()->directoryManager()->scriptPath()) {
        QDir scriptDir = QDir(scriptDirectory);
        QStringList scripts = scriptDir.entryList(QStringList("*"), QDir::Files | QDir::Readable);
        for(QString script : scripts) {
            if (script.endsWith("~"))
                continue;
            QString scriptFile = scriptDirectory + "/" + script;

            if (!QFileInfo (scriptFile).isExecutable ())
                continue;

            QString baseName = scriptFile;
            if (baseName.endsWith(".exe"))
                baseName = scriptFile.left(scriptFile.length() - 4);
            if (QFile::exists (baseName + ".conf"))
                continue;

            QAction *action = this->scriptsMenu->addAction(QIcon(":/icons/script-attribute-e.png"), script);
            connect(action, &QAction::triggered, [this, scriptFile]() -> void {
                    startScript(scriptFile);
                });
        }
    }
    this->scriptsMenu->addSeparator();

    {
        QStringList fileTypes;

        Root::instance()->settings()->beginGroup("scripting");
        int size = Root::instance()->settings()->beginReadArray("externals");
        for (int i = 0; i < size; ++i) {
            Root::instance()->settings()->setArrayIndex(i);
            fileTypes.append(Root::instance()->settings()->value("extension").toString().split(';'));
        }
        Root::instance()->settings()->endArray();
        Root::instance()->settings()->endGroup();

        if(fileTypes.size() > 0) {
            for (auto scriptDirectory : Root::instance()->directoryManager()->scriptPath()) {
                QDir scriptDir = QDir(scriptDirectory);
                QStringList externalScripts = scriptDir.entryList(fileTypes, QDir::Files | QDir::Readable);
                for(QString script : externalScripts) {
                    if (script.endsWith("~"))
                        continue;
                    QString scriptFile = scriptDirectory + "/" + script;

                    if (QFileInfo (scriptFile).isExecutable ())
                        continue;

                    QString baseName = scriptFile;
                    if (baseName.endsWith(".exe"))
                        baseName = scriptFile.left(scriptFile.length() - 4);
                    if (QFile::exists (baseName + ".conf"))
                        continue;

                    QAction *action = this->scriptsMenu->addAction(QIcon(":/icons/script-attribute-p.png"), script);
                    connect(action, &QAction::triggered, [this, scriptFile]() -> void {
                            Root::instance()->settings()->beginGroup("scripting");
                            int size = Root::instance()->settings()->beginReadArray("externals");
                            for (int i = 0; i < size; ++i) {
                                Root::instance()->settings()->setArrayIndex(i);
                                QString executable = Root::instance()->settings()->value("executable").toString();
                                // Not used yet.
                                // QString arguments = Root::instance()->settings()->value("arguments").toString();

                                for(const QString &ext : Root::instance()->settings()->value("extension").toString().split(';')) {
                                    QRegExp regexp(ext, Qt::CaseInsensitive, QRegExp::WildcardUnix);
                                    if(regexp.exactMatch(scriptFile) == false)
                                        continue;

                                    startScript(scriptFile, &executable);
                                }
                            }
                            Root::instance()->settings()->endArray();
                            Root::instance()->settings()->endGroup();
                        });
                }
            }
        }
    }
}

QProcess* CoreWindow::startScript(const QString& scriptFile, const QString* executable, const QStringList& arguments) {
    currentScriptExecId++;
    quint64 id = currentScriptExecId;

    if (!QDBusConnection::sessionBus().isConnected()) {
        QMessageBox(QMessageBox::Critical, this->windowTitle(), QString("Cannot run external script because DBus is not available"), QMessageBox::Ok, this).exec();
        return nullptr;
    }

    QFileInfo fileInfo(scriptFile);

    QStringList args;
    if (executable)
        args.append(scriptFile);
    //args.append("--voxie-bus-address=" + ...);
    args.append("--voxie-bus-name=" + QDBusConnection::sessionBus().baseService());
    for (const auto& arg : arguments)
        args << arg;
    QProcess *process = new QProcess();
    process->setProcessChannelMode(QProcess::MergedChannels);
    if (executable)
        process->setProgram(*executable);
    else
        process->setProgram(scriptFile);
    process->setArguments(args);
    auto isStarted = createQSharedPointer<bool>();
    connect<void(QProcess::*)(int), std::function<void(int)>>(process, &QProcess::finished, this, std::function<void(int)>([process, id](int exitCode) -> void
        {
            Root::instance()->log(QString("Script %1 finished with exit status %2 / exit code %3").arg(id).arg(process->exitStatus()).arg(exitCode));
            process->deleteLater();
        }));
    /*
    connect(process, &QProcess::errorOccurred, this, [process, id](QProcess::ProcessError error) { 
            Root::instance()->log(QString("Error occurred for script %1: %2").arg(id).arg(error));
            process->deleteLater();
        });
    */
    connect(process, &QProcess::started, this, [isStarted]() { *isStarted = true; });
    connect(process, &QProcess::stateChanged, this, [process, id, isStarted](QProcess::ProcessState newState) { 
            //Root::instance()->log(QString("State change occurred for script %1: %2").arg(id).arg(newState));
            if (newState == QProcess::NotRunning && !*isStarted) {
                Root::instance()->log(QString("Error occurred for script %1: %2").arg(id).arg(process->error()));
                process->deleteLater();
            }
        });
    auto buffer = createQSharedPointer<QString>();
    connect(process, &QProcess::readyRead, this, [process, id, buffer]() -> void {
            int pos = 0;
            QString data = QString(process->readAll());
            for (;;) {
                int index = data.indexOf('\n', pos);
                if (index == -1)
                    break;
                Root::instance()->log(QString("Script %1: %2%3").arg(id).arg(*buffer).arg(data.mid(pos, index - pos)));
                buffer->clear();
                pos = index + 1;
            }
            *buffer += data.mid(pos);
        });
    connect(process, &QProcess::readChannelFinished, this, [process, id, buffer]() -> void {
            int pos = 0;
            QString data = QString(process->readAll());
            for (;;) {
                int index = data.indexOf('\n', pos);
                if (index == -1)
                    break;
                Root::instance()->log(QString("Script %1: %2%3").arg(id).arg(*buffer).arg(data.mid(pos, index - pos)));
                buffer->clear();
                pos = index + 1;
            }
            *buffer += data.mid(pos);
            if (*buffer != "") {
                Root::instance()->log(QString("Script %1: %2").arg(id).arg(*buffer));
                buffer->clear();
            }
        });
    process->start();
    //qDebug() << "started";
    
    Root::instance()->log(QString("Started execution of %1 with ID %2").arg(scriptFile).arg(id));

    return process;
}

void CoreWindow::loadFile() {
    const auto& loaders = Load::getLoaders(Root::instance());

    QStringList filters;

    QString supportedFilter;
    supportedFilter += "All supported files (";
    int i = 0;
    for (auto loader : *loaders) {
        for (auto pattern : loader->filter().patterns()){
            if (i != 0)
                supportedFilter += " ";
            supportedFilter += pattern;
            i++;
        }
    }
    supportedFilter += ")";
    filters << supportedFilter;

    QMap<QString, QSharedPointer<voxie::io::Loader>> map;
    for (auto loader : *loaders) {
        const QString& filterString = loader->filter().filterString();
        if (map.contains(filterString)) {
            qWarning() << "Got multiple loaders with filter string" << filterString;
        } else {
            filters << filterString;
            map[filterString] = loader;
        }
    }

    QFileDialog dialog(this, "Select file to load");
    dialog.setNameFilters(filters);
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);
    if(dialog.exec() != QDialog::Accepted)
        return;

    LoadOperation* op;
    if (dialog.selectedNameFilter() == supportedFilter) {
        op = Load::openFile(Root::instance(), dialog.selectedFiles().first());
    } else {
        auto loader = map[dialog.selectedNameFilter()];
        if(!loader) {
            QMessageBox(
                        QMessageBox::Critical,
                        this->windowTitle(),
                        "Somehow the selected file type disappeared inbetween the time the dialog was opened.",
                        QMessageBox::Ok,
                        this).exec();
            return;
        }
        op = Load::openFile(Root::instance(), loader, dialog.selectedFiles().first());
    }
    connect(op, &LoadOperation::loadAborted, this, [this] (QSharedPointer<voxie::scripting::ScriptingException> e) {
            errorMessage(e->message());
        });
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
