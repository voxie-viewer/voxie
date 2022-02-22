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

#include <Voxie/Data/Colorizer.hpp>
#include <VoxieBackend/Data/HistogramProvider.hpp>

#include <QWidget>

namespace vx {
class VOXIECORESHARED_EXPORT HistogramVisualizerWidget : public QWidget {
  Q_OBJECT

 public:
  HistogramVisualizerWidget(QWidget* parent = nullptr);

  /**
   * @brief setYAxisLogScale Used to choose if the histogram's y-axis should be
   * scaled linearly or logarithmicly.
   * @param yAxisLogScale If set to true the histogram's y-axis will have a
   * logarithmic scale.
   */
  void setYAxisLogScale(const bool yAxisLogScale);
  /**
   * @return Returns true if y-axis scale is logarithmic, and false if y-axis
   * scale is linear.
   */
  bool isYAxisLogScale() const;

  /**
   * @return Returns a pointer to the colorizer used by the widget to colorize
   * the histogram.
   */
  QSharedPointer<Colorizer> colorizer() const;
  /**
   * @brief setColorizer Used to set the colorizer that should be used to
   * colorize the histogram's bars. If passing a nullptr (or a QSharedPointer
   * pointing to null) then the histogram will be uniformly colored using the
   * chosen foreground color.
   * @param colorizer The colorizer that should be used for colorization.
   */
  void setColorizer(const QSharedPointer<Colorizer>& colorizer);

  QSharedPointer<HistogramProvider> histogramProvider() const;
  /**
   * @brief setHistogramProvider Used to pass the histogram provider with the
   * data that should be displayed by the widget.
   * @param histogramProvider The histogram provider containing the data.
   */
  void setHistogramProvider(
      const QSharedPointer<HistogramProvider>& histogramProvider);

  /**
   * @brief setForegroundColor Used to set the foreground color. The foreground
   * color is used as the color for the axis and labels and for the color of the
   * histogram bars if no colorizer is set.
   * @param foregroundColor The color that should be used.
   */
  void setForegroundColor(const QColor foregroundColor);
  /**
   * @brief foregroundColor Used to get the foreground color. The foreground
   * color is used as the color for the axis and labels and for the color of the
   * histogram bars if no colorizer is set.
   * @return Returns the foreground color.
   */
  QColor foregroundColor() const;

  /**
   * @brief setBackgroundColor Used to set the background color.
   * @param backgroundColor The color that should be used.
   */
  void setBackgroundColor(const QColor backgroundColor);
  /**
   * @brief backgroundColor Used to get the background color.
   * @return  Returns the background color.
   */
  QColor backgroundColor() const;

  /**
   * @brief setYAxisLabel Used to set the text label that is displayed next to
   * the y-axis.
   * @param label The text that should be displayed.
   */
  void setYAxisLabel(const QString label);
  /**
   * @brief yAxisLabel Used to get the text label that is displayed next to the
   * y-axis.
   * @return Returns the text of the y-axis label.
   */
  QString yAxisLabel() const;

  /**
   * @brief setXAxisLabel Used to set the text label that is displayed under the
   * x-axis.
   * @param label The text that should be displayed.
   */
  void setXAxisLabel(const QString label);
  /**
   * @brief xAxisLabel Used to get the text label that is displayed under the
   * x-axis.
   * @return  Returns the text of the x-axis label.
   */
  QString xAxisLabel() const;

  /**
   * @brief setHoverValueSnappingEnabled Hover value snapping is q feature that
   * snaps the y-value of the crosshair that is shown when hovering over the
   * histogram to the y-value of the bar that is at the current mouse x pos.
   * @param enabled Set to true to enable the feature, set to false otherwise.
   */
  void setHoverValueSnappingEnabled(const bool enabled);
  /**
   * @brief isHoverValueSnappingEnabled Hover value snapping is q feature that
   * snaps the y-value of the crosshair that is shown when hovering over the
   * histogram to the y-value of the bar that is at the current mouse x pos.
   * @return Returns true when the feature is enabled, false otherwise.
   */
  bool isHoverValueSnappingEnabled() const;

  /**
   * @brief setAutomaticUpperBound If set to true, the upper bound of the
   * histogram y will be chosen automatically. If set to false it can be set
   * with setUpperBound()
   * @param enabled Set to true to enable manual override.
   */
  void setAutomaticUpperBound(const bool enabled) {
    automaticUpperBound = enabled;
    repaint();
  }
  /**
   * @return If true, the upper bound of the histogram y will be chosen
   * automatically. If false it can be set with setUpperBound()
   */
  bool isAutomaticUpperBound() const { return automaticUpperBound; }

