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

#include <Voxie/OldFilterMask/Selection2DMask.hpp>

#include <QtCore/QObject>
#include <QtCore/QXmlStreamAttributes>

#include <QtWidgets/QDialog>

namespace vx {
namespace plugin {
class MetaFilter2D;
}
namespace filter {

/**
 * @brief The Filter2D class
 * This class implements already most of the methods.
 * Concrete filter inheriting from this class, need to implement
 * methods getTargetVolume and applyTo().
 *
 */
class VOXIECORESHARED_EXPORT Filter2D : public QObject {
  // QTAnotataions
  Q_OBJECT

  // private attributes
 private:
  vx::plugin::MetaFilter2D* metaFilter_;
  bool enabled = true;
  // TODO: Is this still used?
  vx::filter::Selection2DMask* mask;

  // public constructor
 public:
  Filter2D(vx::plugin::MetaFilter2D* metaFilter);

  quint64 filterID;

  // public methods
 public:
  vx::plugin::MetaFilter2D* metaFilter() { return metaFilter_; }

  /**
   * @brief applyToCopy
   * Already implemented apply method.
   * This method applies a concrete filter implementation to a FloatImage
   * @param input FloatImage where filter will be applied.
   * @return output FloatImage after filter applied
   */
  vx::FloatImage applyToCopy(vx::FloatImage input);

  /**
   * @brief applyToCopy
   * Already implemented apply method.
   * This method applies a concrete filter implementation to a SliceImage
   * @param input SliceImage where filter will be applied.
   * @return output SliceImage after filter applied
   */
  vx::SliceImage applyToCopy(vx::SliceImage input);

  /**
   * @brief isEnabled
   * Method to check wether the filter is enabled or disabled
   * @return true if filter is enabled, flase if disabled
   */
  virtual bool isEnabled();

  /**
   * @brief setEnabled
   * Setter for the enabled attribute
   * @param enable true if filter shall enabled else false
   */
  virtual void setEnabled(bool enable);

  /**
   * @brief exportFilterSettingsXML
   * Exports all settings of a concrete filter.
   * @return
   */
  virtual QXmlStreamAttributes exportFilterSettingsXML() = 0;

  /**
   * @brief importFilterSettingsXML
   * Imports all settings of a filter from xml
   * @param attributes
   */
  virtual void importFilterSettingsXML(QXmlStreamAttributes attributes) = 0;

  /**
   * @brief getSettingsDialog
   * A pointer to a customaziable QDialog that allows to modify filter settings.
   */
  virtual QDialog* getSettingsDialog() { return nullptr; }

  /**
   * @brief hasSettingsDialog
   * returns whether the filter has a settings dialog or not (default true).
   */
  virtual bool hasSettingsDialog() { return true; }

  /**
   * @return this filter's mask
   */
  Selection2DMask* getMask() { return this->mask; }

  /**
   * @return this filter's mask
   */
  const Selection2DMask* getMask() const { return this->mask; }

 protected:
  /**
   * @brief applyTo
   * Applies this filter to input FloatImage and stores result in output.
   * This method must be implemented by a concrete filter
   * @param input FloatImage before filter applied
   * @param output FloatImage after filter applied
   */
  virtual void applyTo(vx::FloatImage input, vx::FloatImage output) = 0;

  /**
   * @brief applyTo
   * Applies this filter to input SliceImage and stores result in output.
   * This method must be implemented by a concrete filter
   * @param input SliceImage before filter applied
   * @param output SliceImage after filter applied
   */
  virtual void applyTo(vx::SliceImage input, vx::SliceImage output) = 0;

  /**
   * convenience method for 'emit this->filterchanged(this)'
   */
  void triggerFilterChanged() { Q_EMIT this->filterChanged(this); }

 Q_SIGNALS:
  /**
   * will be emitted whenever the filter is enabled or disabled,
   * when the filters filter mask changes or when the filters internal
   * settings are changed
   * @param filter that emmited the signal
   */
  void filterChanged(Filter2D* filter);
};

}  // namespace filter
}  // namespace vx
