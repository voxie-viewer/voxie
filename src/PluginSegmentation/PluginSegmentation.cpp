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

#include <PluginSegmentation/PluginSegmentation.hpp>

#include <PluginSegmentation/Segmentation.hpp>
#include <PluginSegmentation/Steps/AssignmentStep.hpp>
#include <PluginSegmentation/Steps/BrushSelectionStep.hpp>
#include <PluginSegmentation/Steps/LassoSelectionStep.hpp>
#include <PluginSegmentation/Steps/ManualSelectionStep.hpp>
#include <PluginSegmentation/Steps/MetaStep.hpp>
#include <PluginSegmentation/Steps/MultiThresholdStep.hpp>
#include <PluginSegmentation/Steps/RemoveLabelStep.hpp>
#include <PluginSegmentation/Steps/SubtractStep.hpp>
#include <PluginSegmentation/Steps/ThresholdSelectionStep.hpp>

using namespace vx::filters;

PluginSegmentation::PluginSegmentation() : QGenericPlugin() {}

QObject* PluginSegmentation::create(const QString& key,
                                    const QString& specification) {
  (void)key;
  (void)specification;
  return nullptr;
}

QList<QSharedPointer<vx::NodePrototype>>
PluginSegmentation::objectPrototypes() {
  QList<QSharedPointer<vx::NodePrototype>> list;
  list.append(Segmentation::getPrototypeSingleton());
  list.append(AssignmentStep::getPrototypeSingleton());
  list.append(ThresholdSelectionStep::getPrototypeSingleton());
  list.append(BrushSelectionStep::getPrototypeSingleton());
  list.append(LassoSelectionStep::getPrototypeSingleton());
  list.append(MetaStep::getPrototypeSingleton());
  list.append(MultiThresholdStep::getPrototypeSingleton());
  list.append(ManualSelectionStep::getPrototypeSingleton());
  list.append(RemoveLabelStep::getPrototypeSingleton());
  list.append(SubtractStep::getPrototypeSingleton());
  return list;
}
