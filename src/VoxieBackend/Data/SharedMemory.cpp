/*
 * Copyright (c) 2014-2022 The Voxie Authors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "SharedMemory.hpp"

#include <VoxieClient/Exception.hpp>

#include <QtCore/QDebug>
#include <QtCore/QMutex>
#include <QtCore/QString>
#include <QtCore/QSysInfo>
#include <QtCore/QUuid>

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusUnixFileDescriptor>

#if !defined(Q_OS_WIN)

#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef MFD_CLOEXEC
#define MFD_CLOEXEC 0x0001U
#endif
#ifndef MFD_ALLOW_SEALING
#define MFD_ALLOW_SEALING 0x0002U
#endif
#if defined(Q_OS_LINUX)
#ifndef F_ADD_SEALS
#define F_ADD_SEALS (1024 + 9)
#endif
#ifndef F_SEAL_SEAL
#define F_SEAL_SEAL 0x0001
#endif
#ifndef F_SEAL_SHRINK
#define F_SEAL_SHRINK 0x0002
#endif
#ifndef F_SEAL_GROW
#define F_SEAL_GROW 0x0004
#endif
#ifndef F_SEAL_WRITE
#define F_SEAL_WRITE 0x0008
#endif
#endif
#if defined(Q_OS_LINUX) && defined(Q_PROCESSOR_X86_32) && \
    !defined(SYS_memfd_create)
#define SYS_memfd_create 356
#endif
#if defined(Q_OS_LINUX) && defined(Q_PROCESSOR_X86_64) && \
    !defined(SYS_memfd_create)
#define SYS_memfd_create 319
#endif
static inline int sys_memfd_create(const char* name, unsigned int flags) {
#if defined(Q_OS_LINUX) && defined(SYS_memfd_create)
  return syscall(SYS_memfd_create, name, flags);
#else
  (void)name;
  (void)flags;
  errno = ENOSYS;
  return -1;
#endif
}

#if defined(F_ADD_SEALS)
// Needed to know whether it is ok when F_ADD_SEALS fails
#include <valgrind/valgrind.h>
#endif

#else

#include <windows.h>

static QString errnumToString(DWORD errnum) {
  wchar_t* lpMsgBuf;
  if (!FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                          FORMAT_MESSAGE_FROM_SYSTEM |
                          FORMAT_MESSAGE_IGNORE_INSERTS,
                      NULL, errnum, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                      (LPWSTR)&lpMsgBuf, 0, NULL)) {
    DWORD err2 = GetLastError();
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "FormatMessage () for " + QString::number(errnum) +
                            " returned " + QString::number(err2));
  }
  size_t len = lstrlenW(lpMsgBuf);
  if (len && lpMsgBuf[len - 1] == '\n') {
    len--;
    if ((len > 0) && lpMsgBuf[len - 1] == '\r') len--;
  }
  QString str = QString::fromWCharArray(lpMsgBuf, (int)len) + " (" +
                QString::number(errnum) + ")";
  LocalFree(lpMsgBuf);
  return str;
}

#endif

using namespace vx;

#if defined(Q_OS_WIN)
#endif

SharedMemory::SharedMemory(std::size_t bytes)
    :
#if defined(Q_OS_WIN)
      mapFile(nullptr),
#else
      rwfd(-1),
      rofd(-1),
#endif
      data_(nullptr),
      bytes_(0) {
#if defined(Q_OS_WIN)
  /*
  data_ = new char[bytes];
  bytes_ = bytes;
  */

  handleName = "VoxieSharedMemory-" + QUuid::createUuid().toString();

  quint32 sizeHigh = (quint32)(((quint64)bytes) >> 32);
  quint32 sizeLow = (quint32)bytes;
  if (bytes == 0) sizeLow = 1;
  mapFile = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
                               sizeHigh, sizeLow, handleName.toUtf8().data());
  if (!mapFile) {
    auto error = GetLastError();
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.OutOfMemory",
        "Error creating file mapping object: " + errnumToString(error));
  }

  data_ = MapViewOfFile(mapFile, FILE_MAP_WRITE, 0, 0, bytes);
  if (!data_) {
    auto error = GetLastError();
    if (!CloseHandle(mapFile))
      qCritical() << "Error closing file mapping handle:" << GetLastError();
    mapFile = nullptr;
    throw vx::Exception("de.uni_stuttgart.Voxie.OutOfMemory",
                        "Error mapping file: " + errnumToString(error));
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
  rwfd =
      sys_memfd_create(prefix.toUtf8().data(), MFD_CLOEXEC | MFD_ALLOW_SEALING);
  if (rwfd == -1 && errno != ENOSYS) {
    int error = errno;
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Error creating shared memory object: memfd_create: " +
                            prefix + ": " + qt_error_string(error));
  }

  if (rwfd == -1) {
    haveMemfd = false;
    QString shmName = prefix;
    // filename = "/dev/shm/" + shmName;
    mode_t mode = 0400;
#ifdef Q_OS_MACOS
    mode =
        0600;  // https://groups.google.com/forum/#!topic/native-client-reviews/EHHHfK_xPZ4
#endif
    // rwfd = open(filename.toUtf8().data(),
    //            O_RDWR | O_CREAT | O_EXCL | O_NOCTTY | O_CLOEXEC, mode);
    rwfd = shm_open(shmName.toUtf8().data(),
                    O_RDWR | O_CREAT | O_EXCL | O_NOCTTY | O_CLOEXEC, mode);
    if (rwfd == -1) {
      int error = errno;
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Error creating shared memory object: shm_open: " +
                              shmName + ": " + qt_error_string(error));
    }

    rofd =
        shm_open(shmName.toUtf8().data(), O_RDONLY | O_NOCTTY | O_CLOEXEC, 0);
    if (rofd == -1) {
      int error = errno;
      close(rwfd);
      rwfd = -1;
      shm_unlink(shmName.toUtf8().data());
      throw vx::Exception(
          "de.uni_stuttgart.Voxie.Error",
          "Error creating shared memory object: shm_open (RO): " + shmName +
              ": " + qt_error_string(error));
    }

    // if (unlink(filename.toUtf8().data()) < 0) {
    if (shm_unlink(shmName.toUtf8().data()) < 0) {
      int error = errno;
      close(rwfd);
      rwfd = -1;
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Error creating shared memory object: shm_unlink: " +
                              shmName + ": " + qt_error_string(error));
    }
  }

  if (rofd == -1) {
    QString filenameFd = "/proc/self/fd/" + QString::number(rwfd);
    // rofd = open (filename.toUtf8().data(), O_RDONLY | O_NOCTTY | O_CLOEXEC);
    rofd = open(filenameFd.toUtf8().data(), O_RDONLY | O_NOCTTY | O_CLOEXEC);
    if (rofd == -1) {
      int error = errno;
      close(rwfd);
      rwfd = -1;
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Error creating shared memory object: open (RO): " +
                              filename + ": " + qt_error_string(error));
    }
  }

  if (bytes > 0) {
    /*
    char c = 0;
    if (pwrite (rwfd, &c, 1, bytes - 1) < 0) {
        int error = errno;
        close (rwfd); rwfd = -1;
        close (rofd); rofd = -1;
        throw
    vx::Exception("de.uni_stuttgart.Voxie.Error", "Error
    creating shared memory object: pwrite: " + filename + ": " +
    qt_error_string(error)); return;
    }
    */
#ifdef Q_OS_MACOS
    if (ftruncate(rwfd, bytes) < 0) {
      int error = errno;
      close(rwfd);
      rwfd = -1;
      close(rofd);
      rofd = -1;
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Error creating shared memory object: ftruncate: " +
                              filename + ": " + qt_error_string(error));
    }
    if (0) {  // F_PREALLOCATE does not seem to be supported for SHM files
      fstore_t store = {F_ALLOCATEALL, F_PEOFPOSMODE, 0, (off_t)bytes};
      if (fcntl(rwfd, F_PREALLOCATE, &store) < 0) {
        int error = errno;
        close(rwfd);
        rwfd = -1;
        close(rofd);
        rofd = -1;
        if (error == ENOSPC)
          throw vx::Exception(
              "de.uni_stuttgart.Voxie.OutOfMemory",
              "Out of memory while creating shared memory object " + filename);
        else
          throw vx::Exception(
              "de.uni_stuttgart.Voxie.Error",
              "Error creating shared memory object: fcntl(F_PREALLOCATE): " +
                  filename + ": " + qt_error_string(error));
      }
    }

