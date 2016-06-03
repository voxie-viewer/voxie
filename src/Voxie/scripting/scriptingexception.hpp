#pragma once

#include <Voxie/voxiecore_global.hpp>

#include <functional>

#include <QtCore/QException>
#include <QtCore/QString>

#include <QtDBus/QDBusContext>
#include <QtDBus/QDBusPendingReply>

namespace voxie
{
namespace scripting
{

class VOXIECORESHARED_EXPORT ScriptingException : public QException {
    QString name_;
    QString message_;
    QString what_;
    QString additional_;

public:
    ScriptingException(const QString& name, const QString& message, const QString& additional = "");
    virtual ~ScriptingException();
    void raise() const { throw *this; }
    ScriptingException *clone() const { return new ScriptingException(*this); }

    const QString& name () const { return name_; }
    const QString& message () const { return message_; }
    const QString& additional () const { return additional_; }
    const char* what () const throw ();

    void handle(QDBusContext* context);

    static void executeWithHandler(const std::function<void()>& code, const std::function<void(ScriptingException)>& handler);
};

template <typename T>
inline T handleDBusPendingReply (QDBusPendingReply<T> reply, const QString& additional = "") {
    reply.waitForFinished();
    if (reply.isError())
        throw ScriptingException(reply.error().name(), reply.error().message(), additional);
    return reply.value();
}
template <>
inline void handleDBusPendingReply<void> (QDBusPendingReply<> reply, const QString& additional) {
    reply.waitForFinished();
    if (reply.isError())
        throw ScriptingException(reply.error().name(), reply.error().message(), additional);
}

}
}

#define HANDLEDBUSPENDINGREPLY(call) ::voxie::scripting::handleDBusPendingReply(call, #call)

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
