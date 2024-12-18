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

#include "TextLayer.hpp"

#include <PluginVisSlice/SliceVisualizer.hpp>

#include <Voxie/Node/ParameterCopy.hpp>

using namespace vx;

TextLayer::TextLayer(SliceVisualizer* sv) : sv(sv) {
  for (const auto& layer : sv->layers())
    QObject::connect(layer.data(), &Layer::isUpToDateChanged, this,
                     &TextLayer::update);
}

void TextLayer::update() {
  QList<QString> updatingLayers;
  for (const auto& layer : sv->layers()) {
    if (layer.data() != this && !layer->isUpToDate())
      updatingLayers.push_back(layer->displayName());
  }

  QMutexLocker lock(&mutex);
  if (this->updatingLayers != updatingLayers) {
    this->updatingLayers = updatingLayers;
    this->triggerRedraw();
  }
}

void TextLayer::render(QImage& outputImage,
                       const QSharedPointer<vx::ParameterCopy>& parameters,
                       bool isMainImage) {
  if (!isMainImage) return;

  SlicePropertiesCopy properties(
      parameters->properties()[parameters->mainNodePath()]);

  QList<QString> updatingLayers;
  {
    QMutexLocker lock(&mutex);
    updatingLayers = this->updatingLayers;
  }

  QPainter painter(&outputImage);

  QFont font = painter.font();
  font.setBold(true);
  painter.setPen(Qt::red);
  font.setPointSize(10);
  painter.setFont(font);

  QString text;

  if (0) {
    static QMutex idMutex;
    static uint64_t i = 0;
    QMutexLocker idLocker(&idMutex);
    i++;
    text += "ID " + QString::number(i);
  }

  if (0)
    for (const auto& layer : updatingLayers) text += "\n" + layer;
  else
    text += updatingLayers.size() ? "Updating" : "";

  painter.drawText(outputImage.rect(), Qt::AlignTop | Qt::AlignLeft, text);
}
