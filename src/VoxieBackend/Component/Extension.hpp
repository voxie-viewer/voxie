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

#include <VoxieBackend/Component/ComponentContainer.hpp>
#include <VoxieBackend/Component/ComponentTypeInfo.hpp>

#include <QtCore/QProcess>

namespace vx {
class ExternalOperation;
class ComponentType;
class ExtensionLauncher;
class DBusService;
template <typename T>
class SharedFunPtr;
class PropertyBase;

class VOXIEBACKEND_EXPORT Extension : public vx::ComponentContainer {
  Q_OBJECT
  REFCOUNTEDOBJ_DECL(Extension)

  QString scriptFilename_;

  QSharedPointer<ExtensionLauncher> extensionLauncher;
  QSharedPointer<DBusService> dbusService;

  QMap<QString, QList<QSharedPointer<vx::Component>>> components_;
  // TODO: Add QMap<QString, QMap<QString, QSharedPointer<vx::Component>>>
  // componentsByName_?

  QSharedPointer<const QList<QSharedPointer<ComponentType>>> componentTypes_;

 protected:
  void initialize() override;

 public:
  using ComponentTypeList =
      QSharedPointer<const QList<QSharedPointer<ComponentType>>>;

  // TODO: Move more Property functionality into VoxieBackend so that
  // parseProperties is not needed here?

  static QList<QSharedPointer<Extension>> loadFromDir(
      const QString& dir, const ComponentTypeList& typeList,
      const QSharedPointer<ExtensionLauncher>& extensionLauncher,
      const QSharedPointer<DBusService>& dbusService,
      const vx::SharedFunPtr<QList<QSharedPointer<PropertyBase>>(
          const QJsonObject&)>& parseProperties);

  Extension(const QString& scriptFilename, const QString& jsonFilename,
            const ComponentTypeList& typeList,
            const QSharedPointer<ExtensionLauncher>& extensionLauncher,
            const QSharedPointer<DBusService>& dbusService,
            const vx::SharedFunPtr<QList<QSharedPointer<PropertyBase>>(
                const QJsonObject&)>& parseProperties);
  virtual ~Extension();

  // TODO: Rename to executableFilename
  const QString& scriptFilename() const { return scriptFilename_; }

  QProcess* start(
      const QString& action, const QStringList& arguments = QStringList(),
      QProcess* process = new QProcess(),
      const QSharedPointer<QString>& scriptOutput = QSharedPointer<QString>());

  void startOperation(const QSharedPointer<vx::ExternalOperation>& exOp,
                      const QStringList& arguments = QStringList());

  void startOperationDebug(const QSharedPointer<vx::ExternalOperation>& exOp,
                           const QStringList& arguments = QStringList());

  bool offerShowSource() const;

  QList<QSharedPointer<vx::Component>> listComponents(
      const QSharedPointer<ComponentType>& componentType) override;

  QSharedPointer<vx::Component> getComponent(
      const QSharedPointer<ComponentType>& componentType, const QString& name,
      bool allowCompatibilityNames) override;

  QSharedPointer<const QList<QSharedPointer<ComponentType>>> componentTypes()
      override {
    return componentTypes_;
  }
};
}  // namespace vx
