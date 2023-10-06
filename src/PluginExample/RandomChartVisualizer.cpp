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

#include "RandomChartVisualizer.hpp"

#include <PluginExample/Prototypes.hpp>

#include <VoxieBackend/Data/ImageDataPixel.hpp>

#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>

#include <QtWidgets/QFormLayout>
#include <QtWidgets/QLabel>

using namespace vx::visualization;
using namespace vx;

RandomChartVisualizer::RandomChartVisualizer()
    : SimpleVisualizer(getPrototypeSingleton()) {
  this->view()->setMinimumSize(300 / 96.0 * this->view()->logicalDpiX(),
                               200 / 96.0 * this->view()->logicalDpiY());

  // Init function + section
  {
    QFormLayout* layout = new QFormLayout();

    QLabel* name = new QLabel("f(x) = ");
    QLabel* func = new QLabel("1");

    static int i = 0;
    switch ((i++) % 4) {
      case 0:
        func->setText("x");
        this->f = [](float x) -> float { return x; };
        break;
      case 1:
        func->setText("x²");
        this->f = [](float x) -> float { return x * x; };
        break;
      case 2:
        func->setText("tan(0.5 * x)");
        this->f = [](float x) -> float { return std::tan(0.5 * x); };
        break;
      case 3:
        func->setText("sin(x)");
        this->f = [](float x) -> float { return std::sin(x); };
        break;
      default:
        this->f = [](float x) -> float {
          (void)x;
          return 1;
        };
        break;
    }
    this->setAutomaticDisplayName("Chart for " + func->text());

    layout->addRow(name, func);

    form = new QWidget();
    form->setLayout(layout);
    form->setWindowTitle("Function Information");
  }

  this->dynamicSections().append(this->form);
}
RandomChartVisualizer::~RandomChartVisualizer() {}

vx::SharedFunPtr<VisualizerNode::RenderFunction>
RandomChartVisualizer::getRenderFunction() {
  // TODO: f probably should be in the parameters
  return [f = this->f](
             const QSharedPointer<vx::ImageDataPixel>& outputImage,
             const vx::VectorSizeT2& outputRegionStart,
             const vx::VectorSizeT2& size,
             const QSharedPointer<vx::ParameterCopy>& parameters,
             const QSharedPointer<vx::VisualizerRenderOptions>& options) {
    Q_UNUSED(parameters);
    Q_UNUSED(options);

    quint64 width = size.x;
    quint64 height = size.y;

    QImage qimage(width, height, QImage::Format_ARGB32);
    qimage.fill(qRgba(0, 0, 0, 0));  // Fill will transparent

    QPainter painter(&qimage);

    painter.fillRect(0, 0, width, height, QColor(255, 255, 255));

    float zoom = 15;
    float centerX = 0.5f * width;
    float centerY = 0.5f * height;

    painter.drawLine(centerX, 0, centerX, height);
    painter.drawLine(0, centerY, width, centerY);

    int ticksX = width / zoom + 1;
    int ticksY = height / zoom + 1;
    for (int t = -ticksX; t <= ticksX; t++) {
      painter.drawLine(centerX + t * zoom, centerY - 2, centerX + t * zoom,
                       centerY + 2);
    }
    for (int t = -ticksY; t <= ticksY; t++) {
      painter.drawLine(centerX - 2, centerY + t * zoom, centerX + 2,
                       centerY + t * zoom);
    }

    float delta = 1.0f / zoom;
    for (float x = -ticksX; x < ticksX; x += delta) {
      float y0 = f(x);
      float y1 = f(x + delta);

      painter.drawLine(centerX + x * zoom, centerY - zoom * y0,
                       centerX + (x + delta) * zoom, centerY - zoom * y1);
    }

    outputImage->fromQImage(qimage, outputRegionStart);
  };
}

NODE_PROTOTYPE_IMPL_2(RandomChart, Visualizer)
