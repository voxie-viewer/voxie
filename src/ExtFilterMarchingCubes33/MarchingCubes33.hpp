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

#include <VoxieClient/Array.hpp>
#include <VoxieClient/DBusProxies.hpp>
#include <VoxieClient/DBusTypeList.hpp>
#include <VoxieClient/DBusUtil.hpp>

#include <QtGui/QVector3D>

class
    // Export for unit tests
    //#if defined(IN_PLUGINFILTERS)
    //    Q_DECL_EXPORT
    //#else
    //    Q_DECL_IMPORT
    //#endif
    MarchingCubes {
 public:
  MarchingCubes();
  ~MarchingCubes();

  typedef qint32 IndexType;
  const static IndexType invalidVertex = -1;

  struct TRIANGLE {
    IndexType indices[3];
  };

  void extract(vx::Array3<const float>& inputVolume,
               const vx::TupleVector<quint64, 3>& dimensions,
               const QVector3D& origin, const QVector3D& spacing,
               float threshold);

  const QVector<QVector3D>* GetVertices();
  const QVector<TRIANGLE>* GetTriangles();
};
