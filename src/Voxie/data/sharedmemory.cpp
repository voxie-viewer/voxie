#include "sharedmemory.hpp"

#include <Voxie/scripting/scriptingexception.hpp>

#include <QtCore/QString>
#include <QtCore/QSysInfo>
#include <QtCore/QUuid>

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusUnixFileDescriptor>

#if !defined(Q_OS_WIN)
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#else
#include <windows.h>
#endif

using namespace voxie::data;

#if defined(Q_OS_WIN)
#endif

SharedMemory::SharedMemory (std::size_t bytes) : 
#if defined(Q_OS_WIN)
    mapFile(nullptr),
#else
    rwfd (-1), rofd (-1),
#endif
    data_ (nullptr), bytes_ (0) {
#if defined(Q_OS_WIN)
    /*
    data_ = new char[bytes];
    bytes_ = bytes;
    */

    handleName = "VoxieSharedMemory-" + QUuid::createUuid().toString();

    quint32 sizeHigh = (quint32) (((quint64) bytes) >> 32);
    quint32 sizeLow = (quint32) bytes;
    mapFile = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, sizeHigh, sizeLow, handleName.toUtf8().data());
    if (!mapFile) {
        qCritical() << "Error creating file mapping object:" << GetLastError();
        return;
    }

    data_ = MapViewOfFile(mapFile, FILE_MAP_WRITE, 0, 0, bytes);
    if (!data_) {
        qCritical() << "Error mapping file:" << GetLastError();
        if (!CloseHandle(mapFile))
            qCritical() << "Error closing file mapping handle:" << GetLastError();
        mapFile = nullptr;
        return;
    }

    bytes_ = bytes;

#else
    QString filename = "/dev/shm/voxie-shm-" + QUuid::createUuid().toString();
    rwfd = open (filename.toUtf8().data(), O_RDWR | O_CREAT | O_EXCL | O_NOCTTY | O_CLOEXEC, 0400);
    if (rwfd == -1) {
        perror (("open: " + filename).toUtf8().data());
        return;
    }
    if (unlink (filename.toUtf8().data()) < 0) {
        perror (("unlink: " + filename).toUtf8().data());
        close (rwfd); rwfd = -1;
        return;
    }
    QString filenameFd = "/proc/self/fd/" + QString::number(rwfd);
    //rofd = open (filename.toUtf8().data(), O_RDONLY | O_NOCTTY | O_CLOEXEC);
    rofd = open (filenameFd.toUtf8().data(), O_RDONLY | O_NOCTTY | O_CLOEXEC);
    if (rofd == -1) {
        perror (("open (RO): " + filenameFd).toUtf8().data());
        close (rwfd); rwfd = -1;
        return;
    }
    if (bytes > 0) {
        char c = 0;
        if (pwrite (rwfd, &c, 1, bytes - 1) < 0) {
            perror (("write: " + filename).toUtf8().data());
            close (rwfd); rwfd = -1;
            close (rofd); rofd = -1;
            return;
        }
    }
    data_ = mmap (NULL, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, rwfd, 0);
    if (!data_) {
        perror (("mmap: " + filename).toUtf8().data());
        close (rwfd); rwfd = -1;
        close (rofd); rofd = -1;
        return;
    }
    bytes_ = bytes;
#endif
}

SharedMemory::~SharedMemory () {
#if defined(Q_OS_WIN)
    /*
    delete[] data_;
    data_ = nullptr;
    */
    if (data_) {
        if (!UnmapViewOfFile(data_))
            qCritical() << "Error unmapping file:" << GetLastError();
        data_ = nullptr;
    }
    if (mapFile) {
        if (!CloseHandle(mapFile))
            qCritical() << "Error closing file mapping handle:" << GetLastError();
        mapFile = nullptr;
    }
#else
    if (data_) {
        munmap (data_, bytes_);
        data_ = nullptr;
    }
    if (rwfd != -1) {
        close (rwfd); rwfd = -1;
    }
    if (rofd != -1) {
        close (rofd); rofd = -1;
    }
#endif
    bytes_ = 0;
}

void SharedMemory::getHandle (bool rw, QMap<QString, QDBusVariant>& handle) {
#if defined(Q_OS_WIN)
    handle["Type"] = QDBusVariant ("WindowsNamedFileMapping");
    handle["MappingObjectName"] = QDBusVariant (handleName);
    handle["LocalMachineID"] = QDBusVariant (QString::fromUtf8(QDBusConnection::localMachineId()));

    DWORD pid = GetCurrentProcessId();
    DWORD sessionId;
    if (!ProcessIdToSessionId(pid, &sessionId))
        throw voxie::scripting::ScriptingException("de.uni_stuttgart.Voxie.InternalError", "Call to ProcessIdToSessionId() failed");
    handle["SessionID"] = QDBusVariant ((quint32) sessionId);
#else
    handle["Type"] = QDBusVariant ("UnixFileDescriptor");
    handle["FileDescriptor"] = QDBusVariant (QVariant::fromValue (QDBusUnixFileDescriptor (rw ? fdRW() : fdRO())));
#endif
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
