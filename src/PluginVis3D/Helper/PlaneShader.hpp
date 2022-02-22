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

#include <QtGui/QOpenGLBuffer>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QOpenGLVertexArrayObject>

#include <PluginVis3D/Data/PlaneData.hpp>
#include <Voxie/Data/Slice.hpp>

using namespace vx;

/**
 * @brief The PlaneShader class contains the special plane shaders and helper
 * functions to setup the vertex arrays and draw the slices.
 * @author Robin Höchster
 */
class PlaneShader : protected QOpenGLFunctions {
 public:
  PlaneShader();
  /**
   * @brief initialize loads the slice shaders, retrieves the IDs of the uniform
   * and input variables and sets up the uv buffer.
   * @return
   */
  QString initialize();

 private:
  QOpenGLShaderProgram textureProgram;
  QOpenGLShaderProgram planeProgram;

  // Texture Shader IDs:
  GLint MVP_ID;
  GLint vertexPosition_modelspaceID;
  GLint vertexUVID;
  GLint sliceTextureID;
  GLint sliceColorID;

  // Slice Shader IDs:
  GLint sliceShader_MVP_ID;
  GLint sliceShader_vertexPosition_modelspaceID;
  GLint sliceShader_sliceColorID;

  QOpenGLBuffer uvBuffer;

 public:
  /**
   * @brief updateBuffers Re-constructs the vertex buffer.
   * @param planeData
   */
  void updateBuffers(QSharedPointer<PlaneData> planeData);
  void updateBuffers(QSharedPointer<PlaneData> planeData, const QRectF& bbox);

  /**
   * @brief setupUVBuffer sets up the uv buffer which is identical for each
   * plane. Make sure this method is called in the correct OpenGL context (Has
   * to be the same as the one when @link draw() is being called.
   * @return an empty QString on success or a QString containing an error
   * message.
   */
  QString setupUVBuffer();

  /**
   * @brief draw draws the slice contained by the given slice data. The
   * transformation of the slice must already been applied to the given
   * projection matrix.
   * @param planeData
   * @param modelViewProjectionMatrix The eventual transformation of the slice,
   * including all view transformations and the local slice transformations.
   * @param color
   */
  void draw(QSharedPointer<PlaneData> planeData,
            const QMatrix4x4& modelViewProjectionMatrix,
            const QVector4D& color);

 private:
  void drawSimple(QSharedPointer<PlaneData> planeData,
                  const QMatrix4x4& modelViewProjectionMatrix,
                  const QVector4D& color);
  void drawTexture(QSharedPointer<PlaneData> planeData,
                   const QMatrix4x4& modelViewProjectionMatrix,
                   const QVector4D& color);

  QString loadSimpleShaders();
  QString loadTextureShaders();
};
