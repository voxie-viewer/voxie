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

#include <Voxie/Gui/ColorizerGradientWidget.hpp>

#include <QMouseEvent>
#include <QPainter>

ColorizerGradientWidget::ColorizerGradientWidget(QWidget* parent)
    : QWidget(parent),
      colorizer(QSharedPointer<vx::Colorizer>::create()),
      checkerboard(":/icons/transparency_.png"),
      contextMenu(new QMenu(this)) {
  setMinimumHeight(50);
  setMouseTracking(true);

  removeAction =
      contextMenu->addAction(QIcon(":/icons/minus.png"), tr("Remove"));

  connect(removeAction, &QAction::triggered, this,
          &ColorizerGradientWidget::removeSelectedEntry);

  connect(this, &ColorizerGradientWidget::selectedIndexChanged, this,
          [this]() { removeAction->setEnabled(hasSelectedEntry()); });

  toggleLogarithmicAction = contextMenu->addAction(
      QIcon(":/icons/edit-scale-vertical.png"), tr("Logarithmic scale"));
  toggleLogarithmicAction->setCheckable(true);

  connect(toggleLogarithmicAction, &QAction::toggled, this,
          [this](bool enabled) { setLogarithmicScale(enabled); });

  copyAction = contextMenu->addAction(QIcon(":/icons/document-copy.png"),
                                      tr("Copy color map"));
  connect(copyAction, &QAction::triggered, this, [this]() {
    auto entries = colorizer->getEntriesIncludeNaN();
    /*
    for (auto& entry : entries) {
      // Rescale entries from histogram interval to normalized interval
      entry = vx::ColorizerEntry(
          (entry.value() - minimumValue) / (maximumValue - minimumValue),
          entry.color(), entry.interpolator());
    }
    */
    clipboard() = entries;
  });

  pasteAction = contextMenu->addAction(QIcon(":/icons/clipboard-paste.png"),
                                       tr("Paste color map"));
  connect(pasteAction, &QAction::triggered, this, [this]() {
    auto entries = clipboard();
    /*
    for (auto& entry : entries) {
      // Rescale entries from normalized interval to histogram interval
      entry = vx::ColorizerEntry(
          entry.value() * (maximumValue - minimumValue) + minimumValue,
          entry.color(), entry.interpolator());
    }
    */
    colorizer->setEntries(entries);
    setSelectedIndex(-1);
  });
}

void ColorizerGradientWidget::setMinimumValue(double value) {
  minimumValue = value;
  Q_EMIT boundsChanged();
}

double ColorizerGradientWidget::getMinimumValue() const { return minimumValue; }

void ColorizerGradientWidget::setMaximumValue(double value) {
  maximumValue = value;
  Q_EMIT boundsChanged();
}

double ColorizerGradientWidget::getMaximumValue() const { return maximumValue; }

void ColorizerGradientWidget::setBounds(double minValue, double maxValue) {
  minimumValue = minValue;
  maximumValue = maxValue;
  Q_EMIT boundsChanged();
}

void ColorizerGradientWidget::setEntryCountBounds(int minEntries,
                                                  int maxEntries) {
  minimumEntries = minEntries;
  maximumEntries = maxEntries;
}

