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

// QDBusConnection should be included as early as possible:
// https://bugreports.qt.io/browse/QTBUG-48351
#include <QtDBus/QDBusConnection>

#include "VisualizerNode.hpp"

#include <VoxieClient/DBusAdaptors.hpp>
#include <VoxieClient/Exception.hpp>

#include <VoxieBackend/Data/ImageDataPixelInst.hpp>

#include <Voxie/Node/ParameterCopy.hpp>

#include <Voxie/Gui/WindowMode.hpp>

#include <QtGui/QIcon>

#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QSpinBox>

using namespace vx;

VisualizerRenderOptions::VisualizerRenderOptions(bool isMainView)
    : isMainView_(isMainView) {}
VisualizerRenderOptions::~VisualizerRenderOptions() {}

namespace vx {
namespace {
class VisualizerNodeAdaptorImpl : public VisualizerNodeAdaptor,
                                  public VisualizerObjectAdaptor {
  VisualizerNode* object;

 public:
  VisualizerNodeAdaptorImpl(VisualizerNode* object)
      : VisualizerNodeAdaptor(object),
        VisualizerObjectAdaptor(object),
        object(object) {}
  ~VisualizerNodeAdaptorImpl() override {}

  void RenderScreenshot(const QDBusObjectPath& image,
                        const vx::TupleVector<quint64, 2>& outputRegionStart,
                        const vx::TupleVector<quint64, 2>& size,
                        const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options);

      auto imagePtr = ImageDataPixel::lookup(image);

      auto fun = object->getRenderFunction();
      auto pars = ParameterCopy::getParameters(object);
      auto visOptions = createQSharedPointer<VisualizerRenderOptions>(false);

      // TODO: This should probably be called on a background thread (and send
      // the DBus reply once the execution has finished)
      fun(imagePtr, vx::VectorSizeT2(outputRegionStart), vx::VectorSizeT2(size),
          pars, visOptions);
    } catch (Exception& e) {
      e.handle(object);
      return;
    }
  }

  vx::TupleVector<double, 2> visualizerPosition() const override {
    try {
      return toTupleVector(voxieRoot().getVisualizerPosition(object));
    } catch (vx::Exception& e) {
      qWarning() << "Got error during DBus property:" << e.what();
      return e.handle(object);
    }
  }

  void setVisualizerPosition(vx::TupleVector<double, 2> pos) override {
    try {
      voxieRoot().setVisualizerPosition(object, toVector(pos));
    } catch (vx::Exception& e) {
      qWarning() << "Got error during DBus property:" << e.what();
      e.handle(object);
    }
  }

  vx::TupleVector<double, 2> visualizerSize() const override {
    try {
      return toTupleVector(voxieRoot().getVisualizerSize(object));
    } catch (vx::Exception& e) {
      qWarning() << "Got error during DBus property:" << e.what();
      return e.handle(object);
    }
  }

  void setVisualizerSize(vx::TupleVector<double, 2> size) override {
    try {
      voxieRoot().setVisualizerSize(object, toVector(size));
    } catch (vx::Exception& e) {
      qWarning() << "Got error during DBus property:" << e.what();
      e.handle(object);
    }
  }

  bool isAttached() const override {
    try {
      return voxieRoot().isAttached(object);
    } catch (vx::Exception& e) {
      qWarning() << "Got error during DBus property:" << e.what();
      return e.handle(object);
    }
  }

  void setIsAttached(bool value) override {
    try {
      voxieRoot().setIsAttached(object, value);
    } catch (vx::Exception& e) {
      qWarning() << "Got error during DBus property:" << e.what();
      e.handle(object);
    }
  }

  QString windowMode() const override {
    try {
      return windowModeToString(voxieRoot().getVisualizerWindowMode(object));
    } catch (vx::Exception& e) {
      qWarning() << "Got error during DBus property:" << e.what();
      return e.handle(object);
    }
  }
  void setWindowMode(const QString& value) override {
    try {
      return voxieRoot().setVisualizerWindowMode(object,
                                                 parseWindowMode(value));
    } catch (vx::Exception& e) {
      qWarning() << "Got error during DBus property:" << e.what();
      e.handle(object);
    }
  }
};
}  // namespace
}  // namespace vx

