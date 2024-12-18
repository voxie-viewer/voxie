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

#include <Voxie/Data/Prototypes.forward.hpp>

#include <Voxie/Node/DataNode.hpp>

namespace vx {
class FileData;

class VOXIECORESHARED_EXPORT FileNode : public DataNode {
  Q_OBJECT
  VX_NODE_IMPLEMENTATION("de.uni_stuttgart.Voxie.Data.File")

 private:
  QSharedPointer<FileData> dataPointer;

  QWidget* infoWidget;

 public:
  explicit FileNode();
  ~FileNode();

  QSharedPointer<Data> data() override;

  void setFileData(const QSharedPointer<FileData>& data);
  const QSharedPointer<FileData>& geometricPrimitiveData() const {
    return dataPointer;
  }

 protected:
  void setDataImpl(const QSharedPointer<Data>& data) override;

  QWidget* getCustomPropertySectionContent(const QString& name) override;
};
}  // namespace vx
