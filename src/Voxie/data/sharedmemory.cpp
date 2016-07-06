#include "sharedmemory.hpp"

#include <Voxie/scripting/scriptingexception.hpp>

#include <QtCore/QString>
#include <QtCore/QSysInfo>
#include <QtCore/QUuid>
#include <QtCore/QDebug>
#include <QtCore/QMutex>

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusUnixFileDescriptor>

#if !defined(Q_OS_WIN)

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/syscall.h>

#ifndef MFD_CLOEXEC
#define MFD_CLOEXEC 0x0001U
#endif
#ifndef MFD_ALLOW_SEALING
#define MFD_ALLOW_SEALING 0x0002U
#endif
#ifndef F_ADD_SEALS
#define F_ADD_SEALS (1024 + 9)
#endif
#ifndef F_SEAL_SEAL
#define F_SEAL_SEAL     0x0001
#endif
#ifndef F_SEAL_SHRINK
#define F_SEAL_SHRINK   0x0002
#endif
#ifndef F_SEAL_GROW
#define F_SEAL_GROW     0x0004
#endif
#ifndef F_SEAL_WRITE
#define F_SEAL_WRITE    0x0008
#endif
#if defined(Q_OS_LINUX) && defined(Q_PROCESSOR_X86_32) && !defined(SYS_memfd_create)
#define SYS_memfd_create 356
#endif
#if defined(Q_OS_LINUX) && defined(Q_PROCESSOR_X86_64) && !defined(SYS_memfd_create)
#define SYS_memfd_create 319
#endif
static inline int sys_memfd_create(const char* name, unsigned int flags) {
#if defined(Q_OS_LINUX) && defined(SYS_memfd_create)
    return syscall(SYS_memfd_create, name, flags);
#else
    errno = ENOSYS;
    return -1;
#endif
}

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
        auto error = GetLastError();
        throw voxie::scripting::ScriptingException("de.uni_stuttgart.Voxie.OutOfMemory", "Error creating file mapping object: " + QString::number(error));
    }

    data_ = MapViewOfFile(mapFile, FILE_MAP_WRITE, 0, 0, bytes);
    if (!data_) {
        auto error = GetLastError();
        if (!CloseHandle(mapFile))
            qCritical() << "Error closing file mapping handle:" << GetLastError();
        mapFile = nullptr;
        throw voxie::scripting::ScriptingException("de.uni_stuttgart.Voxie.OutOfMemory", "Error mapping file: " + QString::number(error));
    }

    bytes_ = bytes;

#else
    static QMutex mutex;
    static quint64 lastId = 0;
    quint64 id;
    {
        QMutexLocker lock(&mutex);
        lastId++;
        id = lastId;
    }
    QString prefix = QString("voxie-shm-%1-%2").arg(getpid()).arg(id);

    QString filename = "/memfd:" + prefix;

    bool haveMemfd = true;
    rwfd = sys_memfd_create(prefix.toUtf8().data(), MFD_CLOEXEC | MFD_ALLOW_SEALING);
    if (rwfd == -1 && errno != ENOSYS) {
        int error = errno;
        throw voxie::scripting::ScriptingException("de.uni_stuttgart.Voxie.Error", "Error creating shared memory object: memfd_create: " + prefix + ": " + qt_error_string(error));
    }

    if (rwfd == -1) {
        haveMemfd = false;
        filename = "/dev/shm/" + prefix + "-" + QUuid::createUuid().toString();
        rwfd = open (filename.toUtf8().data(), O_RDWR | O_CREAT | O_EXCL | O_NOCTTY | O_CLOEXEC, 0400);
        if (rwfd == -1) {
            int error = errno;
            throw voxie::scripting::ScriptingException("de.uni_stuttgart.Voxie.Error", "Error creating shared memory object: open: " + filename + ": " + qt_error_string(error));
        }
        if (unlink (filename.toUtf8().data()) < 0) {
            int error = errno;
            close (rwfd); rwfd = -1;
            throw voxie::scripting::ScriptingException("de.uni_stuttgart.Voxie.Error", "Error creating shared memory object: unlink: " + filename + ": " + qt_error_string(error));
        }
    }
    QString filenameFd = "/proc/self/fd/" + QString::number(rwfd);
    //rofd = open (filename.toUtf8().data(), O_RDONLY | O_NOCTTY | O_CLOEXEC);
    rofd = open (filenameFd.toUtf8().data(), O_RDONLY | O_NOCTTY | O_CLOEXEC);
    if (rofd == -1) {
        int error = errno;
        close (rwfd); rwfd = -1;
        throw voxie::scripting::ScriptingException("de.uni_stuttgart.Voxie.Error", "Error creating shared memory object: open (RO): " + filename + ": " + qt_error_string(error));
    }
    if (bytes > 0) {
        /*
        char c = 0;
        if (pwrite (rwfd, &c, 1, bytes - 1) < 0) {
            int error = errno;
            close (rwfd); rwfd = -1;
            close (rofd); rofd = -1;
            throw voxie::scripting::ScriptingException("de.uni_stuttgart.Voxie.Error", "Error creating shared memory object: pwrite: " + filename + ": " + qt_error_string(error));
            return;
        }
        */
        int error = posix_fallocate(rwfd, 0, bytes);
        if (error) {
            close (rwfd); rwfd = -1;
            close (rofd); rofd = -1;
            if (error == ENOSPC)
                throw voxie::scripting::ScriptingException("de.uni_stuttgart.Voxie.OutOfMemory", "Out of memory while creating shared memory object " + filename);
            else
                throw voxie::scripting::ScriptingException("de.uni_stuttgart.Voxie.Error", "Error creating shared memory object: posix_fallocate: " + filename + ": " + qt_error_string(error));
        }
    }

    if (haveMemfd) {
        // Prevent file from being grown or shrinked
        if (fcntl(rwfd, F_ADD_SEALS, F_SEAL_SEAL | F_SEAL_SHRINK | F_SEAL_GROW) < 0) {
            int error = errno;
            close (rwfd); rwfd = -1;
            close (rofd); rofd = -1;
            throw voxie::scripting::ScriptingException("de.uni_stuttgart.Voxie.Error", "Error creating shared memory object: fcntl(F_ADD_SEALS): " + filename + ": " + qt_error_string(error));
        }
    }

    data_ = mmap (NULL, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, rwfd, 0);
    if (!data_) {
        int error = errno;
        close (rwfd); rwfd = -1;
        close (rofd); rofd = -1;
        throw voxie::scripting::ScriptingException("de.uni_stuttgart.Voxie.Error", "Error creating shared memory object: mmap: " + filename + ": " + qt_error_string(error));
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
