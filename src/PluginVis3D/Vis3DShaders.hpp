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

#include <QtGui/QOpenGLFunctions>
#include <QtGui/QOpenGLShaderProgram>

#include <VoxieBackend/Data/SurfaceAttribute.hpp>

namespace vx {
class SurfaceData;
}

class Vis3DShaders : public QObject {
  Q_OBJECT

  int maxClipDistances_;

  QOpenGLShaderProgram renderTriangles_;
  QOpenGLShaderProgram pickTriangles_;

  // The surfaceAttributes value is used to get the list (and kind) of
  // attributes available to the shader. It can be nullptr if there are no
  // attributes.
  QString LoadShaderFile(
      QString path, const QSharedPointer<vx::SurfaceData>& surfaceAttributes,
      const QString& shadingTechnique);

  // Shader variable IDs

#define VAR(fname, fun, varname)                                    \
 public:                                                            \
  GLuint fname##_##varname() const { return fname##_##varname##_; } \
                                                                    \
 private:                                                           \
  GLuint fname##_##varname##_

  VAR(renderTriangles, glGetUniformLocation, MVP);
  VAR(renderTriangles, glGetUniformLocation, M);
  VAR(renderTriangles, glGetUniformLocation, lightPosition);
  VAR(renderTriangles, glGetUniformLocation, clippingPlanes);
  VAR(renderTriangles, glGetUniformLocation, clippingDirections);
  VAR(renderTriangles, glGetUniformLocation, cuttingLimit);
  VAR(renderTriangles, glGetUniformLocation, cuttingMode);
  VAR(renderTriangles, glGetUniformLocation, numClipDistances);
  VAR(renderTriangles, glGetUniformLocation, invertColor);
  VAR(renderTriangles, glGetUniformLocation, highlightedTriangle);
  VAR(renderTriangles, glGetUniformLocation, defaultFrontColor);
  VAR(renderTriangles, glGetUniformLocation, defaultBackColor);
  VAR(renderTriangles, glGetAttribLocation, vertexPosition_modelspace);
  VAR(renderTriangles, glGetAttribLocation, vertexNormal);

  VAR(pickTriangles, glGetAttribLocation, vertexPosition_modelspace);
  VAR(pickTriangles, glGetUniformLocation, MVP);
  VAR(pickTriangles, glGetUniformLocation, gObjectIndex);
  VAR(pickTriangles, glGetUniformLocation, gDrawIndex);

#undef VAR

 public:
  // The surfaceAttributes value is used to get the list (and kind) of
  // attributes available to the shader. It can be nullptr if there are no
  // attributes.
  // throws Exception
  Vis3DShaders(QOpenGLFunctions* functions,
               const QSharedPointer<vx::SurfaceData>& surfaceAttributes,
               const QString& shadingTechnique);

  ~Vis3DShaders();

  int maxClipDistances() const { return maxClipDistances_; }

  QOpenGLShaderProgram& renderTriangles() { return renderTriangles_; }
  QOpenGLShaderProgram& pickTriangles() { return pickTriangles_; }
};