VisualizerNode::VisualizerNode(const QSharedPointer<NodePrototype>& prototype)
    : vx::Node("VisualizerNode", prototype), sections() {
  new VisualizerNodeAdaptorImpl(this);

  // Set default name to visualizer name
  this->setAutomaticDisplayName(this->prototype()->displayName());

  // Add "save screenshot" action
  auto saveScreenshotAction = new QAction("Save screenshot", this);
  QObject::connect(saveScreenshotAction, &QAction::triggered, this,
                   &VisualizerNode::saveScreenshot);
  addContextMenuAction(saveScreenshotAction);
}

QSize VisualizerNode::contentAreaSize() { return this->mainView()->size(); }

void VisualizerNode::saveScreenshot() {
  auto currentSize = contentAreaSize();

  QFileDialog fileDialog(nullptr, Qt::Dialog);
  fileDialog.setOption(QFileDialog::DontUseNativeDialog);
  fileDialog.setAcceptMode(QFileDialog::AcceptSave);
  // TODO: Should the selected filter be used to determine the format (currently
  // this is based on the extension)
  QList<QString> filters;
  filters << "PNG (*.png)";
  filters << "JPEG (*.jpeg)";
  fileDialog.setNameFilters(filters);
  fileDialog.setDefaultSuffix(".png");

  auto layout = dynamic_cast<QGridLayout*>(fileDialog.layout());
  if (!layout) {
    qCritical() << "QFileDialog does not have a QGridLayout as layout";
    return;
  }

  auto sizeLabel = new QLabel("Image size:");
  layout->addWidget(sizeLabel, 4, 0);

  auto sizeLayout = new QGridLayout();
  layout->addLayout(sizeLayout, 4, 1);
  auto widthBox = new QSpinBox();
  sizeLayout->addWidget(widthBox, 0, 1);
  auto heightBox = new QSpinBox();
  sizeLayout->addWidget(heightBox, 0, 2);
  widthBox->setMaximum(9999);
  heightBox->setMaximum(9999);
  widthBox->setValue(currentSize.width());
  heightBox->setValue(currentSize.height());

  if (!fileDialog.exec()) return;

  QString file = fileDialog.selectedFiles().first();
  int width = widthBox->value();
  int height = heightBox->value();

  try {
    auto fun = this->getRenderFunction();
    auto pars = ParameterCopy::getParameters(this);
    auto options = createQSharedPointer<VisualizerRenderOptions>(false);

    // TODO: do this in a background thread?
    auto img =
        ImageDataPixel::createInst(width, height, 4, DataType::Float32, false);
    fun(img, vx::VectorSizeT2(0, 0), vx::VectorSizeT2(width, height), pars,
        options);
    auto qimg = img->convertToQImage();
    if (!qimg.save(file))
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Error while saving file");
  } catch (vx::Exception& e) {
    qCritical() << "Error while saving screenshot:" << e.what();
    QMessageBox(QMessageBox::Critical, "Error while saving Screenshot",
                QString() + "Error while saving screenshot: " + e.what(),
                QMessageBox::Ok, mainView())
        .exec();
  }
}

VisualizerNode::~VisualizerNode() {}

QList<QString> VisualizerNode::supportedDBusInterfaces() {
  return {
      "de.uni_stuttgart.Voxie.VisualizerNode",
      "de.uni_stuttgart.Voxie.VisualizerObject",
  };
}

SharedFunPtr<VisualizerNode::RenderFunction>
VisualizerNode::getRenderFunction() {
  throw vx::Exception(
      "de.uni_stuttgart.Voxie.NotImplemented",
      "getRenderFunction() is not implemented for " + prototype()->name());
}

bool VisualizerNode::isAllowedChild(vx::NodeKind) { return false; }

bool VisualizerNode::isAllowedParent(vx::NodeKind kind) {
  return kind == NodeKind::Data || kind == NodeKind::Property;
}

QVector<QWidget*>& VisualizerNode::dynamicSections() { return this->sections; }
