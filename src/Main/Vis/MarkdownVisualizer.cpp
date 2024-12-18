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

#include "MarkdownVisualizer.hpp"

// QDBusConnection should be included as early as possible:
// https://bugreports.qt.io/browse/QTBUG-48351
#include <QtDBus/QDBusConnection>

#include <Main/Prototypes.hpp>
#include <Main/Root.hpp>

#include <Main/Help/CMark.hpp>

#include <Voxie/Data/FileNode.hpp>

#include <Voxie/Gui/ErrorMessage.hpp>

#include <Voxie/Node/ParameterCopy.hpp>
#include <Voxie/Node/PropertyHelper.hpp>
#include <Voxie/Node/PropertyUI.hpp>

#include <VoxieBackend/Data/FileData.hpp>
#include <VoxieBackend/Data/ImageDataPixel.hpp>

#include <VoxieClient/Format.hpp>

VX_NODE_INSTANTIATION(vx::MarkdownVisualizer)

using namespace vx;

MarkdownVisualizer::MarkdownVisualizer()
    : SimpleVisualizer(getPrototypeSingleton()),
      properties(new visualizer_prop::MarkdownProperties(this)) {
  this->viewRoot = new QWidget;
  QObject::connect(this, &QObject::destroyed, viewRoot, &QObject::deleteLater);
  this->viewRoot->setMinimumSize(300 / 96.0 * this->viewRoot->logicalDpiX(),
                                 200 / 96.0 * this->viewRoot->logicalDpiY());

  auto backend = Root::instance()->helpBrowserBackend();
  if (!backend) {
    qWarning() << "No help browser backend found";
  }
  if (backend) {
    backendView = backend->createView();
    // TODO
    /*
    QObject::connect(
        backendView.data(), &HelpBrowserBackendView::handleLink, this,
        [this](const QUrl& url) { this->linkHandler.handleLink(url); });
        */
    // TODO: Handle reload somehow?
    /*
    QObject::connect(backendView.data(), &HelpBrowserBackendView::doReload,
                     this, [this]() { this->openHelpForUri(this->lastUri); });
    */
  }

  // this->setWindowTitle("File data");

  auto layout = new QVBoxLayout;
  layout->setContentsMargins(QMargins(0, 0, 0, 0));
  if (backendView) {
    layout->addWidget(backendView->widget());
  } else {
    auto label = new QLabel("No help browser backend found");
    label->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    layout->addWidget(label);
  }
  viewRoot->setLayout(layout);
  viewRoot->setWindowFlags(Qt::Window);

  connect(this, &MarkdownVisualizer::fileDisplayNameChanged, this, [this]() {
    auto fileNode = properties->file();

    QString name;
    if (fileNode)
      name = "MarkdownVisualizer - " + fileNode->displayName();
    else
      name = "MarkdownVisualizer - Not connected";
    this->setAutomaticDisplayName(name);
  });

  connect(this, &MarkdownVisualizer::fileDataChangedFinished, this, [this]() {
    auto fileNode = properties->file();
    auto fileNodeData = dynamic_cast<FileNode*>(fileNode);
    auto data = fileNodeData ? fileNodeData->data() : QSharedPointer<Data>();
    auto dataByteStream = qSharedPointerDynamicCast<FileDataByteStream>(data);
    if (dataByteStream &&
        dataByteStream->mediaType().startsWith("text/markdown")) {
      auto shmem = dataByteStream->data();
      std::size_t size = shmem->getSizeBytes();
      QString markdown;
      // TODO: Use charset parameter?
      markdown = QString::fromUtf8((char*)shmem->getData(), size);

      QString title = "File data";

      // QUrl baseUrl("invalid:");
      // TODO: This is currently needed for simple.css, clean this up
      QUrl baseUrl("file:///invalid/");

      // TODO: Clean up the help page opening code?
      QString html;
      try {
        html = vx::cmark::parseDocumentWithExtensions(markdown)->renderHtml();
      } catch (vx::Exception& e) {
        html = "";
        vx::showErrorMessage("Got exception while rendering markdown data", e);
      }

      // TODO
      QString simpleCssBase =
          QUrl::fromLocalFile(voxieRoot().directoryManager()->simpleCssPath())
              .toString() +
          "/";
      auto htmlFull = vx::format(
                          "<link rel=\"stylesheet\" "
                          "href=\"{}/simple.min.css\">\n",
                          simpleCssBase) +
                      html;
      if (this->backendView) this->backendView->setHtml(htmlFull, baseUrl);
      // qDebug() << htmlFull.toUtf8().data();
    } else {
      if (this->backendView)
        this->backendView->setHtml("", QUrl("file:///invalid/"));
    }
  });

  forwardSignalFromPropertyNodeOnReconnect(
      properties, &vx::visualizer_prop::MarkdownProperties::file,
      &vx::visualizer_prop::MarkdownProperties::fileChanged,
      &DataNode::dataChangedFinished, this,
      &MarkdownVisualizer::fileDataChangedFinished);
  forwardSignalFromPropertyNodeOnReconnect(
      properties, &vx::visualizer_prop::MarkdownProperties::file,
      &vx::visualizer_prop::MarkdownProperties::fileChanged,
      &Node::displayNameChanged, this,
      &MarkdownVisualizer::fileDisplayNameChanged, true);
}

MarkdownVisualizer::~MarkdownVisualizer() {}

vx::SharedFunPtr<VisualizerNode::RenderFunction>
MarkdownVisualizer::getRenderFunction() {
  return [](const QSharedPointer<ImageDataPixel>& outputImage,
            const VectorSizeT2& outputRegionStart, const VectorSizeT2& size,
            const QSharedPointer<ParameterCopy>& parameters,
            const QSharedPointer<VisualizerRenderOptions>& options) {
    // TODO: implement render function
    Q_UNUSED(parameters);
    Q_UNUSED(options);

    QImage image(static_cast<int>(size.x), static_cast<int>(size.y),
                 QImage::Format_ARGB32);
    image.fill(qRgba(0, 0, 0, 0));

    outputImage->fromQImage(image, outputRegionStart);
  };
}

QWidget* MarkdownVisualizer::mainView() { return this->viewRoot; }
