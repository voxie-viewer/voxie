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

#include "Vis3DPlugin.hpp"

#include <PluginVis3D/GeometricPrimitive.hpp>
#include <PluginVis3D/Grid.hpp>
#include <PluginVis3D/Plane.hpp>
#include <PluginVis3D/Surface.hpp>
#include <PluginVis3D/Test3DObject.hpp>
#include <PluginVis3D/Visualizer3D.hpp>
#include <PluginVis3D/VolumeRenderingVisualizer.hpp>

#include <VoxieBackend/OpenCL/CLInstance.hpp>

using namespace vx::plugin;
using namespace vx::opencl;

Vis3DPlugin::Vis3DPlugin(QObject* parent) : QGenericPlugin(parent) {
  Q_INIT_RESOURCE(PluginVis3D);
  try {
    if (CLInstance::getDefaultInstance()->isValid())
      CLInstance::getDefaultInstance()->createProgramFromFile(
          ":/XRay.cl", "", "voxie3d::x-ray-3d");
  } catch (CLException& ex) {
    qWarning() << ex;
  }
}

QObject* Vis3DPlugin::create(const QString& name, const QString& spec) {
  (void)name;
  (void)spec;
  return nullptr;
}

QList<QSharedPointer<vx::NodePrototype>> Vis3DPlugin::objectPrototypes() {
  QList<QSharedPointer<vx::NodePrototype>> list;
  list.append(Visualizer3D::getPrototypeSingleton());
  list.append(VolumeRenderingVisualizer::getPrototypeSingleton());
  list.append(Test3DObject::getPrototypeSingleton());
  list.append(vx::vis3d::Plane::getPrototypeSingleton());
  list.append(vx::vis3d::Surface::getPrototypeSingleton());
  list.append(vx::vis3d::Grid::getPrototypeSingleton());
  list.append(vx::vis3d::GeometricPrimitive::getPrototypeSingleton());
  return list;
}
