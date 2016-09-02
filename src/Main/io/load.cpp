#include "load.hpp"

// QDBusConnection should be included as early as possible: https://bugreports.qt.io/browse/QTBUG-48351
#include <QtDBus/QDBusInterface>

#include <Voxie/io/loader.hpp>
#include <Voxie/io/operation.hpp>

#include <Voxie/scripting/scriptingexception.hpp>

#include <Main/root.hpp>
#include <Main/directorymanager.hpp>

#include <Main/script/externaloperation.hpp>

#include <QtCore/QDir>

using namespace voxie::io;
using namespace voxie::scripting;

namespace {
    class ScriptLoader : public Loader {
        voxie::Root* root;
        QString executable;

    public:
        explicit ScriptLoader(voxie::Root* root, Filter filter, QObject *parent, const QString& executable) : Loader(filter, parent), root(root), executable(executable) {
        }
        virtual ~ScriptLoader() {
        }

    protected:
        // throws ScriptingException
        QSharedPointer<voxie::data::VoxelData> load(const QSharedPointer<Operation>& op, const QString &fileName) override {
            QSharedPointer<ExternalOperationLoad> exOp;
            QSharedPointer<QSharedPointer<ExternalOperation>> initialRef;
            QProcess* process;
            { // Execute on main thread
                auto main = QCoreApplication::instance();
                QObject obj;
                connect(&obj, &QObject::destroyed, main, [&] {
                        exOp = QSharedPointer<ExternalOperationLoad>(new ExternalOperationLoad(op), [](QObject* obj) { obj->deleteLater(); });
                        registerObject(exOp);
                        initialRef = createQSharedPointer<QSharedPointer<ExternalOperation> >();
                        *initialRef = exOp;
                        exOp->initialReference = initialRef;

                        QStringList args;
                        args << "--voxie-action=Load";
                        args << "--voxie-operation=" + exOp->getPath().path();
                        args << "--voxie-load-filename=" + fileName;
                        process = root->mainWindow()->startScript(executable, nullptr, args);
                    }, QThread::currentThread() == main->thread() ? Qt::DirectConnection : Qt::BlockingQueuedConnection);
            } // this will destroy obj and trigger the lambda function on the main thread

            // When loading using a script the loader thread will simply wait
            // until the load has finished
            auto loop = createQSharedPointer<QEventLoop>();
            auto exitWithoutClaim = createQSharedPointer<bool>();
            *exitWithoutClaim = false;
            connect(process, &QObject::destroyed, this, [initialRef, exitWithoutClaim] () {
                    if (*initialRef)
                        *exitWithoutClaim = true;
                    initialRef->reset();
                });
            connect(exOp.data(), &QObject::destroyed, this, [loop] () {
                    loop->exit();
                });

            auto result = createQSharedPointer<QSharedPointer<voxie::data::VoxelData>>();
            auto error = createQSharedPointer<QSharedPointer<ScriptingException>>();
            connect(exOp.data(), &ExternalOperationLoad::finished, this, [result, loop] (const QSharedPointer<voxie::data::VoxelData>& data) {
                    *result = data;
                    loop->exit();
                });
            connect(exOp.data(), &ExternalOperation::error, this, [error, loop] (const ScriptingException& err) {
                    *error = createQSharedPointer<ScriptingException>(err);
                    loop->exit();
                });

            exOp.reset();
            loop->exec();

            //if (*initialRef)
            if (*exitWithoutClaim)
                throw ScriptingException("de.uni_stuttgart.Voxie.LoadScriptErrorNoClaim", "Script failed to claim the loading operation");

            if (op->isCancelled())
                throw OperationCancelledException();

            if (*error) {
                //throw **error;
                /*
                if ((*error)->name() == "de.uni_stuttgart.Voxie.OperationCancelled")
                    throw OperationCancelledException();
                */
                throw ScriptingException("de.uni_stuttgart.Voxie.LoadScriptError", (*error)->name() + ": " + (*error)->message());
            }

            if (*result)
                return *result;

            // Script called ClaimOperation() but neither Finished() nor SetError()
            throw ScriptingException("de.uni_stuttgart.Voxie.Error", "Script failed to produce a VoxelData object");
        }
    };
}