  /**
   * @brief setUpperBoundOverride If automaticUpperBound is set to false this
   * value will be used as the upper bound of the histogram instead.
   * @param upperBound The value to use as the upper bound
   */
  void setUpperBoundOverride(const quint64 upperBound) {
    manualUpperBound = upperBound;
    repaint();
  }
  /**
   * @return If automaticUpperBound is set to false this value will be used as
   * the upper bound of the histogram instead.
   */
  bool upperBoundOverride() const { return manualUpperBound; }

  /**
   * @brief setAutomaticBucketLowerBound If set to true the lower bound of the
   * bucket index will be chosen automatically. This means that the lower bound
   * will be chosen so as to display the whole histogram.
   */
  void setAutomaticBucketLowerBound(const bool automaticLowerBound) {
    automaticBucketLowerBound = automaticLowerBound;
    repaint();
  }
  /**
   * @brief setAutomaticBucketLowerBound If set to true the lower bound of the
   * bucket index will be chosen automatically. This means that the lower bound
   * will be chosen so as to display the whole histogram.
   */
  bool isAutomaticBucketLowerBound() const { return automaticBucketLowerBound; }

  /**
   * @brief setAutomaticBucketLowerBound If set to true the upper bound of the
   * bucket index will be chosen automatically. This means that the upper bound
   * will be chosen so as to display the whole histogram.
   */
  void setAutomaticBucketUpperBound(const bool automaticUpperBound) {
    automaticBucketUpperBound = automaticUpperBound;
    repaint();
  }
  /**
   * @brief setAutomaticBucketLowerBound If set to true the upper bound of the
   * bucket index will be chosen automatically. This means that the upper bound
   * will be chosen so as to display the whole histogram.
   */
  bool isAutomaticBucketUpperBound() const { return automaticBucketUpperBound; }

  /**
   * @brief setBucketLowerBoundOverrideValue If isAutomaticBucketLowerBound is
   * set to false the value set here will be used by the visualizer to determine
   * the lowest (left-most) bucket that should be drawn. In simpler terms: It
   * allows the left border of the histogram to be moved one bucket at a time.
   * @param bucketIndex The bucket index to use as the lower bound (0 is the
   * default, which means all buckets will be displayed)
   */
  void setBucketLowerBoundOverrideValue(const unsigned int bucketIndex) {
    bucketLowerBoundOverride = bucketIndex;
    repaint();
  }
  /**
   * @brief bucketLowerBoundOverrideValue If isAutomaticBucketLowerBound is set
   * to false the value set here will be used by the visualizer to determine the
   * lowest (left-most) bucket that should be drawn. In simpler terms: It allows
   * the left border of the histogram to be moved one bucket at a time.
   */
  unsigned int bucketLowerBoundOverrideValue() const {
    return bucketLowerBoundOverride;
  }

  /**
   * @brief setBucketLowerBoundOverrideValue If isAutomaticBucketUpperBound is
   * set to false the value set here will be used by the visualizer to determine
   * the highest (right-most) bucket that should be drawn. In simpler terms: It
   * allows the right border of the histogram to be moved one bucket at a time.
   * @param bucketIndex The bucket index to use as the lower bound (1 is the
   * default, which means only one bucket will be drawn)
   */
  void setBucketUpperBoundOverrideValue(const unsigned int bucketIndex) {
    bucketUpperBoundOverride = bucketIndex;
    repaint();
  }

  /**
   * @brief bucketUpperBoundOverrideValue If isAutomaticBucketUpperBound is set
   * to false the value set here will be used by the visualizer to determine the
   * highest (right-most) bucket that should be drawn. In simpler terms: It
   * allows the right border of the histogram to be moved one bucket at a time.
   */
  unsigned int bucketUpperBoundOverrideValue() const {
    return bucketUpperBoundOverride;
  }

  static QRgb defaultForegroundColor();
  static QRgb defaultBackgroundColor();

 private:
  void paintEvent(QPaintEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  /**
   * @brief onHistogramProviderDataChanged Slot (event handler) for the
   * dataChanged signal of the histogramProvider this this oobject is currently
   * using. Simply forces a repaint of the widget to display the new data.
   * @param data Data of the histogramProvider.
   */
  void onHistogramProviderDataChanged(vx::HistogramProvider::DataPtr data);

  QSharedPointer<HistogramProvider> histogramProvider_;
  QSharedPointer<Colorizer> colorizer_;

  bool yAxisLogScale = false;
  bool hoverValuesSnapping = false;

  bool automaticUpperBound = true;
  quint64 manualUpperBound;

  QColor backgroundColor_ = defaultBackgroundColor();
  QColor foregroundColor_ = defaultForegroundColor();

  QString xAxisLabel_ = "";
  QString yAxisLabel_ = "";

  bool automaticBucketLowerBound = true;
  bool automaticBucketUpperBound = true;

  unsigned int bucketLowerBoundOverride = 0;
  unsigned int bucketUpperBoundOverride = 1;
};
}  // namespace vx
