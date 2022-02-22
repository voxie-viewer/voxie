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

#include <QObject>

#include <Voxie/IO/RunFilterOperation.hpp>

#include <Voxie/Data/Colorizer.hpp>
#include <Voxie/Data/SurfaceNode.hpp>

#pragma once

/**
 * @brief This class is used for the actual calculation of the colorization done
 * by ColorizeSurfaceFromAttribute.
 */
class ColorizeSurfaceFromAttributeOperation : public QObject {
  Q_OBJECT

 public:
  ColorizeSurfaceFromAttributeOperation(
      const QSharedPointer<vx::io::RunFilterOperation>& operation);
  ~ColorizeSurfaceFromAttributeOperation();
  /**
   * @brief This method gets called by ColorizeSurfaceFromAttribute with the
   * appropriate data that should be colorized.
   * @param surfaceNode The surface node that contains the data that should
   * be colorized.
   * @param attributeName The string name of the attribute whose data should be
   * colorized.
   * @param colorizer Contains a color ramp that is used to decide what color a
   * vertex should have based on the value that the associated attribute has.
   */
  void colorizeModel(vx::SurfaceNode* surfaceNode, QString attributeName,
                     vx::Colorizer* colorizer);

 private:
  QSharedPointer<vx::io::RunFilterOperation> operation;

 Q_SIGNALS:
  /**
   * @brief colorizationDone gets raised when the colorizeModel() method is done
   * with the colorization.
   * @param outputSurface The resulting surface which contains the color data.
   * @param operation The RunFilterOperation object for our operation.
   */
  void colorizationDone(
      const QSharedPointer<vx::SurfaceDataTriangleIndexed> outputSurface,
      QSharedPointer<vx::io::RunFilterOperation> operation);
};
