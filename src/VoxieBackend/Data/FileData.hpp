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

#include <VoxieBackend/Data/Data.hpp>

#include <memory>

namespace vx {
class VOXIEBACKEND_EXPORT FileData : public Data {
  Q_OBJECT
  VX_REFCOUNTEDOBJECT

 protected:
  FileData();

 public:
  ~FileData();
};

class VOXIEBACKEND_EXPORT FileDataByteStream : public FileData {
  Q_OBJECT
  VX_REFCOUNTEDOBJECT

 private:
  QString mediaType_;
  QSharedPointer<SharedMemory> data_;

 public:
  // TODO: should this be private so that only the create() method can call it?
  FileDataByteStream(const QString& mediaType, std::size_t lengthBytes);

 public:
  ~FileDataByteStream();

  const QString& mediaType() { return mediaType_; }

  const QSharedPointer<SharedMemory>& data() { return data_; }

  QList<QString> supportedDBusInterfaces() override;

  std::size_t size();

  QList<QSharedPointer<SharedMemory>> getSharedMemorySections() override;
};
}  // namespace vx
