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

#include "HistogramVisualizerWidget.hpp"
#include "LabeledAxis.hpp"

#include <QPainter>

using namespace vx;

HistogramVisualizerWidget::HistogramVisualizerWidget(QWidget* parent)
    : QWidget(parent) {
  this->setWindowTitle("Histogram");
  setMouseTracking(true);
}

void HistogramVisualizerWidget::setYAxisLogScale(const bool yAxisLogScale) {
  this->yAxisLogScale = yAxisLogScale;
  repaint();
}

bool HistogramVisualizerWidget::isYAxisLogScale() const {
  return yAxisLogScale;
}

QSharedPointer<Colorizer> HistogramVisualizerWidget::colorizer() const {
  return colorizer_;
}

void HistogramVisualizerWidget::setColorizer(
    const QSharedPointer<Colorizer>& colorizer) {
  this->colorizer_ = colorizer;
  repaint();
}

QSharedPointer<HistogramProvider> HistogramVisualizerWidget::histogramProvider()
    const {
  return histogramProvider_;
}

void HistogramVisualizerWidget::setHistogramProvider(
    const QSharedPointer<HistogramProvider>& histogramProvider) {
  if (!histogramProvider_.isNull()) {
    disconnect(histogramProvider_.data(), &HistogramProvider::dataChanged, this,
               &HistogramVisualizerWidget::onHistogramProviderDataChanged);
  }
  this->histogramProvider_ = histogramProvider;
  connect(histogramProvider_.data(), &HistogramProvider::dataChanged, this,
          &HistogramVisualizerWidget::onHistogramProviderDataChanged);
  repaint();
}

void HistogramVisualizerWidget::setForegroundColor(
    const QColor foregroundColor) {
  foregroundColor_ = foregroundColor;
  repaint();
}

QColor HistogramVisualizerWidget::foregroundColor() const {
  return foregroundColor_;
}

void HistogramVisualizerWidget::setBackgroundColor(
    const QColor backgroundColor) {
  backgroundColor_ = backgroundColor;
  repaint();
}

QColor HistogramVisualizerWidget::backgroundColor() const {
  return backgroundColor_;
}

void HistogramVisualizerWidget::setYAxisLabel(const QString label) {
  yAxisLabel_ = label;
  repaint();
}

QString HistogramVisualizerWidget::yAxisLabel() const { return yAxisLabel_; }

void HistogramVisualizerWidget::setXAxisLabel(const QString label) {
  xAxisLabel_ = label;
  repaint();
}

QString HistogramVisualizerWidget::xAxisLabel() const { return xAxisLabel_; }

void HistogramVisualizerWidget::setHoverValueSnappingEnabled(bool enabled) {
  hoverValuesSnapping = enabled;
}

bool HistogramVisualizerWidget::isHoverValueSnappingEnabled() const {
  return hoverValuesSnapping;
}