#else
    // qDebug() << "posix_fallocate" << rwfd << 0 << bytes;
    int error;
    do {
      error = posix_fallocate(rwfd, 0, bytes);
      // This can happen e.g. if a child process (e.g. a filter) exits while the
      // posix_fallocate call is running (the call might take some time when a
      // large memory area is allocated).
      // if (error == EINTR) qWarning() << "posix_fallocate returned EINTR";
    } while (error == EINTR);
    if (error) {
      close(rwfd);
      rwfd = -1;
      close(rofd);
      rofd = -1;
      if (error == ENOSPC)
        throw vx::Exception(
            "de.uni_stuttgart.Voxie.OutOfMemory",
            "Out of memory while creating shared memory object " + filename);
      else
        throw vx::Exception(
            "de.uni_stuttgart.Voxie.Error",
            "Error creating shared memory object: posix_fallocate: " +
                filename + ": " + qt_error_string(error));
    }
#endif
  }

#if defined(F_ADD_SEALS) && defined(F_SEAL_SEAL) && defined(F_SEAL_SHRINK) && \
    defined(F_SEAL_GROW)
  if (haveMemfd) {
    // Prevent file from being grown or shrinked
    if (fcntl(rwfd, F_ADD_SEALS, F_SEAL_SEAL | F_SEAL_SHRINK | F_SEAL_GROW) <
        0) {
      int error = errno;
      // Note: On valgrind this call might fail
      if (!RUNNING_ON_VALGRIND) {
        close(rwfd);
        rwfd = -1;
        close(rofd);
        rofd = -1;
        throw vx::Exception(
            "de.uni_stuttgart.Voxie.Error",
            "Error creating shared memory object: fcntl(F_ADD_SEALS): " +
                filename + ": " + qt_error_string(error));
      }
    }
  }
