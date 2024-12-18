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

#include <Voxie/Node/FilterNode.hpp>

#include <QtWidgets/QComboBox>

#include <Voxie/IO/RunFilterOperation.hpp>

#include <PluginFitting/Prototypes.forward.hpp>

namespace vx {
namespace vis3d {

class FitPlane : public FilterNode {
  VX_NODE_IMPLEMENTATION("de.uni_stuttgart.Voxie.Filter.FitPlane")

  /**
   * @brief fitSurface does the actual fitting using the specified threshold and
   * points.
   */
  void fitSurface();

 public:
  FitPlane();
  ~FitPlane();

 private:
  QSharedPointer<vx::io::RunFilterOperation> calculate(
      bool isAutomaticFilterRun) override;
};
}  // namespace vis3d
}  // namespace vx