void ColorizerGradientWidget::paintEvent(QPaintEvent* event) {
  Q_UNUSED(event);
  static const int handleSizeSelected = 8;
  static const int handleSizeUnselected = 7;
  static const int lineThicknessUnselected = 1;
  static const int lineThicknessSelected = 2;
  // we need the padding at the bottom to display values under the gradient
  static const int paddingBottom = 15;

  QPainter painter(this);

  // Disable antialiasing
  painter.setRenderHint(QPainter::RenderHint(0));

  // Draw checkerboard background
  painter.setBrush(checkerboard);
  painter.drawRect(0, 0, width(), (height() - paddingBottom));
  painter.setBrush(Qt::NoBrush);

  // Skip drawing color gradient on empty colorizer
  if (colorizer->getEntryCount() > 0) {
    float factor = (maximumValue - minimumValue) / width();

    for (int i = 0; i < width(); ++i) {
      float value = minimumValue + i * factor;
      QColor color = colorizer->getColor(value).asQColor();

      painter.setPen(color);
      painter.drawLine(i, 0, i, height() - paddingBottom);

      float y = (1.f - getBucketFractionForValue(value) * 0.8f) *
                (height() - paddingBottom);
      painter.setPen(QColor::fromHslF(0, 0, 1, 0.5));
      painter.drawLine(i, y + 1, i, height() - paddingBottom);

      painter.setPen(QColor::fromHslF(0, 0, 0, 0.5));
      painter.drawPoint(i, y);
    }

    for (int i = 0; i < colorizer->getEntryCount(); ++i) {
      const auto& entry = colorizer->getEntries()[i];

      float x = (entry.value() - minimumValue) / factor;
      QColor color = entry.color().asQColor();
      bool light = color.lightnessF() > 0.5;

      painter.setPen(Qt::NoPen);
      painter.setBrush(
          QColor::fromHslF(color.hueF(), color.saturationF(),
                           color.lightnessF() + (light ? -.3 : .3)));

      bool selected = getSelectedIndex() == i;
      bool hovered = getHoveredIndex() == i;

      int thickness =
          hovered || selected ? lineThicknessSelected : lineThicknessUnselected;
      painter.drawRect(x - thickness + 1, 0, thickness * 2 - 1,
                       height() - paddingBottom);

      int handleSize = selected ? handleSizeSelected : handleSizeUnselected;

      QPolygon triangle;
      triangle.append(QPoint(-handleSize, 0));
      triangle.append(QPoint(0, handleSize));
      triangle.append(QPoint(handleSize, 0));
      triangle.translate(x, 0);

      QColor contrastColor = light ? Qt::black : Qt::white;
      painter.setPen(contrastColor);
      painter.setBrush(selected ? contrastColor : color);
      painter.drawPolygon(triangle);
    }
  }

  // draw value at cursor position when hovering over gradient and min and max
  // values at the left and right edges
  bool hideMin = false;  // these variables are used later, they are set to true
                         // when the value that is
  bool hideMax = false;  // displayed under the cursor would overlap the min or
                         // max values so we can hide them

  QString minText = QString::number(minimumValue, 'f', 2);
  float minTextWidth = fontMetrics().boundingRect(minText).width();

  QString maxText = QString::number(maximumValue, 'f', 2);
  float maxTextWidth = fontMetrics().boundingRect(maxText).width();

  painter.setPen(QPen(Qt::black));
  if (underMouse()) {
    // draw vertical line at cursor x
    QPoint cursorPos = mapFromGlobal(QCursor::pos());
    painter.drawLine(cursorPos.x(), 0, cursorPos.x(), height() - paddingBottom);

    // print the value at the cursor position under the gradient
    float valueAtCursor = convertMousePosition(cursorPos.x());
    QString valueString = QString::number(valueAtCursor, 'f', 2);
    float textWidth = fontMetrics().boundingRect(valueString).width();
    // draw the text at the correct position. We need the qMin() and qMax()
    // functions so the text is never (partially) drawn outside of the widget
    // (it would then be invisible). Instead we clip it so that it never goes
    // over the border
    painter.drawText(QPoint(qMin(qMax(cursorPos.x() - textWidth / 2, 0.0f),
                                 width() - textWidth),
                            height()),
                     valueString);

    // hide min text if left edge of cursor value text is more to the left than
    // the right edge of the min text do the same the other way around for the
    // max text which is at the right side of the screen
    hideMin = cursorPos.x() - textWidth / 2 < minTextWidth;
    hideMax = cursorPos.x() + textWidth / 2 > width() - maxTextWidth;
  }

  // draw min and max values of the gradient
  if (!hideMin) {
    painter.drawText(QPoint(0, height()), minText);
  }
  if (!hideMax) {
    painter.drawText(QPoint(width() - maxTextWidth, height()), maxText);
  }

  painter.setPen(Qt::NoPen);
}

void ColorizerGradientWidget::updateHistogram() {
  if (histogramProvider) {
    histogram = histogramProvider->getData();
    double newMinimum = histogram->minimumValue;
    double newMaximum = histogram->maximumValue;

    if (qFuzzyCompare(newMinimum, newMaximum)) {
      // Maximum too close to minimum? Push it away
      newMaximum = newMaximum + 1.0;
    }

    if (autoRescaling) {
      colorizer->rescale(minimumValue, maximumValue, newMinimum, newMaximum);
    }

    setBounds(newMinimum, newMaximum);
  }

  updateMaximumBucket();
  update();
}

void ColorizerGradientWidget::updateMaximumBucket() {
  maximumBucket = 0;
  if (histogram) {
    for (auto bucket : histogram->buckets) {
      maximumBucket = std::max<quint64>(maximumBucket, bucket);
    }
  }
}

