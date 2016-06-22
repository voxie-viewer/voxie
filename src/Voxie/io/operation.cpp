#include "operation.hpp"

#include <QtCore/QDebug>

using namespace voxie::io;

OperationCancelledException::OperationCancelledException() : voxie::scripting::ScriptingException("de.uni_stuttgart.Voxie.OperationCancelled", "The operation has been cancelled") {
}
OperationCancelledException::~OperationCancelledException() {
}

Operation::Operation(QObject* parent) : QObject(parent) {
}
Operation::~Operation() {
}

void Operation::cancel() {
    cancelled.store(1);
}

void Operation::updateProgress(float progress) {
    if (progress < 0 || progress > 1) {
        qWarning() << "Got progress value of" << progress;
        if (progress < 0)
            progress = 0;
        else
            progress = 1;
    }
    emit progressChanged(progress);
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
