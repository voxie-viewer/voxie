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

#include <VoxieBackend/Component/ComponentTypeInfo.hpp>

#include <VoxieBackend/IO/Exporter.hpp>

namespace vx {
template <typename T>
class SharedFunPtr;
class PropertyBase;

namespace io {
class VOXIEBACKEND_EXPORT ExtensionExporter : public vx::io::Exporter {
 public:
  ExtensionExporter(const QString& name, const FilenameFilter& filter,
                    const QList<QString>& targetPrototypeNames);
  ~ExtensionExporter() override;

  void validateComponent(
      const QSharedPointer<ComponentContainer>& allComponents) override;

  // throws Exception
  QSharedPointer<OperationResult> exportData(
      const QSharedPointer<vx::Data>& data, const QString& fileName) override;
};

VOXIEBACKEND_EXPORT QSharedPointer<vx::io::ExtensionExporter> parseExporter(
    const QJsonObject& json);

}  // namespace io

template <>
struct VOXIEBACKEND_EXPORT ComponentTypeInfoExt<vx::io::Exporter> {
  static const char* jsonName() { return "Exporter"; }
  static QSharedPointer<vx::Component> parse(
      const QJsonObject& json,
      const vx::SharedFunPtr<QList<QSharedPointer<PropertyBase>>(
          const QJsonObject&)>& parseProperties) {
    (void)parseProperties;
    return vx::io::parseExporter(json);
  }
};
}  // namespace vx
