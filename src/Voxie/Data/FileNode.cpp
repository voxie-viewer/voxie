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

#include "FileNode.hpp"

#include <Voxie/Data/Prototypes.hpp>

#include <Voxie/Node/NodePrototype.hpp>

#include <VoxieBackend/Data/FileData.hpp>
#include <VoxieBackend/Data/SharedMemory.hpp>

#include <QtWidgets/QPushButton>
#include <QtWidgets/QTextEdit>

VX_NODE_INSTANTIATION(vx::FileNode)

using namespace vx;

FileNode::FileNode()
    : DataNode("FileNode", getPrototypeSingleton()),
      properties(new FileProperties(this)) {
  this->setAutomaticDisplayName("File");

  this->infoWidget = new QWidget();

  auto layout = new QVBoxLayout();
  layout->setContentsMargins(0, 0, 0, 0);
  this->infoWidget->setLayout(layout);

  auto mediaTypeLabel = new QLabel();
  layout->addWidget(mediaTypeLabel);
  auto mediaTypeLabelUpdate = [mediaTypeLabel](
                                  const QSharedPointer<Data>& data,
                                  const QSharedPointer<DataVersion>& newVersion,
                                  DataChangedReason reason) {
    Q_UNUSED(newVersion);
    if (reason != DataChangedReason::DataInstanceChanged) return;
    auto dataByteStream = qSharedPointerDynamicCast<FileDataByteStream>(data);
    if (!dataByteStream)
      mediaTypeLabel->setText("");
    else
      mediaTypeLabel->setText(
          "Media type: " +
          QString(dataByteStream->mediaType()).replace("; ", ";\n    "));
  };
  QObject::connect(this, &DataNode::dataChangedFinished, this,
                   mediaTypeLabelUpdate);
  mediaTypeLabelUpdate(this->data(), QSharedPointer<DataVersion>(),
                       DataChangedReason::DataInstanceChanged);

  auto sizeLabel = new QLabel();
  layout->addWidget(sizeLabel);
  auto sizeLabelUpdate = [sizeLabel](
                             const QSharedPointer<Data>& data,
                             const QSharedPointer<DataVersion>& newVersion,
                             DataChangedReason reason) {
    Q_UNUSED(newVersion);
    if (reason != DataChangedReason::DataInstanceChanged) return;
    auto dataByteStream = qSharedPointerDynamicCast<FileDataByteStream>(data);
    if (!dataByteStream)
      sizeLabel->setText("");
    else
      sizeLabel->setText("Size: " + QString::number(dataByteStream->size()) +
                         " bytes");
  };
  QObject::connect(this, &DataNode::dataChangedFinished, this, sizeLabelUpdate);
  sizeLabelUpdate(this->data(), QSharedPointer<DataVersion>(),
                  DataChangedReason::DataInstanceChanged);

  auto previewTextEdit = new QTextEdit();
  layout->addWidget(previewTextEdit);
  previewTextEdit->setReadOnly(true);
  auto previewTextEditUpdate =
      [previewTextEdit](const QSharedPointer<Data>& data,
                        const QSharedPointer<DataVersion>& newVersion,
                        DataChangedReason reason) {
        Q_UNUSED(newVersion);
        Q_UNUSED(reason);

        auto dataByteStream =
            qSharedPointerDynamicCast<FileDataByteStream>(data);
        if (!dataByteStream ||
            !dataByteStream->mediaType().startsWith("text/")) {
          previewTextEdit->setPlainText("");
          previewTextEdit->setVisible(false);
        } else {
          auto shmem = dataByteStream->data();
          std::size_t size = shmem->getSizeBytes();
          std::size_t max = 10000;
          QString str;
          // TODO: Use charset parameter?
          if (size > max) {
            // TODO: Handle partial data at the end better?
            str = QString::fromUtf8((char*)shmem->getData(), max) + "…";
          } else {
            str = QString::fromUtf8((char*)shmem->getData(), size);
          }
          previewTextEdit->setPlainText(str);
          previewTextEdit->setVisible(true);
        }
      };
  QObject::connect(this, &DataNode::dataChangedFinished, this,
                   previewTextEditUpdate);
  previewTextEditUpdate(this->data(), QSharedPointer<DataVersion>(),
                        DataChangedReason::DataInstanceChanged);
}
FileNode::~FileNode() {}

QSharedPointer<Data> FileNode::data() { return dataPointer; }

void FileNode::setDataImpl(const QSharedPointer<Data>& data) {
  auto dataCast = qSharedPointerDynamicCast<FileData>(data);
  if (!dataCast && data)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Not a FileData object");
  dataPointer = dataCast;
}

void FileNode::setFileData(const QSharedPointer<FileData>& data) {
  setData(data);
}

QWidget* FileNode::getCustomPropertySectionContent(const QString& name) {
  if (name == "de.uni_stuttgart.Voxie.Data.File.ContentInfo") {
    return infoWidget;
  } else {
    return DataNode::getCustomPropertySectionContent(name);
  }
}
