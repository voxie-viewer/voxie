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

#include <PluginVisSlice/Layer.hpp>

#include <PluginVisSlice/MultivariateDataWidget/helpingStructures.hpp>

class SliceVisualizer;

class LegendLayer : public Layer {
  Q_OBJECT
  VX_REFCOUNTEDOBJECT

 private:
  QLinearGradient hueGradient;
  QLinearGradient satGradient;
  QLinearGradient valGradient;
  void buildHsvComponentGradient();

  /**Gives width and heigth for the given text
   *  if drawn with the given font.
   * @param QFont: Font with font size and font type
   * @param QString: Text that should be drawn
   * @returns QPair<int,int>: Pair of int width and int height of
   * the bounding rect of the text drawn with the given font
   */
  QPair<int, int> getTextWidthHeight(QFont font, QString text);

  /**
   * This methode draws a legend into the given QImage.
   * This legend shows the mapping from source scale into target scale.
   * The used scaling variables can be manipulated in the multivariatedata
   * widget in the sidebar of the slice visualizer. If width >= height legend is
   * drawn horizontal hard coded. Else it is drawn vertical in a more dynamic
   * way that includes additional ticks for intermediate values, dynamic heights
   * adjustment.
   * @param QImage* outputImage: Image which the legend gets drawn into.
   * @param SliceVisualizer* sv: pointer to slice visualizer.
   * @param QPoint upperLeft: Point of the upper left corner of the legend. This
   * point coordinates are in Image-Plane of outputImage.
   * @param qreal colorbarWidth: Width of the color bar in the legend.
   * @param qreal colorbarHeight: Height of the color bar in the legend.
   */
  void drawLegend(QImage* outputImage, SliceVisualizer* sv, QPoint upperLeft,
                  qreal colorbarWidth, qreal ColorbarHeight);

 public:
  LegendLayer(SliceVisualizer* sv);
  ~LegendLayer() {}

  QString getName() override {
    return "de.uni_stuttgart.Voxie.SliceVisualizer.Layer.Legend";
  }

  void render(QImage& outputImage,
              const QSharedPointer<vx::ParameterCopy>& parameters,
              bool isMainImage) override;

 private:
  SliceVisualizer* sv;
};
