#include "operation.hpp"

#include <QtCore/QDebug>
#include <QtCore/QThread>

using namespace voxie::io;

OperationCancelledException::OperationCancelledException() : voxie::scripting::ScriptingException("de.uni_stuttgart.Voxie.OperationCancelled", "The operation has been cancelled") {
}
OperationCancelledException::~OperationCancelledException() {
}

Operation::Operation(QObject* parent) : QObject(parent) {
    cancelledVal.store(0);
}
Operation::~Operation() {
    //qDebug() << "~Operation()";
}

void Operation::cancel() {
    if (QThread::currentThread() != thread())
        qCritical() << "Operation::cancel() called from wrong thread" << QThread::currentThread();

    if (isCancelled())
        return;

    cancelledVal.store(1);

    emit cancelled();
}

template <typename T>
static void enqueueOnThread(QObject* obj, T&& fun) {
   QObject srcObj;
   QObject::connect(&srcObj, &QObject::destroyed, obj, std::move(fun), Qt::QueuedConnection);
}

void Operation::updateProgress(float progress) {
    if (progress < 0 || progress > 1) {
        qWarning() << "Got progress value of" << progress;
        if (progress < 0)
            progress = 0;
        else
            progress = 1;
    }

    // This will emit this->progressChanged(progress) but will compress multiple invocations of updateProgress() while the slots are still running into a single invocation

    QMutexLocker lock(&progressMutex);

    progressPending = true;
    progressPendingValue = progress;

    if (!progressUpdating) {
        enqueueOnThread(this, [this] { emitProgressChanged(); });
        progressUpdating = true;
    }
}

void Operation::emitProgressChanged() {
    QMutexLocker lock(&progressMutex);

    if (!progressUpdating) {
        qCritical() << "Operation::updateProgress: progressUpdating is false;";
        return;
    }
    if (!progressPending) {
        qCritical() << "Operation::updateProgress: progressPending is false;";
        progressUpdating = false;
        return;
    }
    float value = progressPendingValue;
    progressPending = false;

    lock.unlock();
    emit this->progressChanged(value);
    lock.relock();

    if (progressPending)
        enqueueOnThread(this, [this] { emitProgressChanged(); });
    else
        progressUpdating = false;
}

void Operation::setDescription(const QString& desc) {
    if (thread() != QThread::currentThread()) {
        qCritical() << "Operation::setDescription() call from incorrect thread";
        return;
    }

    description_ = desc;
    emit descriptionChanged(description_);
}
const QString& Operation::description() {
    if (thread() != QThread::currentThread()) {
        qCritical() << "Operation::description() call from incorrect thread";
    }

    return description_;
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
