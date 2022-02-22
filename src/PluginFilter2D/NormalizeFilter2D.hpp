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

#include <Voxie/OldFilter/Filter2D.hpp>

#include <Voxie/Component/MetaFilter2D.hpp>

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDoubleSpinBox>

class NormalizeFilter2D : public vx::filter::Filter2D {
  Q_OBJECT
  Q_DISABLE_COPY(NormalizeFilter2D)

 public:
  NormalizeFilter2D(QObject* parent = 0);
  ~NormalizeFilter2D();

  virtual void applyTo(vx::FloatImage input, vx::FloatImage output) override;

  virtual void applyTo(vx::SliceImage input, vx::SliceImage output) override;

  virtual QDialog* getSettingsDialog() override;

  bool hasSettingsDialog() override { return true; }

  virtual QXmlStreamAttributes exportFilterSettingsXML() override;

  virtual void importFilterSettingsXML(
      QXmlStreamAttributes attributes) override;

 private:
  virtual void updateSettings();
  virtual void updateDialog();
  float lowerLimit = 0.0;
  float upperLimit = 1.0;
  bool autoLimits = true;
  QDoubleSpinBox* minBox = nullptr;
  QDoubleSpinBox* maxBox = nullptr;
  QCheckBox* autoBox = nullptr;
  QDialog* dialog = nullptr;
};

class NormalizeMetaFilter2D : public vx::plugin::MetaFilter2D {
 public:
  NormalizeMetaFilter2D()
      : vx::plugin::MetaFilter2D("de.uni_stuttgart.Voxie.NormalizeFilter2D") {}

  vx::filter::Filter2D* createFilter() const override;
};
