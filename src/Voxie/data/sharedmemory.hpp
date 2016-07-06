#pragma once

#include <Voxie/scripting/dbustypes.hpp>

#include <cstdlib>

#include <QtCore/QtGlobal>

#include <QtDBus/QDBusVariant>

namespace voxie { namespace data {
class SharedMemory {
#if !defined(Q_OS_WIN)
    int rwfd, rofd;
#else
    QString handleName;
    /*HANDLE*/ void* mapFile;
#endif
    void* data_;
    std::size_t bytes_;
public:
    // throws ScriptingException
    SharedMemory (std::size_t bytes);
    ~SharedMemory ();
    void* getData() const { return data_; }
#if !defined(Q_OS_WIN)
    int fdRW () { return rwfd; }
    int fdRO () { return rofd; }
#endif
    void getHandle (bool rw, QMap<QString, QDBusVariant>& handle);
};
}}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
