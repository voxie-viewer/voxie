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

#include <PluginVisSlice/GridSizeMode.hpp>  // TODO: avoid cross-object include
#include <PluginVisSlice/SizeUnit.hpp>      // TODO: avoid cross-object include

#include <QColor>
#include <QObject>

#include <Voxie/Vis/OpenGLWidget.hpp>

/**
 * @brief This class contains the logic for the grid.
 */
class Grid3D : public QObject {
  Q_OBJECT
 private:
  bool active;
  QColor color;
  GridSizeMode mode;
  float size;
  SizeUnit lengthUnit;
  int opacity;
  bool xyPlane;
  bool xzPlane;
  bool yzPlane;

  typedef vx::visualization::OpenGLDrawWidget::PrimitiveBuffer PrimitiveBuffer;

  /**
   * @brief setGridSizeForSelf is an setter which sets the size of the grid mesh
   * width and should clled only by grid3d itself. This is necessary to prevent
   * endlessloops.
   * @param float value
   */
  void setGridSizeForSelf(float value);

  /**
   * @brief setUnitForSelf is an setter which sets the unit of the grid mesh
   * width and should clled only by grid3d itself. This is necessary to prevent
   * endlessloops.
   * @param float value
   */
  void setUnitForSelf(SizeUnit unit);

  /**
   * @brief Gives the factor coresponding to the unit
   * @return the unit factor
   */
  float unit();

  /**
   * @brief Ensures that the grid is no longer visible.
   */
  void removeGrid();

  /**
   * @brief setBiggerUnit if current unit is not the biggest unit, it sets the
   * unit next bigger unit
   */
  void setBiggerUnit();

  /**
   * @brief setSmallerUnit if current unit is not the smallest unit, it sets the
   * unit next smaller unit
   */
  void setSmallerUnit();

 public:
  /**
   * @brief Draws grid over Slice with current parameter of the grid.
   */
  void drawGrid(PrimitiveBuffer& drawingBuffer, const QVector3D& min,
                const QVector3D& max, float prefLength);

  /**
   * @brief Grid3D constructor
   * @param Isovisualizer* which is converted to an QObject* isoVis
   */
  explicit Grid3D(QObject* isoVis);

  Grid3D(Grid3D const&) = delete;
  void operator=(Grid3D const&) = delete;

  /**
   * @brief is true if grid is active.
   * @param bool active
   */
  void setActive(bool active);

  /**
   * @brief setColor sets the current color of the Grid.
   * @param QColor color
   */
  void setColor(QColor color);

  /**
   * @brief setMode sets the current grid mode.
   * @param GridSizeMode mode
   */
  void setMode(GridSizeMode mode);

  /**
   * @brief setSizeForWidget is a sette of the grid mesh width.
   * This sette should called only by othe classes.
   * @param float size
   */
  void setSizeForWidget(float size);

  /**
   * @brief setUnitForWidget is a setter of the grid mesh width unit.
   * This sette should called only by othe classes.
   * @param SizeUnit unit
   */
  void setUnitForWidget(SizeUnit unit);

  /**
   * @brief setOpacity is a setter of the grids opacity.
   * @param int opacity (Range: 0 to 255)
   */
  void setOpacity(int opacity);

  /**
   * @brief setXYPlane is a Setter for the xy plane. Only active plane will be
   * drawn.
   * @param bool checked
   */
  void setXYPlane(bool checked);

  /**
   * @brief setXZPlane is a Setter for the xz plane. Only active plane will be
   * drawn.
   * @param bool checked
   */
  void setXZPlane(bool checked);

  /**
   * @brief setYZPlane is a Setter for the yz plane. Only active plane will be
   * drawn.
   * @param bool checked
   */
  void setYZPlane(bool checked);

  /**
   * @brief getSize is a getter for the gird mesh width.
   * @return float size
   */
  float getSize();

  /**
   * @brief If called checks if active Parameter of grid is true.
   * If its true drawGrid is called. Otherwise removeGrid is called.
   */
  void updateGrid();

  /**
   * @brief unitToString returns a String thats the representation of the
   * current unit
   * @return a String thats the representation of the current unit
   */
  QString unitToString();

 Q_SIGNALS:

  /**
   * @brief unitChanged is an signal which is called if the grid length unit has
   * changed.
   * @param SizeUnit newValue
   */
  void unitChanged(SizeUnit newValue);

  /**
   * @brief sizeChanged is an signal which is called if the grid mesh width
   * changed.
   * @param float newValue
   */
  void sizeChanged(float newValue);

  /**
   * @brief gridChanged is an signal which is called if the grid has changed.
   */
  void gridChanged();

 public Q_SLOTS:
};