double ColorizerGradientWidget::getBucketFractionForValue(double value) const {
  if (histogram && maximumBucket > 0) {
    double histMin = histogram->minimumValue;
    double histMax = histogram->maximumValue;
    qint64 histBuckets = histogram->buckets.size();
    qint64 index = (value - histMin) / (histMax - histMin) * (histBuckets - 1);

    if (index >= 0 && index < histBuckets) {
      if (logarithmic) {
        return logf(histogram->buckets[index] + 1) / logf(maximumBucket + 1);
      } else {
        return histogram->buckets[index] / double(maximumBucket);
      }
    }
  }

  return 0.f;
}

double ColorizerGradientWidget::getHoverRange() const {
  // Convert 8 pixels to the gradient's value domain
  return 8 * (maximumValue - minimumValue) / width();
}

int ColorizerGradientWidget::updateHoveredIndex(double value) {
  double hoverRange = getHoverRange();

  // Track the closest handle and its distance to the mouse pointer
  int closestIndex = -1;
  double closestIndexDistance = 0;

  // Find the handle closest to the mouse point (that is within hover range)
  for (int i = 0; i < colorizer->getEntryCount(); ++i) {
    double distance = std::abs(value - colorizer->getEntries()[i].value());
    if (distance < hoverRange &&
        (closestIndex < 0 || distance < closestIndexDistance)) {
      closestIndex = i;
      closestIndexDistance = distance;
    }
  }

  if (hoveredIndex != closestIndex) {
    hoveredIndex = closestIndex;
    update();
  }

  return closestIndex;
}

double ColorizerGradientWidget::convertMousePosition(int x) const {
  return minimumValue + x * (maximumValue - minimumValue) / width();
}

std::vector<vx::ColorizerEntry>& ColorizerGradientWidget::clipboard() {
  static std::vector<vx::ColorizerEntry> clipboardInstance;
  return clipboardInstance;
}

void ColorizerGradientWidget::setLogarithmicScale(bool logarithmic) {
  if (this->logarithmic != logarithmic) {
    this->logarithmic = logarithmic;
    update();
  }
}

bool ColorizerGradientWidget::isLogarithmicScale() const { return logarithmic; }

void ColorizerGradientWidget::setHistogramProvider(
    QSharedPointer<vx::HistogramProvider> histogramProvider) {
  if (this->histogramProvider) {
    disconnect(this->histogramProvider.data(), nullptr, this, nullptr);
  }

  this->histogramProvider = histogramProvider;

  if (histogramProvider) {
    connect(histogramProvider.data(), &vx::HistogramProvider::dataChanged, this,
            &ColorizerGradientWidget::updateHistogram);
  }

  updateHistogram();
}

QSharedPointer<vx::HistogramProvider>
ColorizerGradientWidget::getHistogramProvider() const {
  return histogramProvider;
}

void ColorizerGradientWidget::setAutoRescaling(bool autoRescaling) {
  this->autoRescaling = autoRescaling;
}

bool ColorizerGradientWidget::isAutoRescaling() const { return autoRescaling; }

void ColorizerGradientWidget::addEntry() {
  if (getEntryCount() == 0) {
    // No entries: create default black stop at 0
    addEntry(getMinimumValue(), QColor(Qt::black));
  } else if (getEntryCount() == 1) {
    // 1 entry: create max/min entry furthest from existing entry
    double value = colorizer->getEntries()[0].value();
    if (std::abs(getMinimumValue() - value) >
        std::abs(getMaximumValue() - value)) {
      addEntry(getMinimumValue(), QColor(Qt::black));
    } else {
      addEntry(getMaximumValue(), QColor(Qt::white));
    }
  } else {
    // 2+ entries: create average mapping between selected and next entry
    auto entries = colorizer->getEntries();
    int index = hasSelectedEntry()
                    ? std::min(getSelectedIndex(), getEntryCount() - 2)
                    : getEntryCount() - 2;
    auto entry1 = entries[index];
    auto entry2 = entries[index + 1];

    double value = (entry1.value() + entry2.value()) * 0.5;

    addEntry(value, colorizer->getColor(value), entry1.interpolator());
  }
}

int ColorizerGradientWidget::addEntry(vx::ColorizerEntry entry) {
  return addEntry(entry.value(), entry.color(), entry.interpolator());
}

