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

#include "OperationImport.hpp"

#include <VoxieBackend/Data/Data.hpp>

#include <VoxieClient/DBusAdaptors.hpp>

using namespace vx;
using namespace vx::io;

OperationImport::OperationImport() {}
OperationImport::~OperationImport() {}

OperationImport::Result::Result(
    const QSharedPointer<vx::Data>& data,
    const QSharedPointer<vx::DataVersion>& dataVersion)
    : data_(data),
      dataVersion_(dataVersion),
      dataNode_(nullptr),
      dataNodePath_("/") {}
OperationImport::Result::~Result() {}

vx::DataNode* OperationImport::Result::dataNode() {
  QMutexLocker locker(&mutex);
  return dataNode_;
}
QDBusObjectPath OperationImport::Result::dataNodePath() {
  QMutexLocker locker(&mutex);
  return dataNodePath_;
}
void OperationImport::Result::setDataNode(vx::DataNode* obj,
                                          const QDBusObjectPath& dataNodePath) {
  QMutexLocker locker(&mutex);
  if (isFinal)
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.InternalError",
        "OperationImport::Result::setDataNode() called on a final object");
  dataNode_ = obj;
  dataNodePath_ = dataNodePath;
}
// TODO: This currently does not get called
void OperationImport::Result::makeFinal() {
  QMutexLocker locker(&mutex);
  isFinal = true;
}

namespace vx {
class OperationResultImportAdaptorImpl : public OperationResultImportAdaptor {
  OperationResultImport* object;

 public:
  OperationResultImportAdaptorImpl(OperationResultImport* object)
      : OperationResultImportAdaptor(object), object(object) {}
  ~OperationResultImportAdaptorImpl() override {}

  QDBusObjectPath data() const override {
    try {
      return vx::ExportedObject::getPath(object->resultSuccess()->data());
    } catch (Exception& e) {
      return e.handle(object);
    }
  }

  QDBusObjectPath dataVersion() const override {
    try {
      return vx::ExportedObject::getPath(
          object->resultSuccess()->dataVersion());
    } catch (Exception& e) {
      return e.handle(object);
    }
  }
};

class OperationResultImportNodeAdaptorImpl
    : public OperationResultImportNodeAdaptor,
      public OperationResultImportObjectAdaptor {
  OperationResultImport* object;

 public:
  OperationResultImportNodeAdaptorImpl(OperationResultImport* object)
      : OperationResultImportNodeAdaptor(object),
        OperationResultImportObjectAdaptor(object),
        object(object) {}
  ~OperationResultImportNodeAdaptorImpl() override {}

  QDBusObjectPath dataNode() const override {
    try {
      return object->resultSuccess()->dataNodePath();
    } catch (Exception& e) {
      return e.handle(object);
    }
  }
  QDBusObjectPath dataObject() const override {
    try {
      return object->resultSuccess()->dataNodePath();
    } catch (Exception& e) {
      return e.handle(object);
    }
  }
};

class DynamicObjectAdaptorImplOperationResultImport
    : public DynamicObjectAdaptor {
  OperationResultImport* object;

 public:
  DynamicObjectAdaptorImplOperationResultImport(OperationResultImport* object)
      : DynamicObjectAdaptor(object), object(object) {}
  ~DynamicObjectAdaptorImplOperationResultImport() override {}

  QStringList supportedInterfaces() const override {
    try {
      QStringList result{"de.uni_stuttgart.Voxie.OperationResultImport"};
      if (object->doCreateNode()) {
        result << "de.uni_stuttgart.Voxie.OperationResultImportNode";
        result << "de.uni_stuttgart.Voxie.OperationResultImportObject";
      }
      return result;
    } catch (Exception& e) {
      return e.handle(object);
    }
  }
};
}  // namespace vx

QSharedPointer<OperationImport::Result> OperationResultImport::resultSuccess() {
  auto result = qSharedPointerDynamicCast<OperationImport::Result>(
      OperationResult::resultSuccess());
  if (!result)
    throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                        "Unable to cast to OperationImport::Result");
  return result;
}

OperationResultImport::OperationResultImport(
    const QSharedPointer<OperationImport>& operation, bool doCreateNode)
    : OperationResult(operation), doCreateNode_(doCreateNode) {
  new DynamicObjectAdaptorImplOperationResultImport(this);
  new OperationResultImportAdaptorImpl(this);
  if (doCreateNode) new OperationResultImportNodeAdaptorImpl(this);
}
OperationResultImport::~OperationResultImport() {}
