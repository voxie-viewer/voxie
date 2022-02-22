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

#include <VoxieBackend/VoxieBackend.hpp>

#include <VoxieBackend/Data/Data.hpp>

#include <VoxieBackend/IO/Importer.hpp>

#include <VoxieBackend/Component/Component.hpp>
#include <VoxieBackend/Component/ComponentTypeInfo.hpp>

#include <VoxieClient/DBusTypeList.hpp>

#include <QtCore/QObject>

namespace vx {
// Forward declarations
class NodePrototype;
class OperationResult;
namespace io {
class Operation;
class Exporter;
}  // namespace io

namespace io {
class VOXIEBACKEND_EXPORT Exporter : public vx::plugin::Component {
  Q_OBJECT
 private:
  FilenameFilter filter_;
  QList<QString> targetPrototypeNames_;

  QWeakPointer<Exporter> self;

 public:
  explicit Exporter(const QString& name, const FilenameFilter& filter,
                    const QList<QString>& targetPrototypeNames);
  virtual ~Exporter();

  const FilenameFilter& filter() { return filter_; }

  const QList<QString>& targetPrototypeNames() const {
    return targetPrototypeNames_;
  }

  bool matches(const QString& prototypeName);

  const QWeakPointer<Exporter>& getSelf() { return self; }
  void setSelf(const QSharedPointer<Exporter>& ptr);

  // throws Exception
  virtual QSharedPointer<OperationResult> exportData(
      const QSharedPointer<vx::Data>& data, const QString& fileName) = 0;

  static QSharedPointer<OperationResult> runThreaded(
      const QString& description,
      std::function<void(const QSharedPointer<Operation>&)> f);

  QList<QString> supportedComponentDBusInterfaces() override;
};
}  // namespace io

template <>
struct ComponentTypeInfo<vx::io::Exporter> {
  static const char* name() {
    return "de.uni_stuttgart.Voxie.ComponentType.Exporter";
  }
  static const QList<std::tuple<QString, bool>> compatibilityNames() {
    return {
        std::make_tuple("de.uni_stuttgart.Voxie.Exporter", true),
    };
  }
};
}  // namespace vx
