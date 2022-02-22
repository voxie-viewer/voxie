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

#include <VoxieBackend/Data/FloatImage.hpp>
#include <VoxieBackend/Data/SliceImage.hpp>

#include <Voxie/Data/Slice.hpp>
#include <Voxie/OldFilter/Filter2D.hpp>

#include <QtCore/QObject>
#include <QtCore/QVector>

namespace vx {
namespace filter {
/**
 * @brief The FilterChain2D class
 * This class holds a multiple number of filters and manages them.
 * e.g. adding/removing new filter to chain, change position of a filter within
 * the chain,
 * applying all filter within a chain to a image
 */
class VOXIECORESHARED_EXPORT FilterChain2D : public QObject {
  Q_OBJECT

 private:
  QVector<vx::filter::Filter2D*> filters;
  vx::FloatImage outputFloatImage;
  vx::SliceImage outputSliceImage;
  quint64 lastID = 0;

 public:
  FilterChain2D(QObject* parent = nullptr)
      : QObject(parent), outputFloatImage(0, 0, false), outputSliceImage() {}
  ~FilterChain2D();

 public:
  /**
   * @brief applyTo
   * applies all filters saved in this chain to a FloatImage slice, stores
   * output
   * and sends a signal when its done
   * @param slice FloatImage before filterchain applied
   */
  void applyTo(vx::FloatImage slice);

  /**
   * @brief applyTo
   * applies all filters saved in this chain to a SliceImage slice, stores
   * output
   * and sends a signal when its done
   * @param slice SliceImage before filterchain applied
   */
  void applyTo(vx::SliceImage slice);

  /**
   * @brief addFilter
   * adds a new filter to the end of the filterchain
   * @param filter to be added
   */
  void addFilter(vx::filter::Filter2D* filter);

  /**
   * @brief removeFilter
   * removes this filter from the filterchain
   * @param filter to be removed
   */
  void removeFilter(vx::filter::Filter2D* filter);

  /**
   * @brief changePosition
   * puts the filter to the specific position in the chain.
   * @param filter this filter changes position
   * @param pos position in the filterchain
   */
  void changePosition(vx::filter::Filter2D* filter, int pos);

  /**
   * @brief getFilters
   * getter for filterchain
   * @return QVector the filterchain
   */
  QVector<Filter2D*> getFilters();

  /**
   * @brief getFilter
   * getter for a single filter on specified position
   * @param position of the filter to be returned
   * @return  filter
   */
  vx::filter::Filter2D* getFilter(int pos);

  vx::filter::Filter2D* getFilterByIDOrNull(quint64 id);

  /**
   * @brief getOutputImage
   * getter for output FloatImage
   * @return FloatImage after all filter of chain applied
   */
  vx::FloatImage& getOutputImage();

  SliceImage& getOutputSlice();

 private:
  void toXML(QXmlStreamWriter* writer);

  void fromXML(QXmlStreamReader* reader);

 public:
  void toXML(QString fileName);

  void fromXML(QString fileName);

  QString toXMLString();
  void fromXMLString(const QString& data);

 public Q_SLOTS:
  void onPlaneChanged(const vx::Slice* slice, const PlaneInfo& oldPlane,
                      const PlaneInfo& newPlane, bool equivalent);

 Q_SIGNALS:
  void filterChanged(Filter2D* filter);
  void filterListChanged();
  void allFiltersApplied();
};

}  // namespace filter
}  // namespace vx
