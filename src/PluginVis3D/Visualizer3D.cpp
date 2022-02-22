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

#include <PluginVis3D/CamProperties.hpp>
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

using namespace vx;
using namespace vx::vis3d;
using namespace vx::visualization;

Visualizer3D::Visualizer3D()
    : VisualizerNode(getPrototypeSingleton()),
      properties(new View3DProperties(this)),
      mouseOperation(IsosurfaceMouseOperation::create()) {
  {
    this->view = new Visualizer3DView(
        properties, new vx::visualization::View3D(this, mouseOperation),
        mouseOperation, &axisFilter);
    new View3DPropertiesConnection(this->properties, this->view->getView3D());
    this->view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    this->view->setWindowTitle("3D Visualizer");
    this->setAutomaticDisplayName("3D Visualizer");
    this->view->setMinimumSize(400, 300);
  }

  QFormLayout* controlLayout;
  {
    QWidget* sectionControl3DSettings = new QWidget();
    sectionControl3DSettings->setWindowTitle("Render Settings");
    {
      controlLayout = new QFormLayout();
      {
        controlLayout->addRow(
            "Projection",
            (this->orthoProjectionBox = new QCheckBox("&Orthographic")));
        orthoProjectionBox->setToolTip(
            "Toggles orthographic projection. Hotkey: o");
        this->orthoProjectionBox->setChecked(this->view->isOrtho());
        // Change the projection when the user clicks the checkbox
        connect(this->orthoProjectionBox, &QCheckBox::clicked, this->view,
                &Visualizer3DView::switchProjection);
        // If we change the projection using the hotkey, we also want to update
        // the status of the checkbox
        connect(view, SIGNAL(projectionChanged()), this,
                SLOT(updateOrthoBox()));

        QHBoxLayout* viewLayout = new QHBoxLayout;
        QHBoxLayout* viewLayout2 = new QHBoxLayout;

        QPushButton* frontViewButton;
        QPushButton* backViewButton;
        QPushButton* rightViewButton;
        QPushButton* leftViewButton;
        QPushButton* topViewButton;
        QPushButton* bottomViewButton;
        viewLayout->addWidget(frontViewButton = new QPushButton("Front"));
        viewLayout->addWidget(rightViewButton = new QPushButton("Right"));
        viewLayout->addWidget(topViewButton = new QPushButton("Top"));
        viewLayout2->addWidget(backViewButton = new QPushButton("Back"));
        viewLayout2->addWidget(leftViewButton = new QPushButton("Left"));
        viewLayout2->addWidget(bottomViewButton = new QPushButton("Bottom"));

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

        QLabel* viewLabel = new QLabel("Views");
        viewLabel->setToolTip(
            "Hotkeys for views: \n"
            "(Ctrl +) 1 : Back/Front \n"
            "(Ctrl +) 3 : Left/Right \n"
            "(Ctrl +) 7 : Bottom/Top \n"
            "4,6 : Rotate Left/Right \n"
            "2,8 : Rotate Down/Up");

        controlLayout->addRow(viewLabel, viewLayout);
        controlLayout->addItem(viewLayout2);
      }
      sectionControl3DSettings->setLayout(controlLayout);
    }
    auto objectWidget = createObjectWidget();
    auto mouseOperationWidget = createMouseOperationWidget();

    CamProperties* camProp = new CamProperties(view);
    settingUpCamProperties(camProp);

    this->planeWidget = new PlanePropertiesUi(view);

    this->dynamicSections().append(objectWidget);
    this->dynamicSections().append(mouseOperationWidget);
    this->dynamicSections().append(this->planeWidget);
    this->dynamicSections().append(camProp);
    this->dynamicSections().append(sectionControl3DSettings);
    this->dynamicSections().squeeze();
  }

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
  connect(view3d, &vx::visualization::View3D::objectPositionChangeRequested,
          this, [this](QVector3D offset) {
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
  connect(view3d, &vx::visualization::View3D::objectRotationChangeRequested,
          this, [this](QQuaternion rotation) {
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

  connect(mouseOperation.data(),
          &IsosurfaceMouseOperation::preferedActionChanged, this,
          &Visualizer3D::preferedMouseActionChanged);
}

void Visualizer3D::settingUpCamProperties(CamProperties* camProp) {
  camProp->setObjectName("Propertie Widget");
  camProp->setWindowTitle("Camera Properties");

  // GUI need the last valid Properties to reset itself
  connect(camProp, &CamProperties::rotationRequest, this->view->getView3D(),
          &View3D::cameraRotationRequested);
  connect(camProp, &CamProperties::zoomRequest, this->view->getView3D(),
          &View3D::zoomRequested);

  // Properties changed by input in the GUI
  connect(camProp, &CamProperties::rotationChanged, this->view->getView3D(),
          &View3D::setRotation);
  connect(camProp, &CamProperties::zoomChanged, this->view->getView3D(),
          &View3D::setZoom);

  // Properties changed by interaction with the object
  connect(this->view->getView3D(), &View3D::changed, camProp,
          &CamProperties::rotationRequest);
  connect(this->view->getView3D(), &View3D::zoomChanged, camProp,
          &CamProperties::setZoom);
  connect(this->view->getView3D(), &View3D::cameraRotationChanged, camProp,
          &CamProperties::setRotation);

  camProp->init();
}

void Visualizer3D::handleViewChange(QString direction) {
  this->view->setFixedAngle(direction);
}

QWidget* Visualizer3D::createMouseOperationWidget() {
  QVBoxLayout* mainLayout = new QVBoxLayout;
  QWidget* mouseOperationWidget = new QWidget;
  mouseOperationWidget->setWindowTitle("Mouse Operation");
  mouseOperationWidget->setLayout(mainLayout);

  QWidget* surfaceControlBox = new QWidget();
  {
    QHBoxLayout* hBoxLayout = new QHBoxLayout;
    surfaceControlBox->setLayout(hBoxLayout);
    moveSurfaceButton =
        new QPushButton(QIcon(":/icons/move-surface.png"), "Move");
    // moveSurfaceButton->setFixedSize(16,16);
    moveSurfaceButton->setCheckable(true);
    connect(moveSurfaceButton, &QPushButton::clicked, [=]() {
      if (moveSurfaceButton->isChecked())
        mouseOperation->setPreferedAction(MouseOperation::MoveObject);
      else
        mouseOperation->setPreferedAction(MouseOperation::RotateView);
    });
    hBoxLayout->addWidget(moveSurfaceButton);

    rotateSurfaceButton =
        new QPushButton(QIcon(":/icons/rotate-surface.png"), "Rotate");
    // rotateSurfaceButton->setFixedSize(16,16);
    rotateSurfaceButton->setCheckable(true);
    connect(rotateSurfaceButton, &QPushButton::clicked, [=]() {
      if (rotateSurfaceButton->isChecked())
        mouseOperation->setPreferedAction(MouseOperation::RotateObject);
      else
        mouseOperation->setPreferedAction(MouseOperation::RotateView);
    });
    hBoxLayout->addWidget(rotateSurfaceButton);

    selectSurfaceButton =
        new QPushButton(QIcon(":/icons/select-surface.png"), "Select");
    // selectSurfaceButton->setFixedSize(16,16);
    selectSurfaceButton->setCheckable(true);
    connect(selectSurfaceButton, &QPushButton::clicked, [=]() {
      if (selectSurfaceButton->isChecked())
        mouseOperation->setPreferedAction(MouseOperation::SelectObject);
      else
        mouseOperation->setPreferedAction(MouseOperation::RotateView);
    });
    hBoxLayout->addWidget(selectSurfaceButton);

    addDataPointButton =
        new QPushButton(QIcon(":/icons/select-surface.png"), "Add point");
    // selectSurfaceButton->setFixedSize(16,16);
    addDataPointButton->setCheckable(true);
    connect(addDataPointButton, &QPushButton::clicked, [=]() {
      if (addDataPointButton->isChecked())
        mouseOperation->setPreferedAction(MouseOperation::SetPoint);
      else
        mouseOperation->setPreferedAction(MouseOperation::RotateView);
    });
    hBoxLayout->addWidget(addDataPointButton);
  }

  axisControlBox = new QWidget();
  {
    axisControlBox->hide();
    QHBoxLayout* hBoxLayout = new QHBoxLayout;
    axisControlBox->setLayout(hBoxLayout);

    filterXButton = new QPushButton("X");
    {
      filterXButton->setCheckable(true);
      connect(filterXButton, &QPushButton::toggled,
              [=] { axisFilter.setFilterX(!filterXButton->isChecked()); });
    }
    hBoxLayout->addWidget(filterXButton);

    filterYButton = new QPushButton("Y");
    {
      filterYButton->setCheckable(true);
      connect(filterYButton, &QPushButton::toggled,
              [=] { axisFilter.setFilterY(!filterYButton->isChecked()); });
    }
    hBoxLayout->addWidget(filterYButton);

    filterZButton = new QPushButton("Z");
    {
      filterZButton->setCheckable(true);
      connect(filterZButton, &QPushButton::toggled,
              [=] { axisFilter.setFilterZ(!filterZButton->isChecked()); });
    }
    hBoxLayout->addWidget(filterZButton);

    /*
    movePlaneButton = new QPushButton("Plane");
    {
      movePlaneButton->setCheckable(true);
      connect(movePlaneButton, &QPushButton::toggled,
              [=] { movePlane = movePlaneButton->isChecked(); });
    }
    hBoxLayout->addWidget(movePlaneButton);
    */

    connect(&axisFilter, &AxisFilter::changed, this,
            &Visualizer3D::axisFilterChanged);
    axisFilterChanged();
  }

  mainLayout->addWidget(surfaceControlBox);
  mainLayout->addWidget(axisControlBox);
  return mouseOperationWidget;
}

void Visualizer3D::axisFilterChanged() {
  filterXButton->setChecked(!axisFilter.filterX());
  filterYButton->setChecked(!axisFilter.filterY());
  filterZButton->setChecked(!axisFilter.filterZ());
}

QWidget* Visualizer3D::createObjectWidget() {
  QWidget* objectWidget = new QWidget();
  QFormLayout* formLayout = new QFormLayout();
  objectWidget->setLayout(formLayout);
  objectWidget->setWindowTitle("Objects");

  return objectWidget;
}

void Visualizer3D::updateOrthoBox() {
  orthoProjectionBox->setChecked(!orthoProjectionBox->isChecked());
}

/**
 * @brief Visualizer3D::preferedMouseActionChanged Sets all the button
 * states according to the prefered mouse action
 */
void Visualizer3D::preferedMouseActionChanged() {
  auto newAction = mouseOperation->getPreferedAction();
  QPushButton* buttonToPush = NULL;
  switch (newAction) {
    case MouseOperation::Action::MoveObject: {
      buttonToPush = moveSurfaceButton;
      break;
    }
    case MouseOperation::Action::RotateObject: {
      buttonToPush = rotateSurfaceButton;
      break;
    }
    case MouseOperation::SelectObject: {
      buttonToPush = selectSurfaceButton;
      break;
    }
    case MouseOperation::SetPoint: {
      buttonToPush = addDataPointButton;
      break;
    }

    default: {
      // Do nothing
    }
  }
  if (buttonToPush != NULL)
    buttonToPush->setChecked(true);
  else
    axisControlBox->hide();

  // Uncheck all other buttons:
  if (buttonToPush != moveSurfaceButton)
    moveSurfaceButton->setChecked(false);
  else
    axisControlBox->show();

  if (buttonToPush != rotateSurfaceButton)
    rotateSurfaceButton->setChecked(false);
  else
    axisControlBox->show();

  if (buttonToPush != addDataPointButton)
    addDataPointButton->setChecked(false);
  else
    axisControlBox->hide();

  if (buttonToPush != selectSurfaceButton)
    selectSurfaceButton->setChecked(false);
  else
    axisControlBox->hide();
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

NODE_PROTOTYPE_IMPL_SEP(View3D, Visualizer3D)
