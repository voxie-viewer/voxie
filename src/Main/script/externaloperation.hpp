#pragma once

#include <Voxie/scripting/scriptingcontainer.hpp>
#include <Voxie/scripting/client.hpp>

#include <QtCore/QPointer>

#include <QtDBus/QDBusContext>
#include <QtDBus/QDBusObjectPath>
#include <QtDBus/QDBusAbstractAdaptor>

namespace voxie {

namespace data {
class VoxelData;
}
namespace scripting {
class ScriptingException;

class ExternalOperationAdaptor;
class ExternalOperation : public voxie::scripting::ScriptableObject, public QDBusContext {
    Q_OBJECT

    friend class ExternalOperationAdaptor;

    QPointer<voxie::scripting::Client> client = nullptr;

protected:
    bool isFinished = false;

    void checkClient();

public:
    ExternalOperation();
    ~ExternalOperation() override;

    QWeakPointer<QSharedPointer<ExternalOperation>> initialReference;

signals:
    void error(const voxie::scripting::ScriptingException& error);
};
class ExternalOperationAdaptor : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "de.uni_stuttgart.Voxie.ExternalOperation")

    ExternalOperation* object;

public:
    ExternalOperationAdaptor(ExternalOperation* object);
    ~ExternalOperationAdaptor() override;

public slots:
    // Must be called before doing anything else with the object
    // After the operation is finished, client.DecRefCount(operation) has to
    // be called.
    void ClaimOperation(const QDBusObjectPath& client);

    // Set the progress of the operation.
    // (progress should be between 0.0 and 1.0)
    void SetProgress(double progress);

    void FinishError(const QString& name, const QString& message);
};

class ExternalOperationLoadAdaptor;
class ExternalOperationLoad : public ExternalOperation {
    Q_OBJECT

    friend class ExternalOperationLoadAdaptor;

public:
    ExternalOperationLoad();
    ~ExternalOperationLoad() override;

signals:
    void finished(const QSharedPointer<voxie::data::VoxelData>& data);
};
class ExternalOperationLoadAdaptor : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "de.uni_stuttgart.Voxie.ExternalOperationLoad")

    ExternalOperationLoad* object;

public:
    ExternalOperationLoadAdaptor(ExternalOperationLoad* object);
    ~ExternalOperationLoadAdaptor() override;

public slots:
    void Finish(const QDBusObjectPath& data);
};

}
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
