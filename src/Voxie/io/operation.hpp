#pragma once

#include <Voxie/scripting/scriptingexception.hpp>

#include <QtCore/QAtomicInt>
#include <QtCore/QObject>

namespace voxie { namespace io {

class VOXIECORESHARED_EXPORT OperationCancelledException : public voxie::scripting::ScriptingException {
 public:
    OperationCancelledException();
    virtual ~OperationCancelledException();
};

class VOXIECORESHARED_EXPORT Operation : public QObject {
    Q_OBJECT

    QAtomicInt cancelled;

public:
    Operation(QObject* parent = nullptr);
    ~Operation();

    void updateProgress(float progress);

    void cancel();
    bool isCancelled() { return cancelled.load() != 0; }
    void throwIfCancelled() {
        if (isCancelled())
            throw OperationCancelledException();
    }

signals:
    void progressChanged(float progress);
};

} }

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
