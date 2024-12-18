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

#include "Visualizer3D.hpp"

#include <Voxie/SpNav/SpaceNavVisualizer.hpp>

#include <VoxieClient/Exception.hpp>

#include <Voxie/IO/SaveFileDialog.hpp>

#include <PluginVis3D/View3DPropertiesConnection.hpp>

#include <QtOpenGL/QGLFormat>

#include <QSignalMapper>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QVBoxLayout>

#include <QtCore>

#include <iostream>
#include <regex>
#include <sstream>

#ifdef ENABLE_OSVR
#include <PluginVis3D/Osvr/OsvrDisplay.hpp>
#endif

VX_NODE_INSTANTIATION(Visualizer3D)

using namespace vx;
using namespace vx::vis3d;
using namespace vx::visualization;

// TODO: Make axisFilter work again?

Visualizer3D::Visualizer3D()
    : VisualizerNode(getPrototypeSingleton()),
      properties(new View3DProperties(this)) {
  {
    this->view = new Visualizer3DView(
        properties, new vx::visualization::View3D(this, View3DProperty::All),
        &axisFilter);
    new View3DPropertiesConnection(this->properties, this->view->getView3D());
    this->view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    this->setAutomaticDisplayName("3D Visualizer");
    this->view->setMinimumSize(400 / 96.0 * this->view->logicalDpiX(),
                               300 / 96.0 * this->view->logicalDpiY());
  }

  {
    this->control3DSettings = new QWidget();
    auto controlLayout = new QGridLayout();

    auto orthoProjectionBox = new QCheckBox("&Orthographic projection");
    controlLayout->addWidget(orthoProjectionBox, 3, 0, 1, 3);
    orthoProjectionBox->setToolTip(
        "Toggles orthographic projection (o, Numpad 5)");
    // Update the box if the property value changes
    connect(
        this->properties, &View3DProperties::fieldOfViewChanged, this,
        [this, orthoProjectionBox]() {
          orthoProjectionBox->setChecked(this->properties->fieldOfView() == 0);
        });
    // Set initial value
    orthoProjectionBox->setChecked(this->properties->fieldOfView() == 0);
    // Change the projection when the user clicks the checkbox
    // TODO: Add animation?
    connect(orthoProjectionBox, &QCheckBox::clicked, this,
            [this, orthoProjectionBox]() {
              if (orthoProjectionBox->isChecked())
                this->properties->setFieldOfView(0);
              else if (this->properties->fieldOfView() == 0)
                this->properties->setFieldOfView(parseVariant<double>(
                    this->properties->fieldOfViewProperty()->defaultValue()));
            });

    auto frontViewButton = new QPushButton("Front");
    frontViewButton->setToolTip("Show front view (Numpad 1)");
    auto rightViewButton = new QPushButton("Right");
    rightViewButton->setToolTip("Show right view (Numpad 3)");
    auto topViewButton = new QPushButton("Top");
    topViewButton->setToolTip("Show top view (Numpad 7)");
    auto backViewButton = new QPushButton("Back");
    backViewButton->setToolTip("Show back view (Ctrl + Numpad 1)");
    auto leftViewButton = new QPushButton("Left");
    leftViewButton->setToolTip("Show left view (Ctrl + Numpad 3)");
    auto bottomViewButton = new QPushButton("Bottom");
    bottomViewButton->setToolTip("Show bottom view (Ctrl + Numpad 7)");

    // The SignalMapper lets us call a function with arguments when a button
    // is pressed
    QSignalMapper* signalMapper = new QSignalMapper(this);
    connect(frontViewButton, SIGNAL(pressed()), signalMapper, SLOT(map()));
    connect(backViewButton, SIGNAL(pressed()), signalMapper, SLOT(map()));
    connect(rightViewButton, SIGNAL(pressed()), signalMapper, SLOT(map()));
    connect(leftViewButton, SIGNAL(pressed()), signalMapper, SLOT(map()));
    connect(topViewButton, SIGNAL(pressed()), signalMapper, SLOT(map()));
    connect(bottomViewButton, SIGNAL(pressed()), signalMapper, SLOT(map()));

    // Mapping the view angle arguments to the buttons
    signalMapper->setMapping(frontViewButton, "front");
    signalMapper->setMapping(backViewButton, "back");
    signalMapper->setMapping(rightViewButton, "right");
    signalMapper->setMapping(leftViewButton, "left");
    signalMapper->setMapping(topViewButton, "top");
    signalMapper->setMapping(bottomViewButton, "bottom");

    connect(signalMapper, SIGNAL(mapped(QString)), this,
            SLOT(handleViewChange(QString)));

    controlLayout->addWidget(frontViewButton, 1, 0);
    controlLayout->addWidget(rightViewButton, 1, 1);
    controlLayout->addWidget(topViewButton, 1, 2);
    controlLayout->addWidget(backViewButton, 2, 0);
    controlLayout->addWidget(leftViewButton, 2, 1);
    controlLayout->addWidget(bottomViewButton, 2, 2);

    QPushButton* resetButton = new QPushButton("Reset zoom");
    resetButton->setToolTip(
        "Reset zoom and position to show everything (Home, Numpad 0)");
    QObject::connect(resetButton, &QPushButton::clicked, this, [this]() {
      this->view->getView3D()->resetView(View3DProperty::LookAt |
                                         View3DProperty::ZoomLog);
    });
    controlLayout->addWidget(resetButton, 0, 0);

    QPushButton* zoomToOneButton = new QPushButton("Zoom 1");
    zoomToOneButton->setToolTip("Set zoom to 1");
    QObject::connect(zoomToOneButton, &QPushButton::clicked, this,
                     [this]() { this->properties->setZoomLog(0); });
    controlLayout->addWidget(zoomToOneButton, 0, 1);

    QPushButton* resetViewSizeButton = new QPushButton("Reset view size");
    resetViewSizeButton->setToolTip(
        "Reset unzoomed view size to default (250mm)");
    QObject::connect(resetViewSizeButton, &QPushButton::clicked, this,
                     [this]() { this->properties->setViewSizeUnzoomed(0.25); });
    controlLayout->addWidget(resetViewSizeButton, 4, 0);

    QPushButton* viewSizePhysicalButton = new QPushButton("Physical view size");
    viewSizePhysicalButton->setToolTip(
        "Set unzoomed view size to physical size");
    QObject::connect(
        viewSizePhysicalButton, &QPushButton::clicked, this, [this]() {
          auto viewSizePixel = this->view->heightPhys();
          auto viewSizeM = viewSizePixel / this->view->physicalDpiY() * 0.0254;
          // qDebug() << "View size" << viewSizePixel << "px /" << viewSizeM
          //          << "m";
          this->properties->setViewSizeUnzoomed(viewSizeM);
        });
    controlLayout->addWidget(viewSizePhysicalButton, 4, 1);

    this->control3DSettings->setLayout(controlLayout);
  }

  this->planeWidget = new PlanePropertiesUi(view);

#ifdef ENABLE_OSVR
  auto osvrEnabled = new QCheckBox();
  controlLayout->addRow("OSVR", osvrEnabled);
  connect(osvrEnabled, &QCheckBox::stateChanged, this,
          [this, osvrEnabled](int state) {
            if (!state) {
              if (!this->osvrDisplay) return;
              osvrDisplay->deleteLater();
            } else {
              if (this->osvrDisplay) return;
              try {
                auto osvr = new OsvrDisplay(view->context(), this);
                this->osvrDisplay = osvr;
                connect(osvr, &QObject::destroyed, osvrEnabled,
                        [osvrEnabled] { osvrEnabled->setChecked(false); });
                connect(osvr, &OsvrDisplay::render, this,
                        [this](const QMatrix4x4& projectionMatrix,
                               const QMatrix4x4& viewMatrix) {
                          float distanceEyesCenter = 0.2;  // TODO
                          QMatrix4x4 translation;
                          translation.translate(0, 0, -distanceEyesCenter);
                          view->paint(viewMatrix * translation *
                                          view->getView3D()->viewMatrix(),
                                      projectionMatrix);
                        });
              } catch (vx::Exception& e) {
                qCritical() << "Failed to create OsvrDisplay:" << e.message();
              }
            }
          });

#if !defined(Q_OS_WIN)
  QPushButton* toggleSideBySide;
  controlLayout->addRow(
      "", (toggleSideBySide = new QPushButton("Toggle side-by-side")));
  connect(toggleSideBySide, &QPushButton::clicked, this, [this] {
    QFile file("/dev/ttyUSB.OSVRHDK");
    if (file.open(QIODevice::ReadWrite)) {
      const char* str = "\n#f1s\n";
      qint64 len = strlen(str);
      if (file.write(str, len) != len) {
        QMessageBox(QMessageBox::Critical, "OSVR",
                    QString("Error while writing to %1").arg(file.fileName()),
                    QMessageBox::Ok, view)
            .exec();
      }
    } else {
      QMessageBox(QMessageBox::Critical, "OSVR",
                  QString("Failed to open %1").arg(file.fileName()),
                  QMessageBox::Ok, view)
          .exec();
    }
  });
#endif

#endif

  auto sn = new vx::spnav::SpaceNavVisualizer(this);
  auto view3d = view->getView3D();
  view3d->registerSpaceNavVisualizer(sn);
  connect(view, &Visualizer3DView::objectPositionChangeRequested, this,
          [this](QVector3D offset) {
            axisFilter.filter(offset);

            auto selectedNodes =
                vx::voxieRoot().activeVisualizerProvider()->selectedNodes();
            if (!selectedNodes.count()) {
              qWarning() << "No node selected";
              return;
            }
            auto obj3D = dynamic_cast<Object3DNode*>(selectedNodes[0]);
            // TODO: allow a vx::SurfaceNode?
            if (!obj3D) {
              qWarning() << "Selected node not an Object3D";
              return;
            }

            // qDebug() << "Visualizer3D::positionChanged" << obj3D << offset;
            obj3D->objectPositionChangeRequested(offset);
          });
  connect(view, &Visualizer3DView::objectRotationChangeRequested, this,
          [this](QQuaternion rotation) {
            axisFilter.filter(rotation);

            auto selectedNodes =
                vx::voxieRoot().activeVisualizerProvider()->selectedNodes();
            if (!selectedNodes.count()) {
              qWarning() << "No node selected";
              return;
            }
            auto obj3D = dynamic_cast<Object3DNode*>(selectedNodes[0]);
            // TODO: allow a vx::SurfaceNode?
            if (!obj3D) {
              qWarning() << "Selected node not an Object3D";
              return;
            }

            // qDebug() << "Visualizer3D::rotationChanged" << obj3D << rotation;
            obj3D->objectRotationChangeRequested(rotation);
          });
}

