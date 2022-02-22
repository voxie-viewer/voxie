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

#include "MappedBuffer.hpp"

#include <VoxieClient/Exception.hpp>

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusUnixFileDescriptor>

#ifndef Q_OS_WIN
#include <sys/mman.h>
#endif

using namespace vx;

#ifndef Q_OS_WIN
class vx::MMapHandle {
 public:
  void* ptr;
  size_t bytes;
  MMapHandle(void* ptr, size_t bytes) : ptr(ptr), bytes(bytes) {}
  ~MMapHandle() {
    if (ptr != MAP_FAILED) munmap(ptr, bytes);
  }
};
#else
#include <windows.h>
#undef interface

class vx::HandleHolder {
 public:
  HANDLE handle;
  HandleHolder(HANDLE handle) : handle(handle) {}
  ~HandleHolder() {
    if (handle) CloseHandle(handle);
  }
};
class vx::MapHolder {
 public:
  void* data;
  MapHolder(void* data) : data(data) {}
  ~MapHolder() {
    if (data) UnmapViewOfFile(data);
  }
};
#endif

MappedBuffer::MappedBuffer(const QMap<QString, QDBusVariant>& handle,
                           size_t size, bool writable) {
  QString type = handle["Type"].variant().toString();

#ifndef Q_OS_WIN
  if (type != "UnixFileDescriptor")
    throw Exception("de.uni_stuttgart.Voxie.Error",
                    "Got unknown handle type '" + type + "'");
  QDBusUnixFileDescriptor fd =
      handle["FileDescriptor"].variant().value<QDBusUnixFileDescriptor>();
  if (!fd.isValid())
    throw Exception("de.uni_stuttgart.Voxie.Error",
                    "Got invalid file descriptor");

  if (size > 0) {
    mmapHandle.reset(new MMapHandle(
        mmap(NULL, size, writable ? (PROT_READ | PROT_WRITE) : PROT_READ,
             MAP_SHARED, fd.fileDescriptor(), 0),
        size));
    if (mmapHandle->ptr == MAP_FAILED)
      throw Exception("de.uni_stuttgart.Voxie.Error",
                      "mmap failed: " + qt_error_string(errno));

    data_ = mmapHandle->ptr;
  } else {
    data_ = nullptr;
  }
#else
  if (type != "WindowsNamedFileMapping")
    throw Exception("de.uni_stuttgart.Voxie.Error",
                    "Got unknown handle type '" + type + "'");

  QString localMachineIDVoxie = handle["LocalMachineID"].variant().toString();
  QString localMachineIDLocal =
      QString::fromUtf8(QDBusConnection::localMachineId());
  if (localMachineIDVoxie != localMachineIDLocal)
    throw Exception("de.uni_stuttgart.Voxie.Error",
                    "Machine ID mismatch: '" + localMachineIDVoxie +
                        "' (Voxie) != '" + localMachineIDLocal + "' (local)");

  DWORD pid = GetCurrentProcessId();
  DWORD sessionIDLocal;
  if (!ProcessIdToSessionId(pid, &sessionIDLocal))
    throw Exception("de.uni_stuttgart.Voxie.Error",
                    "Call to ProcessIdToSessionId() failed");
  quint32 sessionIDVoxie = handle["SessionID"].variant().value<quint32>();
  if (sessionIDVoxie != sessionIDLocal)
    throw Exception("de.uni_stuttgart.Voxie.Error",
                    QString("Session ID mismatch: '%1' (Voxie) != '%2' (local)")
                        .arg(sessionIDVoxie)
                        .arg(sessionIDLocal));

  QString objectName = handle["MappingObjectName"].variant().toString();

  handleHolder.reset(new HandleHolder(
      OpenFileMappingA(writable ? FILE_MAP_WRITE : FILE_MAP_READ, false,
                       objectName.toUtf8().data())));
  if (!handleHolder->handle)
    throw Exception("de.uni_stuttgart.Voxie.Error",
                    QString("OpenFileMappingA failed: %1").arg(GetLastError()));

  map.reset(new MapHolder(
      MapViewOfFile(handleHolder->handle,
                    writable ? FILE_MAP_WRITE : FILE_MAP_READ, 0, 0, size)));
  if (!map->data)
    throw Exception("de.uni_stuttgart.Voxie.Error",
                    QString("MapViewOfFile failed: %1").arg(GetLastError()));

  data_ = map->data;
#endif
}

MappedBuffer::~MappedBuffer() {}
