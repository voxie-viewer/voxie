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

#include <Voxie/Data/Color.hpp>
#include <Voxie/Data/Colorizer.hpp>
#include <Voxie/Data/ColorizerEntry.hpp>
#include <VoxieBackend/Data/HistogramProvider.hpp>

#include <QMenu>
#include <QSharedPointer>
#include <QWidget>

class VOXIECORESHARED_EXPORT ColorizerGradientWidget : public QWidget {
  Q_OBJECT
 public:
  explicit ColorizerGradientWidget(QWidget* parent = nullptr);

  void setMaximumValue(double value);
  double getMaximumValue() const;

  void setMinimumValue(double value);
  double getMinimumValue() const;

  void setBounds(double minValue, double maxValue);
  void setEntryCountBounds(int minEntries, int maxEntries);

  void setColorizer(QSharedPointer<vx::Colorizer> colorizer);
  QSharedPointer<vx::Colorizer> getColorizer() const;

  void setHistogramProvider(QSharedPointer<vx::HistogramProvider> value);
  QSharedPointer<vx::HistogramProvider> getHistogramProvider() const;

  void setAutoRescaling(bool autoRescaling);
  bool isAutoRescaling() const;

  void addEntry();
  int addEntry(vx::ColorizerEntry entry);
  int addEntry(double value, vx::Color color,
               vx::ColorInterpolator interpolator = vx::ColorInterpolator::RGB);
  void removeSelectedEntry();

  void setSelectedIndex(int index);
  int getSelectedIndex() const;

  int getHoveredIndex() const;

  int getEntryCount() const;

  const std::vector<vx::ColorizerEntry>& getAllEntries() const;
  void clearAllEntries();

  void nextEntry();
  void previousEntry();

  int findIndexByValue(double value);

  vx::ColorizerEntry getSelectedEntry() const;
  vx::ColorizerEntry getEntry(int idx) const;
  void setEntryValue(int index, double value);
  void setEntryColor(int index, vx::Color color);
  bool hasSelectedEntry() const;

  void setSelectedEntryValue(double value);
  void setSelectedEntryColor(vx::Color color);
  void setSelectedEntryInterpolator(vx::ColorInterpolator interpolator);

  void setBlockOverlaps(bool status);

  void setLogarithmicScale(bool logarithmic);
  bool isLogarithmicScale() const;

 Q_SIGNALS:
  void selectedIndexChanged();
  void selectedEntryValueChanged();
  void boundsChanged();
  void entryDoubleClicked(int index);
  void entryAdded();
  void entryRemoved(int idx);
  void entryValueChanged(int idx, double oldVal, double newVal);

 protected:
  void leaveEvent(QEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void mouseDoubleClickEvent(QMouseEvent* event) override;
  void paintEvent(QPaintEvent* event) override;

 private:
  void updateHistogram();
  void updateMaximumBucket();
  double getBucketFractionForValue(double value) const;
  double getHoverRange() const;
  int updateHoveredIndex(double value);
  double convertMousePosition(int x) const;

  static std::vector<vx::ColorizerEntry>& clipboard();

  QSharedPointer<vx::Colorizer> colorizer;
  int selectedIndex = -1;

  int hoveredIndex = -1;
  bool dragging = false;

  QSharedPointer<vx::HistogramProvider> histogramProvider;
  vx::HistogramProvider::DataPtr histogram;
  bool autoRescaling = false;
  bool logarithmic = false;

  double minimumValue = 0.0;
  double maximumValue = 1.0;

  bool blockEntryOverlaps = false;

  // limit entry counts, if exceeded add/remove actions are blocked
  // -1 (default) and other negative values means limits are ignored
  int minimumEntries = -1;
  int maximumEntries = -1;

  quint64 maximumBucket = 0;

  QPixmap checkerboard;

  QMenu* contextMenu;
  QAction* removeAction;
  QAction* toggleLogarithmicAction;
  QAction* copyAction;
  QAction* pasteAction;
};
