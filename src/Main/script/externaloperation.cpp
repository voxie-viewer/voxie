#include "externaloperation.hpp"

#include <Voxie/data/voxeldata.hpp>

#include <Voxie/io/operation.hpp>

#include <Voxie/scripting/client.hpp>
#include <Voxie/scripting/scriptingexception.hpp>

#include <QtCore/QDebug>

using namespace voxie::scripting;
using namespace voxie::data;
using namespace voxie::io;

ExternalOperation::ExternalOperation(const QSharedPointer<voxie::io::Operation>& operation) : ScriptableObject("ExternalOperation", nullptr), operation(operation) {
    auto adapter = new ExternalOperationAdaptor(this);

    if (!operation)
        qCritical() << "ExternalOperation::ExternalOperation: operation is null";

    connect(operation.data(), &Operation::cancelled, adapter, &ExternalOperationAdaptor::Cancelled);
}

ExternalOperation::~ExternalOperation() {
}

ExternalOperationAdaptor::ExternalOperationAdaptor(ExternalOperation* object) : QDBusAbstractAdaptor(object), object(object) {
}
ExternalOperationAdaptor::~ExternalOperationAdaptor() {
}

void ExternalOperation::checkClient() {
    if (!client)
        throw ScriptingException("de.uni_stuttgart.Voxie.InvalidOperation", "ExternalOperation not yet claimed");
    if (client->uniqueConnectionName() == "")
        return;
    if (!calledFromDBus()) {
        qWarning() << "ExternalOperationAdaptor::checkClient() called from non-DBus context:" << this;
        return;
    }
    if (client->uniqueConnectionName() != message().service()) {
        qWarning() << "ExternalOperationAdaptor::checkClient(): connection mismatch, expected" << client->uniqueConnectionName() << "got" << message().service() << "for" << this;
    }
}

bool ExternalOperationAdaptor::isCancelled() {
    return object->operation->isCancelled();
}

void ExternalOperationAdaptor::ClaimOperation(const QDBusObjectPath& client) {
    try {
        //qDebug() << "ExternalOperation::ClaimOperation" << client.path();

        //voxie::scripting::ScriptableObject::checkOptions(options);
        Client* clientPtr = qobject_cast<Client*> (voxie::scripting::ScriptableObject::lookupWeakObject(client));
        if (!clientPtr) {
            throw ScriptingException("de.uni_stuttgart.Voxie.ObjectNotFound", "Cannot find client object");
        }

        QSharedPointer<QSharedPointer<ExternalOperation> > ref(object->initialReference);
        if (!ref)
            throw ScriptingException("de.uni_stuttgart.Voxie.Error", "initialReference not set");
        if (!*ref)
            throw ScriptingException("de.uni_stuttgart.Voxie.InvalidOperation", "ExternalOperation already claimed");
        auto ptr = *ref;
        if (ptr.data() != object)
            throw ScriptingException("de.uni_stuttgart.Voxie.Error", "initialReference set to invalid value");
        if (object->client)
            throw ScriptingException("de.uni_stuttgart.Voxie.Error", "object->client already set");
        object->client = clientPtr;

        ref->reset();

        clientPtr->IncRefCount(ptr);
        return;
    } catch (voxie::scripting::ScriptingException& e) {
        e.handle(object);
        return;
    }
}

void ExternalOperationAdaptor::SetProgress(double progress) {
    try {
        object->checkClient();

        //qDebug() << "ExternalOperation::SetProgress" << progress;
        object->operation->updateProgress(progress);
    } catch (voxie::scripting::ScriptingException& e) {
        e.handle(object);
        return;
    }
}

void ExternalOperationAdaptor::FinishError(const QString& name, const QString& message) {
    try {
        object->checkClient();

        //qDebug() << "ExternalOperation::FinishError" << name << message;
        if (object->isFinished)
            throw ScriptingException("de.uni_stuttgart.Voxie.InvalidOperation", "ExternalOperationLoad is already finished");

        emit object->error(ScriptingException(name, message));
        object->isFinished = true;
    } catch (voxie::scripting::ScriptingException& e) {
        e.handle(object);
        return;
    }
}

ExternalOperationLoad::ExternalOperationLoad(const QSharedPointer<voxie::io::Operation>& operation) : ExternalOperation(operation) {
    new ExternalOperationLoadAdaptor(this);
}

ExternalOperationLoad::~ExternalOperationLoad() {
}

ExternalOperationLoadAdaptor::ExternalOperationLoadAdaptor(ExternalOperationLoad* object) : QDBusAbstractAdaptor(object), object(object) {
}
ExternalOperationLoadAdaptor::~ExternalOperationLoadAdaptor() {
}

void ExternalOperationLoadAdaptor::Finish(const QDBusObjectPath& data) {
    try {
        object->checkClient();

        //qDebug() << "ExternalOperation::Finish" << data.path();

        QSharedPointer<voxie::scripting::ScriptableObject> obj = voxie::scripting::ScriptableObject::lookupObject(data);
        if (!obj)
            throw voxie::scripting::ScriptingException("de.uni_stuttgart.Voxie.ObjectNotFound", "Object " + data.path() + " not found");
        auto voxelData = qSharedPointerCast<VoxelData> (obj);
        if (!voxelData)
            throw voxie::scripting::ScriptingException("de.uni_stuttgart.Voxie.InvalidObjectType", "Object " + data.path() + " is not a voxel data object");

        if (object->isFinished)
            throw ScriptingException("de.uni_stuttgart.Voxie.InvalidOperation", "ExternalOperationLoad is already finished");

        emit object->finished(voxelData);
        object->isFinished = true;
    } catch (voxie::scripting::ScriptingException& e) {
        e.handle(object);
        return;
    }
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