#endif

  if (bytes > 0) {
    data_ = mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, rwfd, 0);
    if (data_ == MAP_FAILED) {
      int error = errno;
      close(rwfd);
      rwfd = -1;
      close(rofd);
      rofd = -1;
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Error creating shared memory object: mmap: " +
                              filename + ": " + qt_error_string(error));
    }
  } else {
    data_ = nullptr;
  }
  bytes_ = bytes;
#endif

  // qDebug() << "Shared memory allocated" << bytes_;
}

SharedMemory::~SharedMemory() {
  // qDebug() << "Shared memory freed" << bytes_;

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
    munmap(data_, bytes_);
    data_ = nullptr;
  }
  if (rwfd != -1) {
    close(rwfd);
    rwfd = -1;
  }
  if (rofd != -1) {
    close(rofd);
    rofd = -1;
  }
#endif
  bytes_ = 0;
}

void SharedMemory::getHandle(bool rw, QMap<QString, QDBusVariant>& handle) {
#if defined(Q_OS_WIN)
  // TODO: Store rw in handle structure?
  (void)rw;

  handle["Type"] = QDBusVariant("WindowsNamedFileMapping");
  handle["MappingObjectName"] = QDBusVariant(handleName);
  handle["LocalMachineID"] =
      QDBusVariant(QString::fromUtf8(QDBusConnection::localMachineId()));

  DWORD pid = GetCurrentProcessId();
  DWORD sessionId;
  if (!ProcessIdToSessionId(pid, &sessionId))
    throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                        "Call to ProcessIdToSessionId() failed");
  handle["SessionID"] = QDBusVariant((quint32)sessionId);
#else
  handle["Type"] = QDBusVariant("UnixFileDescriptor");
  handle["FileDescriptor"] = QDBusVariant(
      QVariant::fromValue(QDBusUnixFileDescriptor(rw ? fdRW() : fdRO())));
#endif
}
