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

#include "GaussFilter2D.hpp"

#include <Voxie/Component/MetaFilter2D.hpp>

#include <math.h>

#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>

using namespace vx;
using namespace vx::plugin;

GaussFilter2D::GaussFilter2D(vx::plugin::MetaFilter2D* metaFilter)
    : vx::filter::Filter2D(metaFilter) {}

GaussFilter2D::~GaussFilter2D() {}

void GaussFilter2D::calcGaussKernel() {
  this->sigma = ((float)(this->radius * 2) + 1) / 3;
  this->kernelSize = this->radius * 2 + 1;
  this->gaussKernel = QVector<float>(this->kernelSize * this->kernelSize);

  float kernelSum = 0;  // for normalization

  float factor = 1 / (2 * M_PI * this->sigma * this->sigma);
  for (int x = 0; x < this->kernelSize; x++) {
    for (int y = 0; y < this->kernelSize; y++) {
      int xVal = kernelSize / 2 - x;
      int yVal = kernelSize / 2 - y;
      float expVal =
          -(xVal * xVal + yVal * yVal) / (2 * this->sigma * this->sigma);
      this->gaussKernel[x + this->kernelSize * y] = factor * exp(expVal);
      kernelSum += factor * exp(expVal);
    }
  }
  // normalize kernel
  float invSum = 1 / kernelSum;
  for (int x = 0; x < this->kernelSize; x++) {
    for (int y = 0; y < this->kernelSize; y++) {
      this->gaussKernel[x + this->kernelSize * y] *= invSum;
    }
  }
}

void GaussFilter2D::applyTo(vx::FloatImage input, vx::FloatImage output) {
  calcGaussKernel();
  for (size_t x = 0; x < input.getWidth(); x++) {
    for (size_t y = 0; y < input.getHeight(); y++) {
      float value = 0;
      if (std::isnan(input.getPixel(x, y))) {  // NaN
        output.setPixel(x, y, NAN);
        continue;
      }
      float scaling = this->kernelSize * this->kernelSize;  // to balance NaNs
      for (size_t i = 0; i < (size_t)this->kernelSize; i++) {
        for (size_t j = 0; j < (size_t)this->kernelSize; j++) {
          long long xPos = x + i - kernelSize / 2;
          long long yPos = y + j - kernelSize / 2;
          if (xPos >= 0 && xPos < (long long)input.getWidth() && yPos >= 0 &&
              yPos < (long long)input.getHeight()) {
            float pixelVal = input.getPixel(xPos, yPos);
            if (!std::isnan(pixelVal)) {  // checking NAN
              value +=
                  this->gaussKernel[(int)(i + j * this->kernelSize)] * pixelVal;
            } else {
              scaling -= 1.0;
            }
          }
        }
      }
      scaling = float(this->kernelSize * this->kernelSize) / scaling;
      output.setPixel(x, y, value * scaling);
    }
  }
}

void GaussFilter2D::applyTo(vx::SliceImage input, vx::SliceImage output) {
  applyTo((FloatImage)input, (FloatImage)output);
}

QDialog* GaussFilter2D::getSettingsDialog() {
  if (this->dialog == nullptr) {
    this->dialog = new QDialog();
    this->dialog->setLayout(new QBoxLayout(QBoxLayout::LeftToRight));
    this->dialog->layout()->addWidget(new QLabel("radius:"));
    this->spinBox = new QSpinBox();
    this->spinBox->setValue(this->radius);
    this->dialog->layout()->addWidget(this->spinBox);
    QPushButton* button = new QPushButton("Update");
    this->dialog->layout()->addWidget(button);
    connect(button, &QPushButton::clicked, this,
            &GaussFilter2D::updateSettings);
  }
  return dialog;
}

void GaussFilter2D::updateSettings() {
  if (this->spinBox != nullptr) {
    this->radius = this->spinBox->value();
  }
  Q_EMIT this->filterChanged(this);
}

QXmlStreamAttributes GaussFilter2D::exportFilterSettingsXML() {
  QXmlStreamAttributes attributes;
  attributes.append("radius", QString::number(this->radius));
  return attributes;
}

void GaussFilter2D::importFilterSettingsXML(QXmlStreamAttributes attributes) {
  this->radius = attributes.value("radius").toInt();
}

vx::filter::Filter2D* GaussMetaFilter2D::createFilter() {
  return new GaussFilter2D(this);
}

QString GaussMetaFilter2D::displayName() { return "Gauss Filter"; }
