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

#include <VoxieBackend/IO/Importer.hpp>

namespace vx {
template <typename T>
class SharedFunPtr;
class PropertyBase;

namespace io {
class VOXIEBACKEND_EXPORT ExtensionImporter : public vx::io::Importer {
 public:
  ExtensionImporter(const QString& name, const FilenameFilter& filter,
                    const QList<QSharedPointer<PropertyBase>>& properties);
  ~ExtensionImporter() override;

  // throws Exception
  QSharedPointer<OperationResultImport> import(
      const QString& fileName, bool doCreateNode,
      const QMap<QString, QVariant>& properties) override;
};

VOXIEBACKEND_EXPORT QSharedPointer<vx::io::ExtensionImporter> parseImporter(
    const QJsonObject& json,
    const vx::SharedFunPtr<QList<QSharedPointer<PropertyBase>>(
        const QJsonObject&)>& parseProperties);
}  // namespace io

template <>
struct VOXIEBACKEND_EXPORT ComponentTypeInfoExt<vx::io::Importer> {
  static const char* jsonName() { return "Importer"; }
  static QSharedPointer<vx::Component> parse(
      const QJsonObject& json,
      const vx::SharedFunPtr<QList<QSharedPointer<PropertyBase>>(
          const QJsonObject&)>& parseProperties) {
    return vx::io::parseImporter(json, parseProperties);
  }
};
}  // namespace vx
