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

#include <VoxieBackend/Data/EventListDataBuffer.hpp>
#include <VoxieBackend/IO/Exporter.hpp>

#include <QPair>
#include <QSharedPointer>
#include <QThreadStorage>

namespace vx {
class EventListDataAccessor;

namespace file {
class ExportEventListCsv : public vx::io::Exporter {
  Q_OBJECT

 public:
  ExportEventListCsv();

  QSharedPointer<vx::OperationResult> exportData(
      const QSharedPointer<vx::Data>& data, const QString& fileName) override;

 private:
  class ExporterImpl {
   public:
    ExporterImpl(vx::EventListDataAccessor& accessor, vx::io::Operation& op,
                 QString fileName);
    void save();

   private:
    using Timestamp = qint64;

    static constexpr Timestamp TimestampBlockSize = 1e7;
    static constexpr qulonglong BlockSize = 1024 * 1024;

    struct Block {
      ExporterImpl* exporter;
      quint64 streamID;
      Timestamp start;
      Timestamp end;
    };

    void appendStream(qulonglong streamID, QIODevice& output);
    static QSharedPointer<QByteArray> handleBlockInThread(Block block);

    vx::EventListDataAccessor& accessor;
    vx::io::Operation& op;
    QString fileName;
    qulonglong streamCount;
    QList<EventListDataBuffer::AttributeInfo> attributes;

    static QThreadStorage<QSharedPointer<EventListDataBuffer>> bufferStorage;
  };
};
}  // namespace file
}  // namespace vx
