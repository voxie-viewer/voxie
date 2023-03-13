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

#include "PluginFilter.hpp"

#include <PluginFilter/CcaResultSelection.hpp>
#include <PluginFilter/ColorizeLabeledSurface.hpp>
#include <PluginFilter/ColorizeSurfaceFromAttribute.hpp>
#include <PluginFilter/CreateSurface/CreateSurface.hpp>

using namespace vx::filters;

PluginFilter::PluginFilter() : QGenericPlugin() {}

QObject* PluginFilter::create(const QString& key,
                              const QString& specification) {
  (void)key;
  (void)specification;
  return nullptr;
}

QList<QSharedPointer<vx::NodePrototype>> PluginFilter::objectPrototypes() {
  QList<QSharedPointer<vx::NodePrototype>> list;
  list.append(CcaResultSelection::getPrototypeSingleton());
  list.append(CreateSurface::getPrototypeSingleton());
  list.append(ColorizeLabeledSurface::getPrototypeSingleton());
  list.append(ColorizeSurfaceFromAttribute::getPrototypeSingleton());
  return list;
}