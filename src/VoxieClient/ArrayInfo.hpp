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

#pragma once

#include <VoxieClient/DBusTypeList.hpp>

namespace vx {

struct Array1Info {
  Array1Info()
      : handle(),
        offset(0),
        dataType(""),
        dataTypeSize(0),
        byteorder(""),
        size(0),
        stride(0),
        metadata() {}
  QMap<QString, QDBusVariant> handle;
  qint64 offset;

  QString dataType;
  quint32 dataTypeSize;
  QString byteorder;

  quint64 size;
  qint64 stride;

  QMap<QString, QDBusVariant> metadata;

  VOXIECLIENT_EXPORT
  Array1Info(const vx::Array1InfoDBus& info);
  VOXIECLIENT_EXPORT
  vx::Array1InfoDBus toDBus() const;
};

struct Array2Info {
  Array2Info()
      : handle(),
        offset(0),
        dataType(""),
        dataTypeSize(0),
        byteorder(""),
        sizeX(0),
        sizeY(0),
        strideX(0),
        strideY(0),
        metadata() {}
  QMap<QString, QDBusVariant> handle;
  qint64 offset;

  QString dataType;
  quint32 dataTypeSize;
  QString byteorder;

  quint64 sizeX, sizeY;
  qint64 strideX, strideY;

  QMap<QString, QDBusVariant> metadata;

  VOXIECLIENT_EXPORT
  Array2Info(const vx::Array2InfoDBus& info);
  VOXIECLIENT_EXPORT
  vx::Array2InfoDBus toDBus() const;
};

struct Array3Info {
  Array3Info()
      : handle(),
        offset(0),
        dataType(""),
        dataTypeSize(0),
        byteorder(""),
        sizeX(0),
        sizeY(0),
        sizeZ(0),
        strideX(0),
        strideY(0),
        strideZ(0),
        metadata() {}
  QMap<QString, QDBusVariant> handle;
  qint64 offset;

  QString dataType;
  quint32 dataTypeSize;
  QString byteorder;

  quint64 sizeX, sizeY, sizeZ;
  qint64 strideX, strideY, strideZ;

  QMap<QString, QDBusVariant> metadata;

  VOXIECLIENT_EXPORT
  Array3Info(const vx::Array3InfoDBus& info);
  VOXIECLIENT_EXPORT
  vx::Array3InfoDBus toDBus() const;
};

}  // namespace vx