int ColorizerGradientWidget::addEntry(double value, vx::Color color,
                                      vx::ColorInterpolator interpolator) {
  // block entry add if entry limits are reached and return -1
  if ((maximumEntries > 0 && getEntryCount() < maximumEntries) ||
      maximumEntries < 0) {
    int index = colorizer->putMapping(value, color, interpolator);
    setSelectedIndex(index);
    Q_EMIT selectedEntryValueChanged();
    Q_EMIT entryAdded();
    return index;
  }
  return -1;
}

void ColorizerGradientWidget::removeSelectedEntry() {
  // block removal if entry limits are reached
  int index = getSelectedIndex();
  if ((minimumEntries > 0 && getEntryCount() > minimumEntries) ||
      minimumEntries < 0) {
    colorizer->removeMapping(index);
    setSelectedIndex(std::min(getEntryCount() - 1, getSelectedIndex()));
    Q_EMIT selectedEntryValueChanged();
    Q_EMIT entryRemoved(index);
  }
}

void ColorizerGradientWidget::clearAllEntries() {
  for (int i = getEntryCount() - 1; i >= 0; i--) {
    colorizer->removeMapping(i);
    Q_EMIT entryRemoved(i);
  }
}

void ColorizerGradientWidget::setSelectedIndex(int index) {
  if (!colorizer->isValidIndex(index)) {
    index = -1;
  }
  if (this->selectedIndex != index) {
    this->selectedIndex = index;
    Q_EMIT selectedIndexChanged();
    update();
  }
}

int ColorizerGradientWidget::getSelectedIndex() const { return selectedIndex; }

int ColorizerGradientWidget::getHoveredIndex() const { return hoveredIndex; }

int ColorizerGradientWidget::getEntryCount() const {
  return colorizer->getEntryCount();
}

void ColorizerGradientWidget::nextEntry() {
  if (!hasSelectedEntry()) {
    setSelectedIndex(0);
  } else {
    setSelectedIndex(getSelectedIndex() + 1);
  }
}

void ColorizerGradientWidget::previousEntry() {
  if (!hasSelectedEntry()) {
    setSelectedIndex(getEntryCount() - 1);
  } else {
    setSelectedIndex(getSelectedIndex() - 1);
  }
}

int ColorizerGradientWidget::findIndexByValue(double value) {
  auto iterator = std::lower_bound(
      colorizer->getEntries().begin(), colorizer->getEntries().end(),
      vx::ColorizerEntry(value, vx::Color(), vx::ColorInterpolator::RGB));
  return std::distance(colorizer->getEntries().begin(), iterator) - 1;
}

vx::ColorizerEntry ColorizerGradientWidget::getSelectedEntry() const {
  return hasSelectedEntry()
             ? colorizer->getEntries()[getSelectedIndex()]
             : vx::ColorizerEntry(0, vx::Color(), vx::ColorInterpolator::RGB);
}

vx::ColorizerEntry ColorizerGradientWidget::getEntry(int idx) const {
  if (hasSelectedEntry()) {
    if (colorizer->isValidIndex(idx)) {
      return colorizer->getEntries()[idx];
    }
  }
  return vx::ColorizerEntry(0, vx::Color(), vx::ColorInterpolator::RGB);
}

void ColorizerGradientWidget::setEntryValue(int idx, double value) {
  if (colorizer->isValidIndex(idx)) {
    auto entry = getEntry(idx);

    // check if order changes and remove/add mapping needed or not
    double lowerValue = -std::numeric_limits<double>::infinity();
    double upperValue = std::numeric_limits<double>::infinity();
    if (idx > 0) lowerValue = getEntry(idx - 1).value();
    if (idx < getEntryCount() - 1) upperValue = getEntry(idx + 1).value();

    if (lowerValue <= value && value <= upperValue) {
      colorizer->changeValueMapping(idx, value);
    } else {
      colorizer->removeMapping(idx);
      colorizer->putMapping(value, entry.color(), entry.interpolator());
    }
    update();
    Q_EMIT entryValueChanged(idx, entry.value(), value);
  }
}

void ColorizerGradientWidget::setEntryColor(int idx, vx::Color color) {
  if (colorizer->isValidIndex(idx)) {
    colorizer->changeMapping(idx, color);
    update();
  }
}

const std::vector<vx::ColorizerEntry>& ColorizerGradientWidget::getAllEntries()
    const {
  return colorizer->getEntries();
}

