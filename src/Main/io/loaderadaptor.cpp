#include "loaderadaptor.hpp"

#include <Main/root.hpp>

#include <Voxie/data/voxeldata.hpp>

#include <Voxie/io/loader.hpp>

#include <Voxie/scripting/scriptingexception.hpp>

using namespace voxie::io;
using namespace voxie::scripting;

LoaderAdaptor::LoaderAdaptor (Loader* object) : QDBusAbstractAdaptor (object), object (object) {}
LoaderAdaptor::~LoaderAdaptor () {}

QVariantMap LoaderAdaptor::filter() {
    return object->filter();
}

QDBusObjectPath LoaderAdaptor::Load(const QString &fileName, const QMap<QString, QVariant>& options) {
    // Get a strong reference to the loader (needed because loading happens on
    // a separate thread and might not finish before the main thread drops its
    // reference on the loader)
    QSharedPointer<Loader> ptr(object->getSelf());
    try {
        if (!ptr)
            throw ScriptingException("de.uni_stuttgart.Voxie.ObjectNotFound", "Could not get strong reference to loader object");
        if (ptr.data() != object)
            throw ScriptingException("de.uni_stuttgart.Voxie.Error", "object->self != object");
    } catch (voxie::scripting::ScriptingException& e) {
        e.handle(object);
        return voxie::scripting::ScriptableObject::getPath(nullptr);
    }

    return voxie::VoxieInstance::OpenFileImpl(Root::instance(), object, "de.uni_stuttgart.Voxie.Loader", "Load", fileName, ptr, options);
}


// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
