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

#include <Voxie/Voxie.hpp>

#include <QtCore/QIODevice>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QPair>
#include <QtCore/QVector>
#include <QtGui/QRgb>

#include <Voxie/Data/Color.hpp>
#include <Voxie/Data/ColorInterpolator.hpp>
#include <Voxie/Data/ColorizerEntry.hpp>

#include <VoxieClient/DBusTypeList.hpp>

namespace vx {
class VolumeNode;
class ImageDataPixel;
class FloatImage;

/**
 * The Colorizer defines a mapping of float values to integer RGBA values
 * for converting FloatImage to QImage. It defines what colors correspond to
 * which float value. Color is interpolated for values that dont have a distinct
 * mapping.
 * @brief The Colorizer class
 */
class VOXIECORESHARED_EXPORT Colorizer : public QObject {
  Q_OBJECT
 public:
  explicit Colorizer(QObject* parent = nullptr);

  /**
   * Removes all mappings from the colorizer.
   */
  void clear();

  /**
   * Sets the list of colorizer entries.
   */
  void setEntries(std::vector<ColorizerEntry> entries);

  /**
   * Sets the list of colorizer entries (QList overload).
   */
  void setEntries(const QList<ColorizerEntry>& entries);

  /**
   * Returns a list of all color mapping entries, including the NaN mapping.
   */
  std::vector<ColorizerEntry> getEntriesIncludeNaN() const;

  /**
   * Returns a list of all color mapping entries, including the NaN mapping, as
   * a QList.
   */
  QList<ColorizerEntry> getEntriesAsQList() const;

  /**
   * Returns a list of all numeric (non-NaN) color mapping entries.
   */
  const std::vector<ColorizerEntry>& getEntries() const;

  /**
   * Adds a new value-color mapping and returns its index.
   */
  int putMapping(ColorizerEntry entry);

  /**
   * Adds a new value-color mapping and returns its index.
   */
  int putMapping(double value, Color color,
                 ColorInterpolator interpolator = ColorInterpolator::RGB);

  /**
   * Changes an existing color mapping to a different color, with the same
   * value.
   */
  void changeMapping(int index, Color color);

  /**
   * Changes an existing mapping to a different value, with the same
   * color.
   */
  void changeValueMapping(int index, double value);

  /**
   * Changes the interpolator used at the specified index.
   */
  void changeInterpolator(int index, ColorInterpolator interpolator);

  /**
   * Removes the mapping at the specified index.
   */
  void removeMapping(int index);

  /**
   * Returns true if the specified index is valid, or false if it is out of
   * range.
   */
  bool isValidIndex(int index) const;

  /**
   * Returns the mapping at the specified index. If no mappings are contained
   * within this colorizer, returns the NaN color mapping. If the index is less
   * than 0, returns the 0th mapping. If the index is greater than or equal to
   * the number of entries, the last entry is returned.
   */
  ColorizerEntry getMapping(int index) const;

  /**
   * Returns the number of mappings within this Colorizer.
   */
  int getEntryCount() const;

  /**
   * Assigns a fallback color to be used for NaN inputs.
   */
  void setNanColor(QRgb color);

  /**
   * Returns the fallback color to be used for NaN inputs.
   */
  QRgb getNanColor() const;

  /**
   * Rescales the colorizer's entire mapping list from a source interval to a
   * target interval.
   */
  void rescale(double sourceMin, double sourceMax, double targetMin,
               double targetMax);

  /**
   * @param in input value
   * @return interpolated color for in value
   */
  Color getColor(double in) const;

  /**
   * @param ioDevice exports colorizers mappings to ioDevice in XML format.
   * if the ioDevice is already opened in readOnly mode the method cannot
   * export to it. Either pass an unopened device or one that can be written to.
   */
  void toXML(QIODevice* ioDevice) const;

  /**
   * @param filename exports colorizers mappings to given file in XML format.
   */
  void toXML(QString filename) const;

  /**
   * inittializes the color mapping with the default values of 0 and 1
   */
  void initDefaultMapping();

  /**
   * loads colorizer mappings from xml from given ioDevice. Will emit
   * mappingChanged().
   * @param ioDevice
   */
  bool loadFromXML(QIODevice* ioDevice);

  /**
   * loads colorizer mappings from xml from given file. Will emit
   * mappingChanged().
   * @param filename
   */
  bool loadFromXML(QString filename);

 public:
  /**
   * @param data for which the colorizer will be automaticly adjusted.
   */
  static QPair<QPair<float, QRgb>, QPair<float, QRgb>> initByAlgorithm(
      VolumeNode* data);
  /**
   * @param data for which the colorizer will be automaticly adjusted.
   */
  static QPair<QPair<float, QRgb>, QPair<float, QRgb>> initByAlgorithm(
      const QSharedPointer<ImageDataPixel>& data);
  static Colorizer* fromXML(QIODevice* ioDevice);

  /**
   * creates a grayscaled QImage from this floatimage.
   * The Values will be mapped to greyscale interpolating
   * between lowest and highest value, where highest value is mapped
   * to white and lowest to black. NaN Values will be transparent.
   * @param lowestValue will be mapped to black
   * @param highestValue will be mapped to white
   * @return qimage
   */
  static QImage toQImageGray(const vx::FloatImage& image, float lowestValue = 0,
                             float highestValue = 1);

  /**
   * creates a Qimage from this floatimage. The values will be mapped to color
   * according to the given colorizer.
   * @param colorizer for value->color mapping
   * @return qimage
   */
  QImage toQImage(const FloatImage& image);

 private:
  std::vector<ColorizerEntry> entries;
  QRgb nanColor;

 Q_SIGNALS:
  void mappingChanged();
};

}  // namespace vx
