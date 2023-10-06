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
#include <Voxie/Node/Node.hpp>

#include <PluginFilter/Prototypes.forward.hpp>

#include <QComboBox>

#include <VoxieBackend/Data/SurfaceData.hpp>

namespace vx {

namespace filters {

/**
 * @brief This class colorizes a surface based on the values of one of the
 * surface's attributes.
 */
class ColorizeSurfaceFromAttribute : public FilterNode {
  NODE_PROTOTYPE_DECL(ColorizeSurfaceFromAttribute)
  Q_OBJECT

 public:
  ColorizeSurfaceFromAttribute();

 private:
  /**
   * @brief Gets called when the filter should run its calculation.
   * @return A pointer to the RunFilterOperation for the calculation.
   */
  QSharedPointer<vx::io::RunFilterOperation> calculate(
      bool isAutomaticFilterRun) override;

  QSharedPointer<QObject> getPropertyUIData(QString propertyName) override;
  /**
   * @brief Updates the UI elements of the properties panel of the filter.
   * Should get called when input data of the filter gets changed or updated.
   */
  void updateUI();
  /**
   * @brief Has to be called when the user selects a different attribute that
   * should be used for colorization. Will update the histogram to reflect the
   * currently selected attribute.
   * @param attributeName The string name of the new attribute.
   */
  void onSelectedAttributeChanged(const QString& attributeName);
  /**
   * @brief Updates the output surface that the filter generates.
   * @param outputSurface
   * @param operation
   */
  void updateOutputSurface(
      const QSharedPointer<SurfaceDataTriangleIndexed>& outputSurface,
      QSharedPointer<vx::io::RunFilterOperation> operation);
  QSharedPointer<HistogramProvider> histogramProvider;
  /**
   * @brief The combo box in the properties panel of the filter which is used to
   * select the attribute of the input surface that should be used for the
   * colorization.
   */
  QComboBox* attributeSelector = nullptr;

 Q_SIGNALS:
  void inputSurfaceChanged();
};
}  // namespace filters
}  // namespace vx
