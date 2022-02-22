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

#ifndef GRID_H
#define GRID_H

#include <QColor>
#include <QObject>
#include <QPushButton>

#include <PluginVisSlice/Layer.hpp>

#include <PluginVisSlice/GridSizeMode.hpp>
#include <PluginVisSlice/SizeUnit.hpp>

class SliceVisualizer;

/**
 * @brief The grid class provides the functionality for the grid above the
 * slice. It receives changes to the grid settings from the gridWidget.
 */
class Grid : public Layer {
  Q_OBJECT
  REFCOUNTEDOBJ_DECL(Grid)

 public:
  explicit Grid(SliceVisualizer*);

  ~Grid();

  Grid(Grid const&) = delete;
  void operator=(Grid const&) = delete;

  QString getName() override {
    return "de.uni_stuttgart.Voxie.SliceVisualizer.Layer.Grid";
  }

  /**
   * @brief Grid::getSize
   * @return the grid mesh width
   */
  float getSize();

  void render(QImage& outputImage,
              const QSharedPointer<vx::ParameterCopy>& parameters) override;
};

#endif  // GRID_H
