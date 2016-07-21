#pragma once

#include <Voxie/io/operation.hpp>

#include <QtCore/QSharedPointer>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QRunnable>

namespace voxie {
class Root;
namespace io {
class Loader;
}
namespace data {
class DataSet;
class VoxelData;
}
namespace scripting {
class ScriptingException;
}

namespace io {

class LoadOperation;

class Load {
    Load() = delete;

public:
    static QSharedPointer<QList<QSharedPointer<voxie::io::Loader>>> getLoaders(voxie::Root* root);

    static LoadOperation* openFile(voxie::Root* root, const QSharedPointer<voxie::io::Loader>& loader, const QString& file);
    static LoadOperation* openFile(voxie::Root* root, const QString& file);
};

class LoadOperation : public voxie::io::Operation {
    Q_OBJECT

public:
    explicit LoadOperation();
    ~LoadOperation();

    static LoadOperation* createErrorOperation(const voxie::scripting::ScriptingException& error);

signals:
    void loadFinished(voxie::data::DataSet* dataSet);
    void loadAborted(QSharedPointer<voxie::scripting::ScriptingException> error);
};

class LoadWorker : public QObject {
    Q_OBJECT

    QSharedPointer<voxie::io::Loader> loader;
    QSharedPointer<voxie::io::LoadOperation> op;
    QString filename;

public:
    explicit LoadWorker(const QSharedPointer<voxie::io::Loader>& loader, const QSharedPointer<voxie::io::LoadOperation>& op, const QString& filename);
    ~LoadWorker();

public slots:
    void run();

signals:
    void loadFinished(QSharedPointer<voxie::data::VoxelData> data);
    void loadAborted(QSharedPointer<voxie::scripting::ScriptingException> error);
};

}
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