bool ColorizerGradientWidget::hasSelectedEntry() const {
  return colorizer->isValidIndex(selectedIndex);
}

void ColorizerGradientWidget::setSelectedEntryValue(double value) {
  if (hasSelectedEntry()) {
    auto entry = getSelectedEntry();
    auto idx = getSelectedIndex();
    colorizer->removeMapping(idx);
    setSelectedIndex(
        colorizer->putMapping(value, entry.color(), entry.interpolator()));
    Q_EMIT entryValueChanged(idx, entry.value(), value);
  }
}

void ColorizerGradientWidget::setSelectedEntryColor(vx::Color color) {
  if (hasSelectedEntry()) {
    colorizer->changeMapping(getSelectedIndex(), color);
  }
}

void ColorizerGradientWidget::setSelectedEntryInterpolator(
    vx::ColorInterpolator interpolator) {
  if (hasSelectedEntry()) {
    colorizer->changeInterpolator(getSelectedIndex(), interpolator);
  }
}

void ColorizerGradientWidget::setBlockOverlaps(bool status) {
  blockEntryOverlaps = status;
}

void ColorizerGradientWidget::mouseMoveEvent(QMouseEvent* event) {
  // Convert the mouse pointer's position to the gradient's value domain
  double value = convertMousePosition(event->x());
  if (blockEntryOverlaps && dragging) {
    // calculate left and right entry values for overlap check
    auto leftIndex = getSelectedIndex() - 1;
    auto rightIndex = getSelectedIndex() + 1;

    if (leftIndex > 0) {
      if (value <= getEntry(leftIndex).value()) {
        // deactivate dragging if left entry undershoot
        dragging = false;
      }
    }
    if (rightIndex < getEntryCount()) {
      if (value >= getEntry(rightIndex).value()) {
        // deactivate dragging if right entry overshoot
        dragging = false;
      }
    }
  }

  if (dragging) {
    setSelectedEntryValue(value);
    Q_EMIT selectedEntryValueChanged();
  } else {
    updateHoveredIndex(value);
  }

  // force repaint if mouse is in widget bounds so the value displayed on hover
  // is updated
  if (underMouse()) {
    repaint();
  }
}

void ColorizerGradientWidget::mousePressEvent(QMouseEvent* event) {
  // Convert the mouse pointer's position to the gradient's value domain
  double value = convertMousePosition(event->x());
  updateHoveredIndex(value);

  switch (event->button()) {
    case Qt::MouseButton::LeftButton:
      setSelectedIndex(getHoveredIndex());
      dragging = true;
      break;

    case Qt::MouseButton::RightButton:
      setSelectedIndex(getHoveredIndex());
      pasteAction->setEnabled(!clipboard().empty());
      contextMenu->popup(event->globalPos());
      break;

    default:
      break;
  }
}

void ColorizerGradientWidget::mouseReleaseEvent(QMouseEvent* event) {
  switch (event->button()) {
    case Qt::MouseButton::LeftButton:
      dragging = false;
      break;

    default:
      break;
  }
}

void ColorizerGradientWidget::mouseDoubleClickEvent(QMouseEvent* event) {
  if (hasSelectedEntry()) {
    Q_EMIT entryDoubleClicked(getSelectedIndex());
  } else {
    double value = convertMousePosition(event->x());
    vx::Color color = colorizer->getColor(value);

    // Use the interpolator on the left side to determine new color
    int index = findIndexByValue(value);
    vx::ColorInterpolator interpolator =
        colorizer->isValidIndex(index)
            ? colorizer->getEntries()[index].interpolator()
            : vx::ColorInterpolator::RGB;

    hoveredIndex = addEntry(value, color, interpolator);
    update();
  }
}

void ColorizerGradientWidget::setColorizer(
    QSharedPointer<vx::Colorizer> colorizer) {
  if (this->colorizer) {
    disconnect(this->colorizer.data(), nullptr, this, nullptr);
  }

  this->colorizer = colorizer;

  if (colorizer) {
    connect(colorizer.data(), &vx::Colorizer::mappingChanged,
            [this] { update(); });
  }
}

QSharedPointer<vx::Colorizer> ColorizerGradientWidget::getColorizer() const {
  return colorizer;
}

void ColorizerGradientWidget::leaveEvent(QEvent* event) {
  Q_UNUSED(event);
  // we need to repaint when the cursor leaves the widget so that the line and
  // value we draw at the cursor position when hovering disappear again
  repaint();
}
