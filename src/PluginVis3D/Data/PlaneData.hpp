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

#include <QObject>
#include <QVector4D>
#include <QVector>

#include <PluginVis3D/Data/CuttingPlane.hpp>
#include <PluginVis3D/Data/Texture.hpp>
#include <QOpenGLBuffer>
#include <Voxie/Data/Color.hpp>
#include <Voxie/Data/Colorizer.hpp>
#include <Voxie/Data/Slice.hpp>
#include <Voxie/PropertyObjects/PlaneNode.hpp>

using namespace vx;

const QVector<QVector4D> planeColorList = {
    QVector4D(1.0, 0.0, 0.0, 0.3), QVector4D(0.0, 1.0, 0.0, 0.3),
    QVector4D(0.0, 0.0, 1.0, 0.3), QVector4D(1.0, 1.0, 0.0, 0.3),
    QVector4D(1.0, 0.0, 1.0, 0.3), QVector4D(0.0, 1.0, 1.0, 0.3),
    QVector4D(1.0, 1.0, 1.0, 0.3)};
static int planeColorCounter = 0;

/**
 * @brief The PlaneData class contains the cutting direction settings, the
 * vertex buffers and the texture setup of a slice. The texture setup includes
 * the resolution, a slice dependent @link Colorizer instance and the texture
 * itself.
 * @author Robin Höchster
 */
class PlaneData : public QObject {
  Q_OBJECT
 private:
  QSharedPointer<PlaneNode> plane;
  QSharedPointer<Slice> slice;
  QSharedPointer<QOpenGLBuffer> vertexBuffer;
  QSharedPointer<Texture> texture;
  bool _drawTexture = false;
  int sliceResolution = 512;
  // TODO: Move to texture and create only if needed?
  QSharedPointer<Colorizer> _colorizer;

  /**
   * @brief color Index of the selected Color in the planeColorList
   */
  int color;

  CuttingDirection _cuttingDirection = None;

 public:
  static Color nextColor() {
    if (planeColorCounter >= planeColorList.size()) {
      planeColorCounter = 0;
    }
    auto color = planeColorCounter++;
    return Color(planeColorList.at(color));
  }

  PlaneData(QSharedPointer<PlaneNode> plane,
            QSharedPointer<QOpenGLBuffer> vertexBuffer,
            QSharedPointer<Colorizer> colorizer)
      : plane(plane), vertexBuffer(vertexBuffer), _colorizer(colorizer) {
    if (planeColorCounter >= planeColorList.size()) {
      planeColorCounter = 0;
    }
    color = planeColorCounter++;
  }

  /**
   * @brief setCuttingDirection sets the given cutting direction or does nothing
   * if the mode is already set.
   * @param newDirection
   */
  void setCuttingDirection(CuttingDirection newDirection) {
    if (_cuttingDirection == newDirection) return;
    if (!Cutting::isValidDirection(newDirection))
      throw "Invalid Cutting Direction";
    _cuttingDirection = newDirection;
    Q_EMIT cuttingDirectionChanged();
  }

  /**
   * @brief cuttingDirection returns the current cutting direction.
   * @return
   */
  CuttingDirection cuttingDirection() const { return _cuttingDirection; }

  /**
   * @brief getVertexBuffer returns the vertex buffer belonging to the slice.
   * @return
   */
  const QSharedPointer<QOpenGLBuffer> getVertexBuffer() const {
    return vertexBuffer;
  }

  /**
   * @brief getTexture returns the @link Texture that contains the pixel data
   * projected onto the slice.
   * @return
   */
  const QSharedPointer<Texture> getTexture() const { return texture; }

  /**
   * @brief getPlane returns the plane this object blongs to.
   * @return
   */
  const QSharedPointer<PlaneNode> getPlane() const { return plane; }

  const QSharedPointer<Slice> getSlice() const { return slice; }
  void setSlice(QSharedPointer<Slice> slice) {
    this->slice = slice;
    Q_EMIT drawTextureFlagChanged();
  }
  void removeSlice() {
    this->slice.clear();
    Q_EMIT drawTextureFlagChanged();
  }

  /**
   * @brief drawTexture states if the texture should be draw onto the slice.
   * @return
   */
  bool drawTexture() const { return _drawTexture; }

  /**
   * @brief resolution returns the pixel resolution of the output slice texture.
   * @return
   */
  int resolution() const { return sliceResolution; }

  /**
   * @brief setDrawTexture sets the draw texture flag statung if the texture
   * should be draw onto the slice.
   * @param draw true, if the texture should be drawn, false if not.
   * @return
   */
  void setDrawTexture(bool draw) {
    if (draw == _drawTexture) return;
    _drawTexture = draw;
    Q_EMIT drawTextureFlagChanged();
  }

  /**
   * @brief setTexture changes the slice texture to the given one.
   * @param texture
   */
  void setTexture(QSharedPointer<Texture> texture) { this->texture = texture; }

  /**
   * @brief setSliceResolution sets the quadratic pixel resolution of the slice
   * texture. The resolution should be a number to the power of two. The output
   * texture is always quadratic, for example 256 x 256.
   * @param resolution
   */
  void setSliceResolution(int resolution) {
    if (resolution < 2) throw "Invalid resolution";
    // TODO: Check if resolution is valid (base 2)
    if (sliceResolution == resolution) return;
    sliceResolution = resolution;
    Q_EMIT sliceResolutionChanged();
  }

  /**
   * @brief colorizer returns the @link Colorizer object that is asociated with
   * the slice texture.
   * @return
   */
  QSharedPointer<Colorizer> colorizer() { return _colorizer; }

  /**
   * @brief getBoundingRectangle Returns the Bounding Box of the Plane. This
   * depends on the currently connected Slice.
   * @return
   */
  QRectF getBoundingRectangle() { return QRectF(-0.1, -0.1, 0.2, 0.2); }

  QVector4D getColor() { return planeColorList.at(color); }

 Q_SIGNALS:
  /**
   * @brief cuttingDirectionChanged is emitted when the cutting direction has
   * changed.
   */
  void cuttingDirectionChanged();

  /**
   * @brief drawTextureFlagChanged is emitted when the draw texture flag is
   * changed. @see setDrawTexture();
   */
  void drawTextureFlagChanged();

  /**
   * @brief sliceResolutionChanged is emitted when the slice resolution has
   * changed. @see setSliceResolution();
   */
  void sliceResolutionChanged();
};
