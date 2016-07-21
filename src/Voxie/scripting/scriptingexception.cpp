#include "scriptingexception.hpp"

#include <QtCore/QThreadStorage>
#include <QtCore/QSharedPointer>

using namespace voxie::scripting;

static QThreadStorage<std::function<void(ScriptingException)>*> handler;

Q_DECLARE_METATYPE(QSharedPointer<voxie::scripting::ScriptingException>);

ScriptingException::ScriptingException(const QString& name, const QString& message, const QString& additional) : name_ (name), message_ (message), additional_ (additional) {
    what_ = "ScriptingException: " + this->name() + ": " + this->message();

    qRegisterMetaType<QSharedPointer<ScriptingException>>();
}

ScriptingException::~ScriptingException() {
}

void ScriptingException::handle(QDBusContext* context) {
    if (handler.localData())
        (*handler.localData())(*this);
    if (context->calledFromDBus())
        context->sendErrorReply(name(), message());
}

namespace {
    struct PushHandler {
        std::function<void(ScriptingException)>* storedValue;
        PushHandler(const std::function<void(ScriptingException)>& handler) {
            storedValue = ::handler.localData();
            ::handler.localData() = (std::function<void(ScriptingException)>*) &handler;
        }
        ~PushHandler() {
            ::handler.localData() = storedValue;
        }
    };
}

void ScriptingException::executeWithHandler(const std::function<void()>& code, const std::function<void(ScriptingException)>& handler) {
    PushHandler push(handler);
    code();
}

const char* ScriptingException::what() const throw () {
    return what_.toUtf8().data();
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