void HistogramVisualizerWidget::paintEvent(QPaintEvent* event) {
  Q_UNUSED(event);

  static const float axisMarginsBL = 40.f;
  static const float axisMarginsTR = 5.f;

  QRectF rect(axisMarginsBL, axisMarginsTR,
              width() - axisMarginsTR - axisMarginsBL,
              height() - axisMarginsTR - axisMarginsBL);

  QPainter painter(this);

  painter.fillRect(0, 0, width(), height(), backgroundColor());

  if (histogramProvider_.isNull()) return;

  painter.setClipRect(rect);

  auto histogramData = histogramProvider_->getData();

  if (histogramData->buckets.size() == 0) return;

  float bucketStepSize =
      (histogramData->maximumValue - histogramData->minimumValue) /
      histogramData->buckets.size();

  // lower and upper bound are just the lowest and highest index of the bucket
  // vector if manual override isn't enabled
  unsigned int bucketLowerBound =
      automaticBucketLowerBound ? 0 : bucketLowerBoundOverride;
  unsigned int bucketUpperBound = automaticBucketUpperBound
                                      ? histogramData->buckets.size()
                                      : bucketUpperBoundOverride;
  unsigned int bucketCount = bucketUpperBound - bucketLowerBound;

  quint64 maximumCount = histogramData->maximumCount;
  // we have to find the maximumcount of the part of the histogram that is
  // displayed if not the whole histogram is displayed
  if (!automaticBucketLowerBound || !automaticBucketUpperBound) {
    maximumCount = 0;
    for (int i = bucketLowerBound; i < histogramData->buckets.size(); i++) {
      if (histogramData->buckets[i] > maximumCount)
        maximumCount = histogramData->buckets[i];
    }
  }

  quint64 upperBound = automaticUpperBound ? maximumCount : manualUpperBound;

  // function to determine original (non-logarithmic) value boundaries
  auto unscaleX = [&](float v) {
    return histogramData->xAxisLog ? expf(v) : v;
  };

  auto scaleY = [&](float v) { return isYAxisLogScale() ? logf(v) : v; };

  auto unscaleY = [&](float v) { return isYAxisLogScale() ? expf(v) : v; };

  float barWidth = rect.width() / bucketCount;
  float invMaxCount =
      1.0f / (isYAxisLogScale() ? logf(upperBound) : upperBound);

  for (int i = bucketLowerBound; i < histogramData->buckets.size(); ++i) {
    auto bucketRaw = histogramData->buckets[i];
    auto bucket =
        (isYAxisLogScale() ? logf(bucketRaw + 1) : bucketRaw) * invMaxCount;

    float xValue = (histogramData->maximumValue - histogramData->minimumValue) *
                       i / histogramData->buckets.count() +
                   histogramData->minimumValue;

    painter.setPen(colorizer_.isNull()
                       ? foregroundColor()
                       : colorizer_->getColor(xValue).asQColor());
    painter.setBrush(colorizer_.isNull()
                         ? foregroundColor()
                         : colorizer_->getColor(xValue).asQColor());
    painter.drawRect(
        QRectF(rect.left() + std::floor(barWidth * (i - bucketLowerBound)),
               rect.top() + std::floor((1.0f - bucket) * rect.height()),
               std::ceil(barWidth), std::ceil(bucket * rect.height())));
  }

  QRectF axisRect(rect.left(), rect.bottom(), rect.width(), -rect.height());

  painter.setClipping(false);

  float min = histogramData->minimumValue + bucketLowerBound * bucketStepSize;
  float unscaledMin = unscaleX(min);
  float max = histogramData->minimumValue + bucketUpperBound * bucketStepSize;
  float unscaledMax = unscaleX(max);

  LabeledAxis axisX(LabeledAxis::Horizontal);
  axisX.setColor(foregroundColor());
  axisX.setLowerValue(unscaledMin);
  axisX.setUpperValue(unscaledMax);
  axisX.setRect(axisRect);
  axisX.setLogScale(histogramData->xAxisLog);
  axisX.setLabel(xAxisLabel());
  axisX.draw(painter);

  LabeledAxis axisY(LabeledAxis::Vertical);
  axisY.setColor(foregroundColor());
  axisY.setLowerValue(isYAxisLogScale() ? 1 : 0);
  axisY.setUpperValue(upperBound);
  axisY.setRect(axisRect);
  axisY.setLogScale(isYAxisLogScale());
  axisY.setLabel(yAxisLabel());
  axisY.draw(painter);

  // drawing lines with x- and y-values on mouse hover
  QPoint cursorPos = mapFromGlobal(QCursor::pos());
  if (underMouse() && cursorPos.x() > axisMarginsBL &&
      cursorPos.x() < width() - axisMarginsTR &&
      cursorPos.y() > axisMarginsTR &&
      cursorPos.y() < height() - axisMarginsBL) {
    painter.setPen(QPen(foregroundColor()));

    // draw vertical line at cursor x
    painter.drawLine(cursorPos.x(), axisMarginsTR, cursorPos.x(),
                     height() - axisMarginsBL);

    // calculate the x-value at the cursor pos
    float xValue =
        unscaleX((cursorPos.x() - axisMarginsBL) /
                     (width() - axisMarginsBL - axisMarginsTR) * (max - min) +
                 min);

    float yValue = 0;
    if (isHoverValueSnappingEnabled()) {
      // get the index of the bucket at the cursor pos
      qint64 index = histogramProvider()->getData()->bucketIndexOfValue(xValue);
      // then use the value of the bucket at the cursor pos as y value
      yValue = histogramProvider()->getData()->buckets[index];

      // scale it if we have a log scale, then draw the horizontal line
      auto yValueScaled =
          (isYAxisLogScale() ? logf(yValue + 1) : yValue) * invMaxCount;
      painter.drawLine(
          axisMarginsBL,
          rect.top() + std::floor((1.0f - yValueScaled) * rect.height()),
          width() - axisMarginsTR,
          rect.top() + std::floor((1.0f - yValueScaled) * rect.height()));
    } else {
      // calculate the y-value at the cursor pos. The first line calculates a
      // fraction (0.0 to 1.0) depending on where the cursor is. The second line
      // uses the fraction and upper and lower bounds of the coordinate system
      // to calculate the value at the cursor pos
      yValue =
          unscaleY((1.0f - (cursorPos.y() - axisMarginsTR) /
                               (height() - axisMarginsBL - axisMarginsTR)) *
                   (scaleY(upperBound) - scaleY((isYAxisLogScale() ? 1 : 0))));

      // draw horizontal line at cursor y
      painter.drawLine(axisMarginsBL, cursorPos.y(), width() - axisMarginsTR,
                       cursorPos.y());
    }

    // print the value at the cursor position under the histogram
    QString xValueString = QString::number(xValue, 'f', 2);
    float textWidth =
        QFontMetrics(painter.font()).boundingRect(xValueString).width();
    // draw the text at the correct position
    painter.drawText(QPoint(cursorPos.x() - textWidth / 2, height() - 10),
                     xValueString);

    // display the y value. Number of decimal places gets chosen to try to keep
    // the number to a max length of 5 digits´
    int digitCount = log10f(yValue) + 1;
    QString yValueString =
        QString::number(yValue, 'f', qMax(5 - digitCount, 0));
    float textHeight = QFontMetrics(painter.font()).height();
    painter.drawText(QPoint(1, cursorPos.y() + textHeight / 2), yValueString);
  }
}

void HistogramVisualizerWidget::mouseMoveEvent(QMouseEvent* event) {
  Q_UNUSED(event);

  // force repaint if mouse is in widget bounds so the value displayed on hover
  // is updated
  if (underMouse()) {
    repaint();
  }
}

void HistogramVisualizerWidget::onHistogramProviderDataChanged(
    HistogramProvider::DataPtr data) {
  Q_UNUSED(data);
  repaint();
}

QRgb HistogramVisualizerWidget::defaultBackgroundColor() {
  return qRgb(20, 20, 20);
}

QRgb HistogramVisualizerWidget::defaultForegroundColor() {
  return qRgb(200, 200, 200);
}