QWidget* Visualizer3D::getCustomPropertySectionContent(const QString& name) {
  if (name == "de.uni_stuttgart.Voxie.Visualizer.View3D.Control3DSettings") {
    return control3DSettings;
  } else if (name == "de.uni_stuttgart.Voxie.Visualizer.View3D.PlaneSettings") {
    return planeWidget;
  } else {
    return Node::getCustomPropertySectionContent(name);
  }
}

void Visualizer3D::handleViewChange(QString direction) {
  this->view->setFixedAngle(direction);
}

void Visualizer3D::axisFilterChanged() {
  filterXButton->setChecked(!axisFilter.filterX());
  filterYButton->setChecked(!axisFilter.filterY());
  filterZButton->setChecked(!axisFilter.filterZ());
}

vx::SharedFunPtr<VisualizerNode::RenderFunction>
Visualizer3D::getRenderFunction() {
  return [this](const QSharedPointer<vx::ImageDataPixel>& outputImage,
                const vx::VectorSizeT2& outputRegionStart,
                const vx::VectorSizeT2& size,
                const QSharedPointer<vx::ParameterCopy>& parameters,
                const QSharedPointer<vx::VisualizerRenderOptions>& options) {
    // TODO: this lambda should not capture this (because if a screenshot
    // is rendered from a background thread this might be destroyed while
    // the function is running) and values from parameters should be used
    // instead
    Q_UNUSED(parameters);

    Q_UNUSED(options);

    this->view->renderScreenshot(outputImage, outputRegionStart, size);
  };
}