QSharedPointer<QList<QSharedPointer<Loader>>> Load::getLoaders(voxie::Root* root) {
    auto result = createQSharedPointer<QList<QSharedPointer<Loader>>>();

    for (auto loader : root->getPluginLoaders())
        result->push_back(loader);

    for (auto scriptDirectory : root->directoryManager()->scriptPath()) {
        QDir scriptDir = QDir(scriptDirectory);
        QStringList configs = scriptDir.entryList(QStringList("*.conf"), QDir::Files | QDir::Readable);
        for (QString config : configs) {
            QString configFile = scriptDirectory + "/" + config;
            if (QFileInfo (configFile).isExecutable ())
                continue;
            QSettings conf(configFile, QSettings::IniFormat);
            if (conf.value("Voxie/Type") != "Loader")
                continue;
            //qDebug() << configFile;
            QString description = conf.value("Voxie/Description", "").toString();
            QString patterns = conf.value("Voxie/Patterns", "").toString();
            QStringList patternList = patterns.split(" ", QString::SkipEmptyParts);
            Loader::Filter filter(description, patternList);
            QString executable = configFile.mid(0, configFile.length() - 5);
            QSharedPointer<Loader> loader(new ScriptLoader(root, filter, root, executable), [](QObject* obj) { obj->deleteLater(); });
            result->push_back(loader);
        }
    }

    qSort(result->begin(), result->end(), [] (const QSharedPointer<Loader>& l1, const QSharedPointer<Loader>& l2) { return l1->filter().filterString() < l2->filter().filterString(); });

    return result;
}

template <typename T>
static void enqueueOnThread(QObject* obj, T&& fun) {
   QObject srcObj;
   QObject::connect(&srcObj, &QObject::destroyed, obj, std::move(fun), Qt::QueuedConnection);
}

LoadOperation* Load::openFile(voxie::Root* root, const QSharedPointer<voxie::io::Loader>& loader, const QString& file) {
    //QSharedPointer<voxie::io::LoadOperation> operation(new voxie::io::LoadOperation(), [](QObject* obj) { obj->deleteLater(); });
    // Hack to workaround https://bugreports.qt.io/browse/QTBUG-54891 by
    // enqueuing the call to deleteLater()
    QSharedPointer<voxie::io::LoadOperation> operation(new voxie::io::LoadOperation(), [](QObject* obj) { enqueueOnThread(obj, [obj] { obj->deleteLater(); }); });

    operation->setDescription("Load " + file);
    root->addProgressBar(operation.data());

    auto worker = new LoadWorker(loader, operation, file);
    auto thread = new QThread();
    worker->moveToThread(thread);
    QObject::connect(thread, &QThread::started, worker, &LoadWorker::run);
    QObject::connect(worker, &LoadWorker::loadFinished, worker, &QThread::deleteLater);
    QObject::connect(worker, &LoadWorker::loadAborted, worker, &QThread::deleteLater);
    QObject::connect(worker, &QObject::destroyed, thread, &QThread::quit);
    QObject::connect(thread, &QThread::finished, thread, &QObject::deleteLater);

    // This must happen on the main thread to make sure that the caller can
    // register a callback for loadAborted() before this function gets called
    QObject::connect(worker, &LoadWorker::loadFinished, operation.data(), [operation, file, root] (QSharedPointer<voxie::data::VoxelData> data) {
            auto dataSet = new voxie::data::DataSet(data);
            dataSet->setFileInfo(QFileInfo(file));
            dataSet->setDisplayName(dataSet->getFileInfo().fileName());
            root->registerDataObject(dataSet);
            root->registerDataSet(dataSet);
            emit operation->loadFinished(dataSet);
        });

    // This must happen on the main thread to make sure that the caller can
    // register a callback for loadAborted() before this function gets called
    QObject::connect(worker, &LoadWorker::loadAborted, operation.data(), [operation, file] (QSharedPointer<voxie::scripting::ScriptingException> error) {
            emit operation->loadAborted(error);
        });

    thread->start();
    
    // Because the QSharedPointer uses deleteLater() as deleter this object will
    // always be valid at least until the code returns to the main loop
    return operation.data();
}
LoadOperation* Load::openFile(voxie::Root* root, const QString& file) {
    const auto& loaders = getLoaders(root);
    for (const auto& loader : *loaders) {
        if (loader->filter().matches(file)) {
            return openFile(root, loader, file);
        }
    }
    return LoadOperation::createErrorOperation(ScriptingException("de.uni_stuttgart.Voxie.NoLoaderFound", "No loader found for file " + file));
}

LoadOperation::LoadOperation() {
}
LoadOperation::~LoadOperation() {
}

LoadOperation* LoadOperation::createErrorOperation(const voxie::scripting::ScriptingException& error) {
    QObject obj;
    auto op = new LoadOperation();
    QObject::connect(&obj, &QObject::destroyed, op, [error, op] {
            // This will be executed once the code returns to the main loop
            emit op->loadAborted(createQSharedPointer<voxie::scripting::ScriptingException>(error));
            op->deleteLater();
        }, Qt::QueuedConnection);
    return op;
}

LoadWorker::LoadWorker(const QSharedPointer<voxie::io::Loader>& loader, const QSharedPointer<voxie::io::LoadOperation>& op, const QString& filename) : loader(loader), op(op), filename(filename) {
}
LoadWorker::~LoadWorker() {
}

void LoadWorker::run() {
    try {
        auto data = loader->load(op, filename);
        emit loadFinished(data);
    } catch (ScriptingException& e) {
        emit loadAborted(createQSharedPointer<voxie::scripting::ScriptingException>(e));
    }
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
