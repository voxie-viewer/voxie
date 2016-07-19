#pragma once

#include <Voxie/scripting/scriptingexception.hpp>

#include <QtCore/QAtomicInt>
#include <QtCore/QObject>
#include <QtCore/QMutex>

namespace voxie { namespace io {

class VOXIECORESHARED_EXPORT OperationCancelledException : public voxie::scripting::ScriptingException {
 public:
    OperationCancelledException();
    virtual ~OperationCancelledException();
};

class VOXIECORESHARED_EXPORT Operation : public QObject {
    Q_OBJECT

    QAtomicInt cancelledVal;

    QMutex progressMutex;
    bool progressUpdating = false;
    bool progressPending = false;
    float progressPendingValue;
    void emitProgressChanged();

    QString description_;

public:
    Operation(QObject* parent = nullptr);
    ~Operation();

    // thread-safe
    void updateProgress(float progress);

public slots:
    void cancel();

public:
    // thread-safe
    bool isCancelled() { return cancelledVal.load() != 0; }
    // thread-safe
    void throwIfCancelled() {
        if (isCancelled())
            throw OperationCancelledException();
    }

    void setDescription(const QString& desc);
    const QString& description();

signals:
    void progressChanged(float progress);
    void descriptionChanged(const QString& description);
    void cancelled();
};

} }

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
