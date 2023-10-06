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

#include <VoxieBackend/Component/Component.hpp>
#include <VoxieBackend/Component/ComponentTypeInfo.hpp>

#include <VoxieClient/ObjectExport/ExportedObject.hpp>

#include <QtCore/QObject>

namespace vx {
namespace filter {
class Filter2D;
}
namespace plugin {

/**
 * Prototype class for a Filter. Every Filter needs to have a MetaFilter of its
 * own. A Plugin featuring Filters needs to provide a list of metafilters to
 * make the filter known to Voxie.
 */
class VOXIECORESHARED_EXPORT MetaFilter2D : public vx::plugin::Component {
  Q_OBJECT
 public:
  explicit MetaFilter2D(const QString& name);
  virtual ~MetaFilter2D();

  /** prototype method for the filter */
  virtual filter::Filter2D* createFilter() = 0;

  virtual QString displayName() = 0;

  QList<QString> supportedComponentDBusInterfaces() override;
};
}  // namespace plugin

template <>
struct ComponentTypeInfo<vx::plugin::MetaFilter2D> {
  static const char* name() {
    return "de.uni_stuttgart.Voxie.ComponentType.Filter2DPrototype";
  }
  static const QList<std::tuple<QString, bool>> compatibilityNames() {
    return {
        std::make_tuple("de.uni_stuttgart.Voxie.Filter2DPrototype", true),
    };
  }
};

}  // namespace vx
