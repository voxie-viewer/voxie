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

#include <VoxieBackend/IO/FilenameFilter.hpp>

#include <VoxieBackend/Component/Component.hpp>
#include <VoxieBackend/Component/ComponentTypeInfo.hpp>

#include <VoxieClient/DBusTypeList.hpp>

#include <QtCore/QObject>

namespace vx {
// Forward declarations
class PropertyBase;
class OperationResultImport;
template <typename T>
class SharedFunPtr;
namespace io {
class Operation;
}

namespace io {

class VOXIEBACKEND_EXPORT Importer : public vx::plugin::Component {
  Q_OBJECT

  FilenameFilter filter_;

  // TODO: Component should be RefCountedObject
  QWeakPointer<Importer> self;

  QList<QSharedPointer<PropertyBase>> properties_;

 public:
  // TODO: Also describe non-extension importers using JSON, similar to
  // ObjectPrototypes
  explicit Importer(const QString& name, const FilenameFilter& filter,
                    const QList<QSharedPointer<PropertyBase>>& properties);
  virtual ~Importer();

  const FilenameFilter& filter() { return filter_; }

  const QWeakPointer<Importer>& getSelf() { return self; }
  void setSelf(const QSharedPointer<Importer>& ptr);

  // throws Exception
  // TODO: Pass in the operation object instead of returning the
  // OperationResultImport object?
  virtual QSharedPointer<OperationResultImport> import(
      const QString& fileName, bool doCreateNode,
      const QMap<QString, QVariant>& properties) = 0;

  static QSharedPointer<OperationResultImport> runThreaded(
      const QString& description, bool doCreateNode,
      std::function<QSharedPointer<vx::Data>(const QSharedPointer<Operation>&)>
          f);

  const QList<QSharedPointer<PropertyBase>>& properties() const {
    return properties_;
  }

  QSharedPointer<PropertyBase> getProperty(const QString& name,
                                           bool allowCompatibilityNames);

  /**
   * Register a handle which will be called if StartImport is called with
   * 'CreateNode' set to true
   */
  using CreateNodeHandler = vx::SharedFunPtr<void(
      const QSharedPointer<OperationResultImport>&, const QString&,
      const QSharedPointer<Importer>&, const QMap<QString, QVariant>&)>;
  static void registerCreateNodeHandler(const CreateNodeHandler& fun);

  QList<QString> supportedComponentDBusInterfaces() override;
};

// ImporterAdaptor is in Main/io/importeradaptor.hpp
}  // namespace io

template <>
struct ComponentTypeInfo<vx::io::Importer> {
  static const char* name() {
    return "de.uni_stuttgart.Voxie.ComponentType.Importer";
  }
  static const QList<std::tuple<QString, bool>> compatibilityNames() {
    return {
        std::make_tuple("de.uni_stuttgart.Voxie.Importer", true),
    };
  }
};
}  // namespace vx
