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

#include <VoxieBackend/IO/Operation.hpp>
#include <VoxieBackend/IO/OperationResult.hpp>

namespace vx {
class Data;
class DataNode;
class DataVersion;
class OperationResultImport;

class VOXIEBACKEND_EXPORT OperationImport : public vx::io::Operation {
  Q_OBJECT
  VX_REFCOUNTEDOBJECT

 public:
  class VOXIEBACKEND_EXPORT Result : public Operation::ResultSuccess {
    QSharedPointer<vx::Data> data_;
    QSharedPointer<vx::DataVersion> dataVersion_;

    QMutex mutex;

    // TODO: This should be a QSharedPointer
    // QSharedPointer<vx::DataNode> dataNode;
    vx::DataNode* dataNode_;
    // Needed because DataNode is not in VoxieBackend
    QDBusObjectPath dataNodePath_;

    bool isFinal = false;

   public:
    Result(const QSharedPointer<vx::Data>& data,
           const QSharedPointer<vx::DataVersion>& dataVersion);
    ~Result() override;

    const QSharedPointer<vx::Data>& data() const { return data_; }
    const QSharedPointer<vx::DataVersion>& dataVersion() const {
      return dataVersion_;
    }

    vx::DataNode* dataNode();
    QDBusObjectPath dataNodePath();
    void setDataNode(vx::DataNode* obj, const QDBusObjectPath& dataNodePath);
    void makeFinal();
  };

  using ResultWrapper = OperationResultImport;

  OperationImport();
  ~OperationImport();
};

// Note: This is a separate class to allow forward declarations to it
class VOXIEBACKEND_EXPORT OperationResultImport : public OperationResult {
  VX_REFCOUNTEDOBJECT

  bool doCreateNode_;

 public:
  OperationResultImport(const QSharedPointer<OperationImport>& operation,
                        bool doCreateNode);
  ~OperationResultImport() override;

  bool doCreateNode() const { return doCreateNode_; }

  QSharedPointer<OperationImport::Result> resultSuccess();
};
}  // namespace vx
