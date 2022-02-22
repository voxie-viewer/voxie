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

#include <QOpenGLBuffer>
#include <QQuaternion>
#include <QtCore/QSharedPointer>
#include <Voxie/Data/SurfaceNode.hpp>
#include <VoxieBackend/Data/SurfaceData.hpp>

/**
 * @brief The SurfaceData3D class is a data class that contains temporary
 * information about the surface like visibility and the OpenGL vertex and color
 * buffer.
 */
class SurfaceData3D : public QObject {
  Q_OBJECT
 private:
  QSharedPointer<vx::SurfaceNode> surfacenode;
  QSharedPointer<vx::SurfaceDataTriangleIndexed> surface;
  QSharedPointer<vx::SurfaceDataTriangleIndexed> surfaceMod_;

  QSharedPointer<QOpenGLBuffer> indexBuffer;
  QSharedPointer<QOpenGLBuffer> vertexBuffer;
  QSharedPointer<QOpenGLBuffer> normalBuffer;

  std::map<QString, QSharedPointer<QOpenGLBuffer>> extraBuffers;

  bool _isVisible = true;
  bool _isHighlighted;
  bool _invertColor;

 public:
  SurfaceData3D(const QSharedPointer<vx::SurfaceNode>& surfacenode,
                const QSharedPointer<QOpenGLBuffer>& indexBuffer,
                const QSharedPointer<QOpenGLBuffer>& vertexBuffer,
                const QSharedPointer<QOpenGLBuffer>& normalBuffer)
      : surfacenode(surfacenode),
        surface(qSharedPointerDynamicCast<vx::SurfaceDataTriangleIndexed>(
            surfacenode->surface())),
        indexBuffer(indexBuffer),
        vertexBuffer(vertexBuffer),
        normalBuffer(normalBuffer) {}

  /**
   * @brief getSurface returns the surface this object belongs to.
   * @return
   */
  const QSharedPointer<vx::SurfaceNode>& getSurfaceNode() {
    return surfacenode;
  }

  /**
   * @brief getSurface returns the surface this object belongs to.
   * @return
   */
  const QSharedPointer<vx::SurfaceDataTriangleIndexed>& getSurface() {
    return surface;
  }

  /**
   * @brief Return a surface with the same triangles as the original one (with
   * the triangles in the same order) where the ID of the controlling vertex
   * is always the same as the ID of the triangle.
   */
  const QSharedPointer<vx::SurfaceDataTriangleIndexed>& createSurfaceModified();

  /**
   * @brief getIndexBuffer returns the index buffer that contains the index data
   * of the surface.
   * @return
   */
  const QSharedPointer<QOpenGLBuffer> getIndexBuffer() { return indexBuffer; }

  /**
   * @brief getVertexBuffer returns the vertex buffer that contains the vertex
   * data of the surface.
   * @return
   */
  const QSharedPointer<QOpenGLBuffer> getVertexBuffer() { return vertexBuffer; }

  /**
   * @brief getNormalBuffer returns the normal buffer that contains the normals
   * of the surface.
   * @return
   */
  const QSharedPointer<QOpenGLBuffer> getNormalBuffer() { return normalBuffer; }

  /**
   * @brief getExtraDataBuffer returns the map containing all buffers for the
   * extra data
   * @return
   */
  std::map<QString, QSharedPointer<QOpenGLBuffer>>& attributeBuffer() {
    return extraBuffers;
  }

  /**
   * @brief isVisible returns true if the surface is marked hidden, false if it
   * is marked visible.
   * @return
   */
  bool isVisible() { return _isVisible; }

  /**
   * @brief setVisibility sets the visibility of the surface.
   * @param isVisible true makes the surface visible, false hidden.
   */
  void setVisibility(bool isVisible);

  /**
   * @brief isColorInverted states if the surface should be drawn highlighted.
   * @return
   */
  bool isHighlighted() { return _isHighlighted; }

  /**
   * @brief setHighlighted Sets the highlight status of this object to true.
   * If the object's status was already like that, nothing is changed and no
   * @link changed(); signal is emitted.
   * @param highlight
   */
  void setHighlighted(bool highlight) {
    if (_isHighlighted == highlight) return;
    _isHighlighted = highlight;
    Q_EMIT changed();
  }

  /**
   * @brief isColorInverted states if the surface should be drawn with inverted
   * colors.
   * @return
   */
  bool isColorInverted() { return _invertColor; }

  /**
   * @brief invertColor sets the color inversion flag to true or false.
   * @param invert
   */
  void invertColor(bool invert) {
    if (_invertColor == invert) return;
    _invertColor = invert;
    Q_EMIT changed();
  }

 Q_SIGNALS:
  void visibilityChanged(SurfaceData3D* surfaceData);
  void changed();
};
