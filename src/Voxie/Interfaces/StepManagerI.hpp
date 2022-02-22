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

#include <QtCore/QList>
#include <QtGui/QVector3D>
#include <Voxie/Interfaces/SliceVisualizerI.hpp>
#include <Voxie/Node/Node.hpp>
#include <Voxie/Voxie.hpp>
#include <VoxieBackend/Data/PlaneInfo.hpp>

namespace vx {

class VOXIECORESHARED_EXPORT StepManagerI {
 public:
  StepManagerI();
  virtual ~StepManagerI();
  /**
   * @brief Sets the BrushSelectionStep properties (Plane, Volume) of the Step
   * related to the currentSV
   * @param plane Plane in which the Brush operates
   * @param currentSV SliceVisualizer to which the brush belongs
   */
  virtual void setBrushSelectionProperties(PlaneInfo plane,
                                           SliceVisualizerI* currentSV) = 0;

  /**
   * @brief Sets or clears the selection bit of the voxels around the center of
   * the brush. Behaviour depends on the choice of Brush or Eraser in the GUI
   * @param centerWithRadius Center of Brush [m] (global system) and Brush
   * @param plane Plane in which the Brush operates
   * @param currentSV SliceVisualizer to which the brush belongs
   * Radius [m]
   */
  virtual void addVoxelsToBrushSelection(
      std::tuple<QVector3D, double> centerWithRadius, PlaneInfo plane,
      SliceVisualizerI* currentSV) = 0;

  /**
   * @brief Creates a new Lasso selection step.
   * @param nodes Nodes that span the simple Polygon
   * @param plane Plane in which the polygon is placed
   * @param currentSV SliceVisualizer to which the brush belongs
   */
  virtual void setLassoSelection(QList<QVector3D> nodes, PlaneInfo plane,
                                 SliceVisualizerI* currentSV) = 0;

  /**
   * @brief creates an MetaStep and starts calculation for the
   * given label id
   * @param labelID label id that shall be modified
   * @param key key of the property to modify
   * @param value new value that shall be assigned to the key
   */
  virtual void createMetaStep(qlonglong labelID, QString key,
                              QVariant value) = 0;
};
}  // namespace vx
